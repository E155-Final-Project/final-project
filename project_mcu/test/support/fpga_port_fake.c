#include "fpga_port_fake.h"
#include "fpga_port.h"
#include "fpga_link.h"
#include "golden.h"
#include "weights.h"

fake_mode_t fake_mode;
uint32_t fake_weight_words, fake_weight_mismatch, fake_image_words;
uint64_t fake_image[16];
int      fake_partial_words, fake_gap_violations, fake_runs, fake_clear_pulses;

/* ---- time (ns) ---- */
static uint64_t t_ns;
static uint64_t last_sck_ns, last_pin_ns;
static int      sck_since_pin;      /* an SCK edge happened since the last pin edge */

/* ---- pins ---- */
static int load, load_weights, clear_pin;
static uint64_t clear_rise_ns;

/* ---- BNN_spi registers ---- */
static uint8_t  bitcnt;             /* 6 bits */
static uint64_t shreg, word;
static int      wasdone, sdodelayed;
static uint8_t  resultcaptured;

/* ---- BNN_top / Memory / core ---- */
static uint32_t load_addr;
static int      done;
static uint8_t  class_out;
static int      core_running;
static uint64_t core_done_at;

#define SCK_HALF_NS  100u           /* 5 MHz */
#define EDGE_MIN_NS  1000u
#define INFER_NS     350000u        /* tb comment: about 0.35 ms */

static int core_reset(void) { return load || load_weights; }

static void advance(uint64_t ns)
{
    t_ns += ns;
    if (core_running && fake_mode != FAKE_NEVER_DONE && t_ns >= core_done_at) {
        core_running = 0;
        if (!done) {
            done = 1;
            fpga_link_done_isr();   /* EXTI0 rising edge */
        }
    }
}

void fake_reset(void)
{
    fake_mode = FAKE_NORMAL;
    fake_weight_words = fake_weight_mismatch = fake_image_words = 0;
    fake_partial_words = fake_gap_violations = fake_runs = fake_clear_pulses = 0;
    t_ns = 0;
    last_sck_ns = last_pin_ns = 0;
    sck_since_pin = 0;
    load = load_weights = clear_pin = 0;
    bitcnt = 0;
    shreg = word = 0;
    wasdone = sdodelayed = 0;
    resultcaptured = 0;
    load_addr = 0;
    done = 0;
    class_out = 0;
    core_running = 0;
}

void fpga_port_init(void)
{
}

/* A load or load_weights edge: must be EDGE_MIN_NS away from SCK activity. */
static void pin_edge(void)
{
    if (sck_since_pin && t_ns - last_sck_ns < EDGE_MIN_NS) fake_gap_violations++;
    last_pin_ns = t_ns;
    sck_since_pin = 0;
}

static void start_core(void)
{
    uint16_t scores[NUM_CLASSES];
    int cls = golden_infer(fake_image, scores);
    class_out = (fake_mode == FAKE_BAD_CLASS) ? 0x7F : (uint8_t)cls;
    core_running = 1;
    core_done_at = t_ns + INFER_NS;
    fake_runs++;
}

static void set_reset_edges(int was_reset)
{
    if (core_reset()) {             /* held in reset: done and the result clear */
        done = 0;
        core_running = 0;
    } else if (was_reset) {
        start_core();               /* both pins low again: core runs once */
    }
}

void fpga_port_load(int high)
{
    high = !!high;
    if (high == load) return;
    int was_reset = core_reset();
    pin_edge();
    if (!high && bitcnt != 0) fake_partial_words++;     /* ended mid-word */
    load = high;
    if (load) {                     /* load_start: address back to 0 */
        load_addr = 0;
        if (!load_weights) fake_image_words = 0;
    } else {
        bitcnt = 0;                 /* async reset on negedge load */
    }
    set_reset_edges(was_reset);
}

void fpga_port_load_weights(int high)
{
    high = !!high;
    if (high == load_weights) return;
    int was_reset = core_reset();
    pin_edge();
    load_weights = high;
    set_reset_edges(was_reset);
}

void fpga_port_clear(int high)
{
    high = !!high;
    if (high && !clear_pin) clear_rise_ns = t_ns;
    if (!high && clear_pin && t_ns - clear_rise_ns >= 10000u) fake_clear_pulses++;
    clear_pin = high;
}

static void new_word(uint64_t w)
{
    if (load_weights) {
        if (load_addr < BNN_WEIGHT_WORDS && w != bnn_weights[load_addr]) fake_weight_mismatch++;
        fake_weight_words++;
    } else {
        fake_image[load_addr & 15u] = w;
        fake_image_words++;
    }
    load_addr++;
}

/* One SCK cycle, Mode 0. Returns the MISO bit the master samples. */
static int sck_cycle(int sdi)
{
    if (sck_since_pin == 0 && last_pin_ns && t_ns - last_pin_ns < EDGE_MIN_NS) {
        fake_gap_violations++;      /* first SCK too soon after a pin edge */
    }
    advance(SCK_HALF_NS);

    /* sdo as BNN_spi drives it, sampled by the master on the rising edge */
    int sdo = (done && !wasdone) ? (int)(class_out & 1u) : sdodelayed;

    /* posedge sck */
    uint64_t next = (shreg >> 1) | ((uint64_t)(sdi & 1) << 63);
    if (load && bitcnt == 63) {
        word = next;
        new_word(word);
    }
    shreg = next;
    if (load) bitcnt = (uint8_t)((bitcnt + 1u) & 63u);
    if (!wasdone) resultcaptured = class_out;
    else          resultcaptured = (uint8_t)(resultcaptured >> 1);

    advance(SCK_HALF_NS);

    /* negedge sck */
    wasdone = done;
    sdodelayed = (resultcaptured >> 1) & 1u;

    last_sck_ns = t_ns;
    sck_since_pin = 1;
    return sdo;
}

uint8_t fpga_port_xfer(uint8_t out)
{
    uint8_t in = 0;
    for (int b = 0; b < 8; b++) {   /* LSB first, both directions */
        in |= (uint8_t)(sck_cycle((out >> b) & 1) << b);
    }
    return in;
}

uint32_t fpga_port_millis(void)
{
    advance(1000);                  /* each call lets 1 us pass, so busy-waits progress */
    return (uint32_t)(t_ns / 1000000u);
}

void fpga_port_delay_us(uint32_t us)
{
    advance((uint64_t)us * 1000u);
}