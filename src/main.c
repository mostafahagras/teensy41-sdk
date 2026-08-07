#include <teensy/gpio.h>

int main(void)
{
    gpio_init();
    gpio_configure(13, GPIO_OUTPUT);
    gpio_write(13, true);

    for (;;) {
        __asm volatile("wfi");
    }
}
