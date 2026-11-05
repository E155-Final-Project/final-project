# BNN for EMNIST on the iCE40UP5K: data -> train -> golden vectors -> RTL sim -> accuracy.
#
#   src/       Python: model + training, vector generation, sim scoring
#   data/      EMNIST via TensorFlow Datasets (gitignored, ~1 GB)
#   models/    trained checkpoints, one per config
#   mem/       weight/threshold ROM images + bnn_params.svh (FPGA build and testbench)
#   vectors/   testbench vectors: smoke/ traces every layer, full/ is the whole test split
#   results/   golden-model and RTL accuracy reports
#   build/     simulator scratch (gitignored)
#
# The RTL lives in ../project_fpga/src and the testbench in ../project_fpga/sim.
#
# Typical flow:
#   make venv data          # once
#   make train              # ~60 epochs on CPU; writes models/
#   make train GPU=1        # same on the GPU, through WSL2 (make gpu-setup once first)
#   make vectors            # mem/ + vectors/ + results/golden_*.txt
#   make sim VEC=smoke      # debug the datapath layer by layer
#   make sim                # whole test split -> results/rtl_full_*.txt

# SHIFT: max random-shift augmentation (px).  VAL: fraction of train held out to
# pick the best epoch.  SMOKE: images in the traced smoke set.  FULL: images in the
# full set (0 = whole test split).  VEC: vector set to simulate, smoke | full.
# GPU: 1 trains in the WSL2 environment, since TensorFlow has no GPU support on
# native Windows.  SEED: random seed for training (default unseeded).
SPLIT  ?= balanced
HIDDEN ?= 512,512,512
EPOCHS ?= 60
SHIFT  ?= 1
LR     ?= 3e-2
VAL    ?= 0.1
SMOKE  ?= 16
FULL   ?= 0
VEC    ?= full
GPU    ?= 0
SEED   ?=

ifeq ($(OS),Windows_NT)
  PY_BOOT ?= py -3.12
  PY      := .venv/Scripts/python.exe
  WSL     := MSYS_NO_PATHCONV=1 wsl.exe -e
else
  PY_BOOT ?= python3.12
  PY      := .venv/bin/python
  WSL     :=
endif

GPU_PY := $(WSL) bash src/gpu_run.sh
ifeq ($(GPU),1)
  TRAIN_PY  := $(GPU_PY)
  TRAIN_DEP :=
else
  TRAIN_PY  := $(PY)
  TRAIN_DEP := $(VENV)
endif

# tfds reads and downloads here instead of ~/tensorflow_datasets
export TFDS_DATA_DIR := $(abspath data)

comma := ,
TAG   := $(SPLIT)_h$(subst $(comma),-,$(HIDDEN))
VENV  := .venv/.installed
CKPT  := models/fpga_$(TAG).weights.h5
# Rewritten on every generation run, so it stands in for all of mem/ + vectors/
GEN   := mem/bnn_params.svh

# --- Simulation (Questa) --------------------------------------------------
RTL_DIR    ?= ../project_fpga/src
TB_DIR     ?= ../project_fpga/sim
TB         ?= tb_bnn
RTL_SRCS   ?= $(filter-out $(RTL_DIR)/tb_%,$(wildcard $(RTL_DIR)/*.sv))
TB_SRCS    ?= $(TB_DIR)/$(TB).sv
# QUESTA_BIN: e.g. C:/lscc/radiant/<ver>/questasim/win64/ if vsim is not on PATH
# VSIM_FLAGS: e.g. -L iCE40UP for the Lattice SB_SPRAM256KA simulation model
QUESTA_BIN ?=
VSIM_FLAGS ?=
SIM_DIR    := build/sim/$(VEC)
PRED       := $(SIM_DIR)/predictions.txt

.DEFAULT_GOAL := help
.DELETE_ON_ERROR:
.PHONY: help venv gpu-setup gpu-check data train vectors sim accuracy clean clean-gen clean-data

help:  ## list targets
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  %-12s %s\n", $$1, $$2}'
	@echo "  config: SPLIT=$(SPLIT) HIDDEN=$(HIDDEN) VEC=$(VEC)  ->  $(CKPT)"

# --- Python environment ---------------------------------------------------
venv: $(VENV)  ## create .venv and install requirements.txt

$(VENV): requirements.txt
	test -x $(PY) || $(PY_BOOT) -m venv .venv
	$(PY) -m pip install -q -r requirements.txt
	touch $@

gpu-setup:  ## create the WSL GPU environment at ~/bnn-gpu and test it
	$(WSL) bash src/gpu_setup.sh

gpu-check:  ## fail unless TensorFlow really runs on the GPU
	$(GPU_PY) src/gpu_check.py

# --- Training data --------------------------------------------------------
data: data/.emnist_$(SPLIT)  ## download EMNIST into data/ (train/vectors also fetch it on demand)

data/.emnist_$(SPLIT): | $(VENV)
	$(PY) src/fetch_data.py $(SPLIT)
	touch $@

# --- Training -------------------------------------------------------------
TRAIN = $(TRAIN_PY) src/bnn_emnist.py --split $(SPLIT) --hidden $(HIDDEN) --epochs $(EPOCHS) \
        --lr $(LR) --shift $(SHIFT) --val-frac $(VAL) --ckpt $(CKPT) \
        $(if $(SEED),--seed $(SEED))

train: | $(TRAIN_DEP)  ## (re)train the MLP, keeping the best epoch in models/ (GPU=1 for GPU)
	$(TRAIN)

$(CKPT): | $(TRAIN_DEP)
	$(TRAIN)

# --- Golden model: ROM images + testbench vectors -------------------------
vectors: $(GEN)  ## write mem/, vectors/{smoke,full}/ and results/golden_*.txt

$(GEN): $(CKPT) src/gen_vectors.py src/bnn_emnist.py | $(VENV)
	$(PY) src/gen_vectors.py --ckpt $(CKPT) --split $(SPLIT) --hidden $(HIDDEN) \
	    --mem-dir mem --vec-dir vectors --smoke $(SMOKE) --full $(FULL) \
	    --report results/golden_$(TAG).txt

# --- RTL simulation -------------------------------------------------------
# The testbench reads $readmemh files from +MEM_DIR / +VEC_DIR, includes
# bnn_params.svh and vectors.svh, and writes one "%0d" prediction per line to
# +PRED_FILE. Any "** Error" or "** Fatal" in the log fails the run.
sim: $(PRED) | $(VENV)  ## simulate vector set VEC (smoke|full) in Questa and score it
	$(PY) src/score.py --pred $(PRED) --vec vectors/$(VEC) --report results/rtl_$(VEC)_$(TAG).txt

$(PRED): $(RTL_SRCS) $(wildcard $(TB_SRCS)) $(GEN)
	@test -f $(TB_SRCS) || { echo "no testbench at $(TB_SRCS) (set TB_DIR / TB)"; exit 1; }
	rm -rf $(SIM_DIR) && mkdir -p $(SIM_DIR)
	cd $(SIM_DIR) && $(QUESTA_BIN)vlib work && $(QUESTA_BIN)vlog -sv \
	    +incdir+$(abspath mem) +incdir+$(abspath vectors/$(VEC)) \
	    $(abspath $(RTL_SRCS) $(TB_SRCS))
	cd $(SIM_DIR) && $(QUESTA_BIN)vsim -c $(VSIM_FLAGS) $(TB) -l sim.log -do "run -all; quit -f" \
	    +MEM_DIR=$(abspath mem) +VEC_DIR=$(abspath vectors/$(VEC)) +PRED_FILE=predictions.txt
	! grep -E '\*\* (Error|Fatal)' $(SIM_DIR)/sim.log

# --- Reports --------------------------------------------------------------
accuracy:  ## print golden and RTL accuracy for the current config
	@for f in results/golden_$(TAG).txt results/rtl_*_$(TAG).txt; do \
	    test -f $$f && { echo "== $$f"; cat $$f; }; done; true

# --- Cleanup --------------------------------------------------------------
clean:  ## remove simulator scratch (build/)
	rm -rf build

clean-gen: clean  ## also remove generated mem/ and vectors/
	rm -rf mem vectors

clean-data:  ## remove the downloaded dataset
	rm -rf data
