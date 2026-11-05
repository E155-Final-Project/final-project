# BNN

Fully binary MLP for EMNIST balanced (47 classes), sized for the iCE40UP5K. The final network is 784→512→512→512→47, with all weights in SPRAM.

The saved model scores 84.49% on the test split. The mean over 3 seeds is 84.4%.

To use the weights and vectors, read the files in `mem/` and `vectors/` directly. They are already generated, so you don't need Python or TensorFlow.

## Weights: `mem/`

All files are hex, one value per line, and load with `$readmemh`.

| File | Contents | Lines |
|---|---|---|
| `l0_weights.mem` | Layer 0 weights, 64-bit words | 6,656 |
| `l1_weights.mem` | Layer 1 weights, 64-bit words | 4,096 |
| `l2_weights.mem` | Layer 2 weights, 64-bit words | 4,096 |
| `l3_weights.mem` | Output layer weights, 64-bit words | 376 |
| `l0_thresh.mem`, `l1_thresh.mem`, `l2_thresh.mem` | One 10-bit popcount threshold per neuron | 512 each |
| `bnn_params.svh` | Layer sizes and widths as `localparam`s | |

Each layer has its own file. There is no combined SPRAM image or flash boot image yet. Concatenated in layer order, the layers start at these word addresses:

| Layer | Inputs | Neurons | Words per neuron | Start word |
|---|---|---|---|---|
| 0 | 784 | 512 | 13 | 0 |
| 1 | 512 | 512 | 8 | 6,656 |
| 2 | 512 | 512 | 8 | 10,752 |
| 3 | 512 | 47 | 8 | 14,848 |

The total is 15,224 words.

## Bit layout

- Weights are neuron-major: all the words for neuron 0, then neuron 1, and so on.
- Bit `j` of word `k` is input `k*64 + j`.
- Layer 0 has 784 inputs in 13 words, so the top 48 bits of the last word are padding. Padding is 1 in weights and 0 in inputs, so XNOR gives 0 there and the popcount needs no mask.
- Activations pack the same way as inputs, so a hidden layer's output is the next layer's input buffer.

## Arithmetic

For each neuron, sum `popcount(XNOR(input, weight))` over its words.

- Hidden layers: the neuron fires when `popcount >= threshold`.
- Output layer: no threshold. The prediction is the argmax of the 47 popcounts.

**Ties:** the lowest class index wins, so the argmax compare must use strict `>`. 188 of the 18,800 test images have a tied top score, so `>=` will fail against `expected.mem`.

## Test vectors: `vectors/`

`vectors/smoke/` holds 16 images with every layer traced. Start here when debugging the datapath. `vectors/full/` holds all 18,800 test images, and its first 16 images are the smoke set.

| File | Contents |
|---|---|
| `inputs.mem` | Image bits, 13 words per image |
| `labels.mem` | True class per image |
| `expected.mem` | Golden prediction per image. The RTL must match this, not `labels.mem` |
| `vectors.svh` | `N_IMAGES` and `HAS_TRACE` |
| `l<i>_popcount.mem` (smoke only) | Popcount per neuron, one line each, image-major |
| `l<i>_act.mem` (smoke only) | Hidden layer activations, packed like `inputs.mem` |

Class indices are 0 to 9 for digits, 10 to 35 for A to Z, and 36 to 46 for a, b, d, e, f, g, h, n, q, r, t.

## Regenerating

`make vectors` rebuilds `mem/`, `vectors/`, and `results/golden_*.txt` from the checkpoint in `models/`. It needs the Python environment and the roughly 1 GB dataset.

Git doesn't preserve timestamps, so on a fresh clone `make vectors` or `make sim` may decide `mem/` is out of date and regenerate it. The output is identical, but it's slow the first time.

`make sim` runs the RTL against the vectors in Questa. It needs `tb_bnn.sv` in `../project_fpga/sim`, which doesn't exist yet. See the top of the `Makefile` for the full flow.

`src/train_best.py` is the experimental trainer used for the wider model search. The saved model came from `src/bnn_emnist.py`.
