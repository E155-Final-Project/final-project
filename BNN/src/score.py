"""
Score RTL simulation predictions against the golden model and the true labels.

The testbench writes one predicted class per line (decimal, `$fdisplay(fd, "%0d", pred)`).
Exits non-zero if any prediction differs from the golden model, since the RTL
is supposed to be bit-exact.

Usage:
  python score.py --pred ../build/sim/full/predictions.txt --vec ../vectors/full \
      --report ../results/rtl_full.txt
"""
import argparse
import os
import sys

import numpy as np


def read_hex(path):
    with open(path) as f:
        return np.array([int(line, 16) for line in f if line.strip()])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--pred", required=True)
    p.add_argument("--vec", required=True)
    p.add_argument("--report", default=None)
    a = p.parse_args()

    labels = read_hex(os.path.join(a.vec, "labels.mem"))
    golden = read_hex(os.path.join(a.vec, "expected.mem"))
    with open(a.pred) as f:
        pred = np.array([int(line) for line in f if line.strip()])

    n = len(pred)
    if n != len(golden):
        print(f"warning: {n} predictions for {len(golden)} vectors, scoring the first {n}")
    labels, golden = labels[:n], golden[:n]
    bad = np.flatnonzero(pred != golden)

    report = (f"vectors: {a.vec}  images: {n}\n"
              f"rtl accuracy:    {np.mean(pred == labels):.4%}\n"
              f"golden accuracy: {np.mean(golden == labels):.4%}\n"
              f"rtl vs golden:   {n - len(bad)}/{n} match\n")
    if len(bad):
        report += "first mismatches (image: rtl golden label): " + ", ".join(
            f"{i}: {pred[i]} {golden[i]} {labels[i]}" for i in bad[:10]) + "\n"
    print(report, end="")
    if a.report:
        os.makedirs(os.path.dirname(a.report) or ".", exist_ok=True)
        with open(a.report, "w") as f:
            f.write(report)
    sys.exit(1 if len(bad) else 0)


if __name__ == "__main__":
    main()
