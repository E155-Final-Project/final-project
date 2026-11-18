CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -Isrc -Itest
BUILD   = build

HOST_SRCS = src/bitmap_print.c src/classes.c src/rasterize.c \
			test/mem_io.c test/strokes.c test/golden.c test/host_rasterize.c
HOST_BIN = $(BUILD)/host_rasterize

LIVE_SRCS = src/live_view.c test/strokes.c test/host_live.c

UI_SRCS = src/ui.c test/test_ui.c

WEIGHTS_SRCS = src/weights.c test/mem_io.c test/test_weights.c

LINK_SRCS = src/fpga_link.c src/classes.c  src/rasterize.c  src/weights.c \
			test/fpga_port_fake.c test/golden.c test/mem_io.c test/strokes.c \
			test/test_fpga_link.c

STROKE_SRCS = src/stroke.c src/rasterize.c test/strokes.c test/test_stroke.c



ui: $(UI_SRCS) $(wildcard scr/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(UI_SRCS) -o $(BUILD)/test_ui
	./$(BUILD)/test_ui

live: $(LIVE_SRCS) $(wildcard src/*.h test/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(LIVE_SRCS) -o $(BUILD)/host_live
	./$(BUILD)/host_live

weights: $(WEIGHTS_SRCS) src/weights.h | $(BUILD)
	$(CC) $(CFLAGS) $(WEIGHTS_SRCS) -o $(BUILD)/test_weights
	./$(BUILD)/test_weights

link: $(LINK_SRCS) $(wildcard src/*.h test/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(LINK_SRCS) -o $(BUILD)/test_fpga_link
	./$(BUILD)/test_fpga_link

stroke: $(STROKE_SRCS) $(wildcard src/*.h test/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(STROKE_SRCS) -o $(BUILD)/test_stroke
	./$(BUILD)/test_stroke

.PHONY: host test clean live ui weights link stroke

host: $(HOST_BIN)

$(HOST_BIN): $(HOST_SRCS) $(wildcard src/*.h test/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(HOST_SRCS) -o $@

$(BUILD):
	mkdir -p $@

test: host
	./$(HOST_BIN)

clean:
	rm -rf $(BUILD)