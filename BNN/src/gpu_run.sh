#!/bin/bash
# Run a Python script from BNN/ in the WSL GPU environment (make gpu-setup creates it).
cd "$(dirname "$0")/.."
if [ ! -x ~/bnn-gpu/bin/python ]; then
    echo "no GPU environment at ~/bnn-gpu: run 'make gpu-setup' first" >&2
    exit 1
fi
source src/gpu_env.sh
export TFDS_DATA_DIR="$PWD/data"
exec ~/bnn-gpu/bin/python "$@"
