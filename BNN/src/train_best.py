"""
Accuracy-focused trainer for the flash-resident BNN. Same network and same export
path as bnn_emnist.py (fully binary MLP: XNOR, popcount, threshold, argmax), so
nothing here changes the hardware. What it adds is the training recipe:

  * The dataset sits on the GPU and augmentation runs there, batch by batch.
  * Affine + elastic augmentation, applied to the grayscale image before it is
    binarized, so a distorted stroke is thresholded like a real one.
  * Distillation from a float CNN teacher (--mode teacher trains one). The teacher
    sees grayscale, so its soft targets carry information the binary input lost.
  * BN recalibration: thresholds are recomputed on clean training images, because
    the running statistics were collected on distorted ones.

Selection discipline: epochs, recipes and shapes are all chosen on the validation
split (the last 10% of train, as in bnn_emnist.py). The test split is only read
with --final.

Usage (through src/gpu_run.sh):
  train_best.py --mode teacher --split balanced --out build/exp/best/t_balanced
  train_best.py --split balanced --hidden 1152,1152,1152,1152,1152,1152 \
      --teacher build/exp/best/t_balanced.weights.h5 --out build/exp/best/run1
"""
import argparse
import json
import math
import os
import time

import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

from bnn_emnist import WORD, build_fpga_mlp, extract_layers, golden_infer

CACHE = "build/exp/best/cache"
FLASH_BPS = 96e6        # quad SPI at 24 MHz
THRESH_BITS = 16        # one stored word per threshold


def flash_bits(hidden, num_classes=47, n_in=784):
    """Bits read from flash per inference: padded weights plus thresholds."""
    sizes = [n_in, *hidden, num_classes]
    w = sum(-(-a // WORD) * WORD * b for a, b in zip(sizes, sizes[1:]))
    return w + THRESH_BITS * sum(hidden)


# ---------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------

def load_arrays(split, val_frac=0.1):
    """uint8 upright images and int32 labels, using the same slices as bnn_emnist.py."""
    path = f"{CACHE}/{split}.npz"
    if not os.path.exists(path):
        import tensorflow_datasets as tfds
        os.makedirs(CACHE, exist_ok=True)
        pct = round(100 * (1 - val_frac))
        names = [f"train[:{pct}%]", f"train[{pct}%:]", "test"]
        parts, info = tfds.load(f"emnist/{split}", split=names, as_supervised=True,
                                with_info=True)
        arrs = {}
        for name, ds in zip(("train", "val", "test"), parts):
            xs, ys = [], []
            for x, y in tfds.as_numpy(ds.batch(8192)):
                xs.append(x[..., 0].transpose(0, 2, 1))     # EMNIST is stored transposed
                ys.append(y)
            arrs[f"x_{name}"] = np.concatenate(xs)
            arrs[f"y_{name}"] = np.concatenate(ys).astype(np.int32)
        np.savez(path, classes=info.features["label"].num_classes, **arrs)
    d = np.load(path)
    return {k: d[k] for k in d.files}


def smooth_upsampler(g, n=28):
    """(n, g) Gaussian interpolation matrix. A matmul, because XLA has no bicubic resize."""
    c = np.linspace(0, n - 1, g)
    w = np.exp(-0.5 * ((np.arange(n)[:, None] - c[None]) / (n / g * 0.8)) ** 2)
    return (w / np.sqrt((w ** 2).sum(1, keepdims=True))).astype(np.float32)


def sample_grid(b, a):
    """Per-image sampling coordinates (b,28,28,2): random affine plus smooth elastic field."""
    u = lambda lo, hi: tf.random.uniform((b,), lo, hi)
    th = u(-a.rot, a.rot) * (math.pi / 180)
    sh = u(-a.shear, a.shear) * (math.pi / 180)
    sx, sy = 1 + u(-a.scale, a.scale), 1 + u(-a.scale, a.scale)
    tx, ty = u(-a.trans, a.trans), u(-a.trans, a.trans)
    c, s, t = tf.cos(th), tf.sin(th), tf.tan(sh)
    r = (tf.range(28, dtype=tf.float32) - 13.5)
    gy, gx = tf.meshgrid(r, r, indexing="ij")
    gx, gy = gx[None], gy[None]
    e = lambda v: v[:, None, None]
    xs = e(sx) * (e(c) * gx - e(s) * gy) + e(t) * gy + e(tx) + 13.5
    ys = e(sy) * (e(s) * gx + e(c) * gy) + e(ty) + 13.5
    if a.elastic > 0:
        g = a.elastic_grid
        up = tf.constant(smooth_upsampler(g))
        coarse = tf.random.normal((b, 2, g, g), stddev=a.elastic)
        d = tf.einsum("ij,bcjk,lk->bcil", up, coarse, up)
        xs, ys = xs + d[:, 0], ys + d[:, 1]
    return xs, ys


def warp(img, xs, ys):
    """Bilinear sample of img (b,28,28) at (xs, ys); outside the image reads as background."""
    p = tf.pad(img, [[0, 0], [1, 1], [1, 1]])
    xs = tf.clip_by_value(xs + 1, 0.0, 28.999)
    ys = tf.clip_by_value(ys + 1, 0.0, 28.999)
    x0, y0 = tf.floor(xs), tf.floor(ys)
    fx, fy = xs - x0, ys - y0
    x0, y0 = tf.cast(x0, tf.int32), tf.cast(y0, tf.int32)
    flat = tf.reshape(p, (-1, 900))
    at = lambda y, x: tf.gather(flat, y * 30 + x, batch_dims=1)
    idx = lambda t: tf.reshape(t, (tf.shape(img)[0], 784))
    y0f, x0f = idx(y0), idx(x0)
    v00, v01 = at(y0f, x0f), at(y0f, x0f + 1)
    v10, v11 = at(y0f + 1, x0f), at(y0f + 1, x0f + 1)
    wx, wy = idx(fx), idx(fy)
    out = (v00 * (1 - wx) + v01 * wx) * (1 - wy) + (v10 * (1 - wx) + v11 * wx) * wy
    return tf.reshape(out, tf.shape(img))


# ---------------------------------------------------------------------------
# Teacher: float CNN, never deployed
# ---------------------------------------------------------------------------

def build_teacher(num_classes):
    inp = keras.Input((28, 28, 1))
    x = inp
    for f, pool in [(64, False), (64, True), (128, False), (128, True), (256, False), (256, False)]:
        x = layers.Conv2D(f, 3, padding="same", use_bias=False)(x)
        x = layers.BatchNormalization()(x)
        x = layers.ReLU()(x)
        if pool:
            x = layers.MaxPooling2D()(x)
    x = layers.GlobalAveragePooling2D()(x)
    x = layers.Dropout(0.3)(x)
    return keras.Model(inp, layers.Dense(num_classes)(x), name="teacher")


# ---------------------------------------------------------------------------
# Student helpers
# ---------------------------------------------------------------------------

def logits_model(m):
    """The student without its final softmax (ScaleLogits output)."""
    return keras.Model(m.input, m.layers[-2].output)


def predict(fn, x, batch=4096):
    return np.concatenate([fn(tf.constant(x[i:i + batch])).numpy()
                           for i in range(0, len(x), batch)])


def recalibrate_bn(m, x, batch=4096):
    """Set each BN's running stats from clean images, in layer order, since each layer's
    inputs depend on the thresholds of the one before."""
    for l in m.layers:
        if not isinstance(l, layers.BatchNormalization):
            continue
        sub = keras.Model(m.input, l.input)
        fn = tf.function(lambda t: sub(t, training=False))
        s = s2 = 0.0
        for i in range(0, len(x), batch):
            z = fn(tf.constant(x[i:i + batch])).numpy().astype(np.float64)
            s = s + z.sum(0)
            s2 = s2 + (z * z).sum(0)
        mean = s / len(x)
        l.moving_mean.assign(mean.astype(np.float32))
        l.moving_variance.assign((s2 / len(x) - mean * mean).astype(np.float32))


def to_student(x_u8, thr):
    """uint8 -> values whose sign is the binarized pixel. The model applies the sign."""
    return (x_u8.astype(np.float32) / 255.0 - thr)[..., None]


# ---------------------------------------------------------------------------
# Training
# ---------------------------------------------------------------------------

def train(a):
    if a.seed is not None:
        keras.utils.set_random_seed(a.seed)
    d = load_arrays(a.split)
    classes = int(d["classes"])
    x_tr, y_tr = d["x_train"], d["y_train"]
    n = len(x_tr)
    steps_ep = n // a.batch
    total = a.epochs * steps_ep
    teacher_mode = a.mode == "teacher"

    gpu_x = tf.constant(x_tr)
    gpu_y = tf.constant(y_tr)
    sched = keras.optimizers.schedules.CosineDecay(
        a.lr, total - a.warmup * steps_ep, alpha=0.01,
        warmup_target=a.lr if a.warmup else None,
        warmup_steps=a.warmup * steps_ep) if a.warmup else \
        keras.optimizers.schedules.CosineDecay(a.lr, total, alpha=0.01)
    opt = keras.optimizers.Adam(sched)

    teacher = None
    if teacher_mode:
        model = build_teacher(classes)
        net = model
    else:
        hidden = tuple(int(h) for h in a.hidden.split(","))
        model = build_fpga_mlp(classes, hidden)
        net = logits_model(model)
        if a.teacher:
            teacher = build_teacher(classes)
            teacher.load_weights(a.teacher)
            teacher.trainable = False
    opt.build(model.trainable_variables)

    loss_acc = tf.Variable(0.0, trainable=False)
    hit_acc = tf.Variable(0.0, trainable=False)
    T, alpha, thr, smooth = a.temp, a.alpha, a.thr, a.smooth

    def step(idx):
        img = tf.cast(tf.gather(gpu_x, idx), tf.float32) / 255.0
        y = tf.gather(gpu_y, idx)
        xs, ys = sample_grid(tf.shape(idx)[0], a)
        img = warp(img, xs, ys)[..., None]
        inp = img * 2 - 1 if teacher_mode else img - thr
        soft = None
        if teacher is not None:
            soft = tf.nn.softmax(teacher(img * 2 - 1, training=False) / T)
        with tf.GradientTape() as tape:
            z = net(inp, training=True)
            oh = tf.one_hot(y, classes)
            if smooth:
                oh = oh * (1 - smooth) + smooth / classes
            ce = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(oh, z))
            loss = ce
            if soft is not None:
                kd = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(soft, z / T))
                loss = (1 - alpha) * ce + alpha * T * T * kd
        g = tape.gradient(loss, model.trainable_variables)
        opt.apply(g, model.trainable_variables)
        loss_acc.assign_add(ce)
        hit_acc.assign_add(tf.reduce_mean(tf.cast(
            tf.equal(tf.argmax(z, 1, output_type=tf.int32), y), tf.float32)))

    step = tf.function(step, jit_compile=True)

    infer = tf.function(lambda t: net(t, training=False))
    prep = (lambda u: u.astype(np.float32)[..., None] / 127.5 - 1) if teacher_mode \
        else (lambda u: to_student(u, thr))
    x_val, y_val = prep(d["x_val"]), d["y_val"]

    def val_acc():
        return float(np.mean(predict(infer, x_val).argmax(1) == y_val))

    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    ckpt = a.out + ".weights.h5"
    best, best_ep, t0 = -1.0, -1, time.time()
    rng = np.random.default_rng(a.seed)
    for ep in range(a.epochs):
        loss_acc.assign(0.0)
        hit_acc.assign(0.0)
        perm = rng.permutation(n)[:steps_ep * a.batch].reshape(steps_ep, a.batch).astype(np.int32)
        for idx in tf.constant(perm):
            step(idx)
        v = val_acc()
        if v > best:
            best, best_ep = v, ep
            model.save_weights(ckpt)
        print(f"ep {ep + 1}/{a.epochs} loss {loss_acc.numpy() / steps_ep:.4f} "
              f"train {hit_acc.numpy() / steps_ep:.4f} val {v:.4f} best {best:.4f} "
              f"t {time.time() - t0:.0f}s", flush=True)

    model.load_weights(ckpt)
    row = dict(tag=os.path.basename(a.out), split=a.split, mode=a.mode, val_ckpt=best,
               best_epoch=best_ep + 1, train_last=float(hit_acc.numpy() / steps_ep),
               seconds=round(time.time() - t0), args=vars(a))

    if not teacher_mode:
        row["hidden"] = a.hidden
        row["flash_mbit"] = flash_bits(hidden, classes) / 1e6
        row["flash_ms"] = flash_bits(hidden, classes) / FLASH_BPS * 1e3
        if a.recal:
            saved = [(l, l.moving_mean.numpy(), l.moving_variance.numpy())
                     for l in model.layers if isinstance(l, layers.BatchNormalization)]
            recalibrate_bn(model, prep(x_tr))
            v = val_acc()
            row["val_recal"] = v
            if v > best:
                model.save_weights(ckpt)
            else:
                for l, mu, var in saved:
                    l.moving_mean.assign(mu)
                    l.moving_variance.assign(var)
        row["val"] = max(best, row.get("val_recal", -1))
        if a.final:
            # Integer golden model, exactly what the RTL computes.
            bits = (prep(d["x_test"]).reshape(len(d["x_test"]), -1) >= 0).astype(np.uint8)
            pred, _ = golden_infer(bits, extract_layers(model))
            row["test_golden"] = float(np.mean(pred == d["y_test"]))
    else:
        row["val"] = best
        if a.final:
            x_te = prep(d["x_test"])
            row["test"] = float(np.mean(predict(infer, x_te).argmax(1) == d["y_test"]))

    with open(a.log, "a") as f:
        f.write(json.dumps(row) + "\n")
    print("RESULT", {k: v for k, v in row.items() if k != "args"}, flush=True)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mode", default="student", choices=["student", "teacher"])
    p.add_argument("--split", default="balanced", choices=["balanced", "letters", "digits"])
    p.add_argument("--hidden", default="1152,1152,1152,1152,1152,1152")
    p.add_argument("--epochs", type=int, default=60)
    p.add_argument("--batch", type=int, default=256)
    p.add_argument("--lr", type=float, default=3e-2)
    p.add_argument("--warmup", type=int, default=0, help="warmup epochs")
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--thr", type=float, default=0.5, help="input binarization threshold, 0..1")
    p.add_argument("--rot", type=float, default=0.0, help="max rotation, degrees")
    p.add_argument("--shear", type=float, default=0.0, help="max shear, degrees")
    p.add_argument("--scale", type=float, default=0.0, help="max scale change, fraction")
    p.add_argument("--trans", type=float, default=1.0, help="max translation, px")
    p.add_argument("--elastic", type=float, default=0.0, help="elastic field stddev, px")
    p.add_argument("--elastic-grid", type=int, default=7, help="coarse elastic grid size")
    p.add_argument("--teacher", default=None, help="teacher weights for distillation")
    p.add_argument("--alpha", type=float, default=0.7, help="distillation loss weight")
    p.add_argument("--temp", type=float, default=2.0, help="distillation temperature")
    p.add_argument("--smooth", type=float, default=0.0, help="label smoothing")
    p.add_argument("--recal", type=int, default=1, help="recalibrate BN on clean images")
    p.add_argument("--final", action="store_true", help="also score the test split")
    p.add_argument("--out", required=True, help="output path stem")
    p.add_argument("--log", default="build/exp/best/runs.jsonl")
    train(p.parse_args())


if __name__ == "__main__":
    main()
