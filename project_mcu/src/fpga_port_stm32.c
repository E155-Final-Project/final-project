#include "fpga_port.h"
#include "pins.h"
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_SPI.h"
#include "STM32L432KC_SYSTICK.h"

void fpga_port_cs(int active)
{
    digitalWrite(PIN_SPI_CS, active ? PIO_LOW : PIO_HIGH);   /* CS is active low */
}

uint8_t fpga_port_xfer(uint8_t out)
{
    return (uint8_t)spiSendReceive((char)out);   /* CHECK: your send/receive function */
}

uint32_t fpga_port_millis(void)
{
    return millis();
}
