"""
Download and prepare an EMNIST split with TensorFlow Datasets.

The data lands in $TFDS_DATA_DIR (the Makefile points it at BNN/data), which
bnn_emnist.py and gen_vectors.py read from too.

Usage:
  python fetch_data.py balanced
"""
import sys

import tensorflow_datasets as tfds

split = sys.argv[1] if len(sys.argv) > 1 else "balanced"
builder = tfds.builder(f"emnist/{split}")
builder.download_and_prepare()
print(f"emnist/{split} ready in {builder.data_dir}")
for name, info in builder.info.splits.items():
    print(f"  {name}: {info.num_examples} images")
