#include <teensy/gpio.h>
#include <teensy/pwm.h>

int main(void)
{
    gpio_init();
    pwm_init();
    pwm_set_frequency(13, 1000.0f);
    pwm_write(13, 128);

    for (;;) {
        __asm volatile("wfi");
    }
}
