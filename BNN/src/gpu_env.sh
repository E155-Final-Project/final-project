# Source in WSL before training: TF does not find the pip CUDA wheels on its own here.
SP=~/bnn-gpu/lib/python3.12/site-packages/nvidia
export LD_LIBRARY_PATH="$(ls -d $SP/*/lib | tr '\n' ':')/usr/lib/wsl/lib"
export PATH="$SP/cuda_nvcc/bin:$PATH"
export XLA_FLAGS="--xla_gpu_cuda_data_dir=$SP/cuda_nvcc"
