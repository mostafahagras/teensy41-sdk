#include <teensy/gpio.h>
#include <teensy/i2c.h>
#include <teensy/time.h>
#include <teensy/pwm.h>
#include <teensy/uart.h>

int main(void)
{
    gpio_init();
    pwm_init();
    pwm_set_frequency(13, 1000.0f);
    pwm_write(13, 128);
    uart_init(uart6, 115200);
    uart_write(uart6, "teensy41 uart ready\r\n", 20);
    i2c_init(i2c1, 100000);

    for (;;) {
        __asm volatile("wfi");
    }
}
