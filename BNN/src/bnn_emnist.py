"""
Binarized Neural Network (BNN) for EMNIST, sized for a Lattice iCE40UP5K.

Target budget (iCE40UP5K): ~5.3K LUTs, 30 EBR x 4 Kbit = 120 Kbit, 4 SPRAM x 256 Kbit
= 1 Mbit, 8 DSPs. That rules out a CNN (activation buffers + control blow the LUT
budget), so the default model is a fully-binary MLP:

    28x28 pixels --sign--> 784 bits
      -> BinaryDense(512) -> BN -> sign      (folds to: popcount >= threshold)
      -> BinaryDense(512) -> BN -> sign
      -> BinaryDense(512) -> BN -> sign
      -> BinaryDense(C)   -> argmax(popcount) (no BN: argmax is invariant to it)

Every layer, including the first, is XNOR + popcount + integer compare, so the
FPGA needs ONE datapath reused per layer. Weights: ~0.97 Mbit -> 93% of SPRAM,
with 4 blocks read in parallel as one 64-bit word per cycle.

Training tricks:
  * Forward: sign() to {-1,+1}. Backward: straight-through estimator (|x|<=1).
  * Real-valued latent weights for the optimizer, clipped to [-1, 1].
  * BN with scale=False (gamma fixed at +1) so BN+sign is always a ">=" threshold.
  * A single trainable scalar on the output popcounts sets softmax temperature
    without changing the argmax the hardware computes.

Usage:
  python bnn_emnist.py                              # balanced split, 784-512-512-512-47
  python bnn_emnist.py --hidden 128                 # 784-128-47, fits in EBR alone
  python bnn_emnist.py --model cnn                  # float-friendly accuracy ceiling
  python bnn_emnist.py --export fpga_out            # .mem files + golden-model check
"""
import argparse
import math
import os

import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

WORD = 64            # bits per memory word = 4 SPRAM blocks x 16 bits
SPRAM_BITS = 4 * 256 * 1024
EBR_BITS = 30 * 4096

# ---------------------------------------------------------------------------
# Binarization primitives
# ---------------------------------------------------------------------------

@tf.custom_gradient
def ste_sign(x):
    """sign(x) in {-1,+1} forward (0 -> +1); hard-tanh STE gradient backward."""
    y = tf.where(x >= 0, tf.ones_like(x), -tf.ones_like(x))

    def grad(dy):
        return dy * tf.cast(tf.abs(x) <= 1.0, dy.dtype)

    return y, grad


class LatentClip(keras.constraints.Constraint):
    def __call__(self, w):
        return tf.clip_by_value(w, -1.0, 1.0)


class BinaryActivation(layers.Layer):
    def call(self, x):
        return ste_sign(x)


class BinaryDense(layers.Dense):
    """Dense layer with binarized weights, no bias (BN absorbs it)."""

    def __init__(self, units, **kw):
        kw.setdefault("use_bias", False)
        kw.setdefault("kernel_initializer", keras.initializers.RandomUniform(-1, 1))
        kw.setdefault("kernel_constraint", LatentClip())
        super().__init__(units, **kw)

    def call(self, x):
        return tf.matmul(x, ste_sign(self.kernel))


class BinaryConv2D(layers.Conv2D):
    def __init__(self, filters, kernel_size, **kw):
        kw.setdefault("use_bias", False)
        kw.setdefault("padding", "same")
        kw.setdefault("kernel_initializer", keras.initializers.RandomUniform(-1, 1))
        kw.setdefault("kernel_constraint", LatentClip())
        super().__init__(filters, kernel_size, **kw)

    def call(self, x):
        return tf.nn.conv2d(x, ste_sign(self.kernel), strides=self.strides,
                            padding=self.padding.upper(), dilations=self.dilation_rate)


class ScaleLogits(layers.Layer):
    """logits = s * z with one trainable scalar s (argmax-invariant)."""

    def __init__(self, init=0.05, **kw):
        super().__init__(**kw)
        self.init = init

    def build(self, _):
        self.s = self.add_weight(name="s", shape=(),
                                 initializer=keras.initializers.Constant(self.init))

    def call(self, z):
        return z * self.s


# ---------------------------------------------------------------------------
# Models
# ---------------------------------------------------------------------------

BN = dict(momentum=0.9, epsilon=1e-4, scale=False)


def build_fpga_mlp(num_classes, hidden=(256, 256)):
    inp = keras.Input((28, 28, 1))
    x = layers.Flatten()(inp)
    x = BinaryActivation(name="binarize_input")(x)      # pixels -> {-1,+1}
    for h in hidden:
        x = BinaryDense(h)(x)
        x = layers.BatchNormalization(**BN)(x)
        x = BinaryActivation()(x)
    z = BinaryDense(num_classes, name="out_popcount")(x)
    x = ScaleLogits(init=1.0 / math.sqrt(hidden[-1]))(z)
    out = layers.Activation("softmax", dtype="float32")(x)
    return keras.Model(inp, out, name="bnn_fpga_mlp")


def build_cnn(num_classes):
    """Reference only - too big for a UP5K. Use it to see the accuracy ceiling."""
    inp = keras.Input((28, 28, 1))
    x = inp
    for f, pool in [(64, False), (64, True), (128, False), (128, True), (256, True)]:
        x = BinaryConv2D(f, 3)(x)
        if pool:
            x = layers.MaxPooling2D()(x)
        x = layers.BatchNormalization(**BN)(x)
        x = BinaryActivation()(x)
    x = layers.Flatten()(x)
    x = BinaryDense(512)(x); x = layers.BatchNormalization(**BN)(x)
    x = BinaryActivation()(x)
    x = BinaryDense(num_classes)(x)
    x = layers.BatchNormalization(momentum=0.9, epsilon=1e-4)(x)
    out = layers.Activation("softmax", dtype="float32")(x)
    return keras.Model(inp, out, name="bnn_cnn")


# ---------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------

def prep_np(img):
    """uint8 (N,28,28[,1]) in EMNIST storage order -> float (N,28,28,1) in [-1,1]."""
    img = img.reshape(-1, 28, 28).transpose(0, 2, 1)     # EMNIST is stored transposed
    return (img.astype(np.float32) / 127.5 - 1.0)[..., None]


def load_emnist(split, batch, val_frac=0.0, shift=2):
    """-> train, val, test, num_classes. val is the tail of train, or test if val_frac=0."""
    import tensorflow_datasets as tfds

    if val_frac:
        pct = round(100 * (1 - val_frac))
        names = [f"train[:{pct}%]", f"train[{pct}%:]", "test"]
    else:
        names = ["train", "test", "test"]
    (train, val, test), info = tfds.load(f"emnist/{split}", split=names,
                                         as_supervised=True, with_info=True)
    num_classes = info.features["label"].num_classes

    def prep(img, label):
        img = tf.cast(tf.transpose(img, [1, 0, 2]), tf.float32) / 127.5 - 1.0
        return img, label

    def augment(img, label):  # random +/-shift px, background stays -1
        size = 28 + 2 * shift
        img = tf.image.pad_to_bounding_box(img + 1.0, shift, shift, size, size) - 1.0
        return tf.image.random_crop(img, (28, 28, 1)), label

    def evaluation(ds):
        return ds.map(prep, tf.data.AUTOTUNE).batch(batch).cache().prefetch(tf.data.AUTOTUNE)

    train = train.map(prep, tf.data.AUTOTUNE).cache().shuffle(20000)
    if shift:
        train = train.map(augment, tf.data.AUTOTUNE)
    train = train.batch(batch).prefetch(tf.data.AUTOTUNE)
    return train, evaluation(val), evaluation(test), num_classes


# ---------------------------------------------------------------------------
# FPGA export + bit-exact golden model
# ---------------------------------------------------------------------------

def extract_layers(model):
    """[(weight_bits (N,M) in {0,1}, popcount_threshold (M,) or None), ...]"""
    out, lyrs = [], model.layers
    for i, l in enumerate(lyrs):
        if not isinstance(l, BinaryDense):
            continue
        w = (l.kernel.numpy() >= 0).astype(np.uint8)        # +1 -> 1, -1 -> 0
        N = w.shape[0]
        nxt = lyrs[i + 1] if i + 1 < len(lyrs) else None
        th = None
        if isinstance(nxt, layers.BatchNormalization):
            # sign(BN(z)) = +1  <=>  z >= T,  T = mu - beta*sqrt(var+eps)  (gamma = 1)
            # z = 2p - N (p = popcount of XNOR)  <=>  p >= ceil((T + N) / 2)
            mu, var = nxt.moving_mean.numpy(), nxt.moving_variance.numpy()
            T = mu - nxt.beta.numpy() * np.sqrt(var + nxt.epsilon)
            th = np.clip(np.ceil((T + N) / 2.0), 0, N + 1).astype(np.int64)
        out.append((w, th))
    return out


def input_bits(x):
    """float images in [-1,1] (N,28,28,1) -> (N,784) bits, matching sign() (0 -> 1)."""
    return (x.reshape(len(x), -1) >= 0).astype(np.uint8)


def xnor_popcount(a, w):
    """popcount(XNOR(a, w)) per neuron for 0/1 arrays a (B,N), w (N,M).

    float32 matmul goes through BLAS (int64 does not, ~100x slower) and is exact:
    every partial sum is an integer <= N, far below 2**24.
    """
    a, w = a.astype(np.float32), w.astype(np.float32)
    return (a @ w + (1 - a) @ (1 - w)).astype(np.int64)


def golden_infer(bits, net):
    """Integer-only reference: XNOR, popcount, compare, argmax. Mirrors the RTL."""
    a = bits
    for w, th in net:
        p = xnor_popcount(a, w)
        if th is None:
            return np.argmax(p, axis=1), p     # first max wins -> use strict '>' in RTL
        a = (p >= th).astype(np.int64)
    raise ValueError("network has no output layer")


def pack_words(bits_row, pad_value):
    """1-D bit vector -> list of WORD-bit ints; bit j of word k = element k*WORD + j."""
    n = len(bits_row)
    nw = -(-n // WORD)
    padded = np.full(nw * WORD, pad_value, np.uint8)
    padded[:n] = bits_row
    return [int("".join(str(b) for b in padded[k * WORD:(k + 1) * WORD][::-1]), 2)
            for k in range(nw)]


def write_mem(path, words, width_bits):
    digits = -(-width_bits // 4)
    with open(path, "w") as f:
        for w in words:
            f.write(f"{w:0{digits}x}\n")


def export_fpga(model, outdir, x_test, y_test, n_vectors=100):
    os.makedirs(outdir, exist_ok=True)
    net = extract_layers(model)

    # Resource report ------------------------------------------------------
    print("\nLayer  N_in  N_out  words/neuron  weight bits  thresh bits")
    total = 0
    for i, (w, th) in enumerate(net):
        N, M = w.shape
        wpn = -(-N // WORD)
        bits = wpn * WORD * M
        total += bits
        tb = math.ceil(math.log2(N + 2))
        print(f"{i:>5}  {N:>4}  {M:>5}  {wpn:>12}  {bits:>11,}  "
              f"{tb if th is not None else '-':>11}")
    words = total // WORD
    print(f"Total weight bits: {total:,}  ({total / SPRAM_BITS:.0%} of SPRAM, "
          f"{total / EBR_BITS:.0%} of EBR)")
    print(f"Memory reads per inference: {words:,} x {WORD}-bit words "
          f"(~{words / 24e6 * 1e6:.0f} us at 24 MHz, 1 word/cycle)")

    # Memory images ---------------------------------------------------------
    # Pad convention: input pad bits = 0, weight pad bits = 1 -> XNOR = 0, so
    # padding never adds to the popcount and thresholds need no correction.
    for i, (w, th) in enumerate(net):
        wordlist = []
        for m in range(w.shape[1]):          # neuron-major: neuron m, word 0..k-1
            wordlist += pack_words(w[:, m], pad_value=1)
        write_mem(os.path.join(outdir, f"l{i}_weights.mem"), wordlist, WORD)
        if th is not None:
            tb = math.ceil(math.log2(w.shape[0] + 2))
            write_mem(os.path.join(outdir, f"l{i}_thresh.mem"), th.tolist(), tb)

    # Golden-model check against Keras --------------------------------------
    bits = input_bits(x_test)
    pred_hw, _ = golden_infer(bits, net)
    pred_keras = np.argmax(model.predict(x_test, batch_size=1024, verbose=0), axis=1)
    agree = np.mean(pred_hw == pred_keras)
    print(f"Golden model vs Keras agreement: {agree:.4%} "
          f"({np.sum(pred_hw != pred_keras)} mismatches)")
    if y_test is not None:
        print(f"Golden model accuracy: {np.mean(pred_hw == y_test):.4%}")

    # Testbench vectors ------------------------------------------------------
    n = min(n_vectors, len(bits))
    vec = []
    for b in bits[:n]:
        vec += pack_words(b, pad_value=0)
    write_mem(os.path.join(outdir, "test_inputs.mem"), vec, WORD)
    np.savetxt(os.path.join(outdir, "test_expected.txt"),
               np.stack([pred_hw[:n], y_test[:n] if y_test is not None else pred_hw[:n]], 1),
               fmt="%d", header="golden_pred label")
    print(f"Wrote memory images + {n} test vectors to {outdir}/")
    return agree


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--split", default="balanced",
                   choices=["balanced", "byclass", "bymerge", "digits", "letters", "mnist"])
    p.add_argument("--model", default="fpga", choices=["fpga", "cnn"])
    p.add_argument("--hidden", default="512,512,512", help="hidden widths for the fpga MLP")
    p.add_argument("--epochs", type=int, default=60)
    p.add_argument("--batch", type=int, default=256)
    p.add_argument("--lr", type=float, default=3e-2)
    p.add_argument("--export", default=None, help="output dir for .mem files")
    p.add_argument("--ckpt", default=None, help="checkpoint path (default bnn_emnist_<model>.weights.h5)")
    p.add_argument("--val-frac", type=float, default=0.1,
                   help="fraction of train held out to pick the best epoch (0 = use test)")
    p.add_argument("--shift", type=int, default=1, help="max random shift augmentation in px")
    p.add_argument("--seed", type=int, default=None, help="random seed (default: unseeded)")
    a = p.parse_args()
    if a.seed is not None:
        keras.utils.set_random_seed(a.seed)

    train, val, test, num_classes = load_emnist(a.split, a.batch, a.val_frac, a.shift)
    if a.model == "fpga":
        model = build_fpga_mlp(num_classes, tuple(int(h) for h in a.hidden.split(",")))
    else:
        model = build_cnn(num_classes)
    model.summary()

    steps = a.epochs * int(train.cardinality())
    model.compile(
        optimizer=keras.optimizers.Adam(keras.optimizers.schedules.CosineDecay(a.lr, steps, alpha=0.01)),
        loss="sparse_categorical_crossentropy", metrics=["accuracy"])
    ckpt = a.ckpt or f"bnn_emnist_{a.model}.weights.h5"
    if os.path.dirname(ckpt):
        os.makedirs(os.path.dirname(ckpt), exist_ok=True)
    model.fit(train, validation_data=val, epochs=a.epochs,
              callbacks=[keras.callbacks.ModelCheckpoint(
                  ckpt, save_best_only=True, save_weights_only=True, monitor="val_accuracy")])
    model.load_weights(ckpt)
    print("Val  [loss, acc]:", model.evaluate(val, verbose=0))
    print("Test [loss, acc]:", model.evaluate(test, verbose=0))

    if a.export:
        if a.model != "fpga":
            raise SystemExit("--export is for the fpga MLP only")
        xs, ys = zip(*[(x.numpy(), y.numpy()) for x, y in test.unbatch()])
        export_fpga(model, a.export, np.stack(xs), np.array(ys))


if __name__ == "__main__":
    main()
