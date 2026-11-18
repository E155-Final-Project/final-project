// fpga_port_stm32.c
// fpga_link hardware hooks on the NUCLEO-L432KC, using the lib/ drivers.
// Board build only; the laptop tests use test/fpga_port_fake.c instead.

#include "fpga_port.h"
#include "pins.h"
#include "STM32L432KC.h"          // CMSIS: CoreDebug, DWT
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_SPI.h"
#include "STM32L432KC_SYSTICK.h"              // CHECK: wherever millis() is declared

#define CPU_MHZ 80u               // SYSCLK after the RCC/PLL setup

void fpga_port_init(void)
{
    gpioEnable(GPIO_PORT_A);
    pinMode(PIN_FPGA_LOAD, GPIO_OUTPUT);
    pinMode(PIN_FPGA_LOAD_WEIGHTS, GPIO_OUTPUT);
    pinMode(PIN_FPGA_CLEAR, GPIO_OUTPUT);
    digitalWrite(PIN_FPGA_LOAD, PIO_LOW);
    digitalWrite(PIN_FPGA_LOAD_WEIGHTS, PIO_LOW);
    digitalWrite(PIN_FPGA_CLEAR, PIO_LOW);

    // DWT cycle counter for microsecond delays (Cortex-M4 core peripheral).
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void fpga_port_load(int high)         { digitalWrite(PIN_FPGA_LOAD, high ? PIO_HIGH : PIO_LOW); }
void fpga_port_load_weights(int high) { digitalWrite(PIN_FPGA_LOAD_WEIGHTS, high ? PIO_HIGH : PIO_LOW); }
void fpga_port_clear(int high)        { digitalWrite(PIN_FPGA_CLEAR, high ? PIO_HIGH : PIO_LOW); }

uint8_t fpga_port_xfer(uint8_t out)
{
    return spiSendReceive(out);
}

uint32_t fpga_port_millis(void)
{
    return millis();
}

void fpga_port_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * CPU_MHZ;
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
    }
}