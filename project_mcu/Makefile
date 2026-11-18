CC      = gcc
INC     = -Isrc/app -Isrc/capture -Isrc/raster -Isrc/fpga -Isrc/ui -Isrc/testdata -Itest/support
CFLAGS  = -std=c11 -Wall -Wextra -O2 $(INC)
BUILD   = build

# Every header in src/ and test/: any change rebuilds the tests
HEADERS = $(wildcard src/*/*.h test/support/*.h)

# Shared pieces
SUPPORT_MEM    = test/support/mem_io.c
SUPPORT_GOLDEN = test/support/golden.c
SUPPORT_STROKE = test/support/strokes.c

RASTER_SRCS  = src/ui/bitmap_print.c src/ui/classes.c src/raster/rasterize.c \
               $(SUPPORT_MEM) $(SUPPORT_STROKE) $(SUPPORT_GOLDEN) test/host_rasterize.c
LIVE_SRCS    = src/ui/live_view.c $(SUPPORT_STROKE) test/host_live.c
UI_SRCS      = src/ui/ui.c test/test_ui.c
WEIGHTS_SRCS = src/fpga/weights.c $(SUPPORT_MEM) test/test_weights.c
LINK_SRCS    = src/fpga/fpga_link.c src/ui/classes.c src/raster/rasterize.c src/fpga/weights.c \
               test/support/fpga_port_fake.c $(SUPPORT_GOLDEN) $(SUPPORT_MEM) $(SUPPORT_STROKE) \
               test/test_fpga_link.c
STROKE_SRCS  = src/capture/stroke.c src/raster/rasterize.c $(SUPPORT_STROKE) test/test_stroke.c

DEMO_SRCS    = src/ui/live_view.c src/ui/classes.c src/raster/rasterize.c \
               $(SUPPORT_GOLDEN) $(SUPPORT_MEM) $(SUPPORT_STROKE) test/demo_live.c


.PHONY: all test weights link stroke ui live clean demo

# make all: every pass/fail test (the live demo is separate)
all: test weights link stroke ui

test: $(BUILD)/host_rasterize
	./$<
weights: $(BUILD)/test_weights
	./$<
link: $(BUILD)/test_fpga_link
	./$<
stroke: $(BUILD)/test_stroke
	./$<
ui: $(BUILD)/test_ui
	./$<
live: $(BUILD)/host_live
	./$<
demo: $(BUILD)/demo_live
	./$<

$(BUILD)/demo_live:      $(DEMO_SRCS)    $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(DEMO_SRCS) -o $@
$(BUILD)/host_rasterize: $(RASTER_SRCS)  $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(RASTER_SRCS) -o $@
$(BUILD)/test_weights:   $(WEIGHTS_SRCS) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(WEIGHTS_SRCS) -o $@
$(BUILD)/test_fpga_link: $(LINK_SRCS)    $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(LINK_SRCS) -o $@
$(BUILD)/test_stroke:    $(STROKE_SRCS)  $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(STROKE_SRCS) -o $@
$(BUILD)/test_ui:        $(UI_SRCS)      $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(UI_SRCS) -o $@
$(BUILD)/host_live:      $(LIVE_SRCS)    $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(LIVE_SRCS) -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
