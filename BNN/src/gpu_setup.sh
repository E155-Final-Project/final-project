#!/bin/bash
# GPU training env in WSL: Python 3.12 via uv (no sudo), venv at ~/bnn-gpu.
# Same pinned packages as the Windows .venv, plus TensorFlow's CUDA wheels.
set -e
BNN="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$HOME/.local/bin:$PATH"
command -v uv >/dev/null || curl -LsSf https://astral.sh/uv/install.sh | sh
uv python install 3.12
[ -d ~/bnn-gpu ] || uv venv --python 3.12 ~/bnn-gpu
uv pip install --python ~/bnn-gpu/bin/python -r "$BNN/requirements.txt" "tensorflow[and-cuda]==2.21.0"
source "$BNN/src/gpu_env.sh"
~/bnn-gpu/bin/python "$BNN/src/gpu_check.py"
