"""Fails unless TensorFlow really runs on the GPU (it silently falls back to CPU otherwise)."""
import sys
import tensorflow as tf
gpus = tf.config.list_physical_devices("GPU")
print("TF", tf.__version__, "GPUs:", gpus)
if not gpus:
    sys.exit("FAIL: no GPU visible")
tf.config.set_soft_device_placement(False)
with tf.device("/GPU:0"):
    x = tf.random.normal((2048, 2048))
    y = x @ x
print("matmul ran on:", y.device, "sum", float(tf.reduce_sum(y)))
if "GPU" not in y.device:
    sys.exit("FAIL: matmul not on GPU")
print("GPU_OK")
