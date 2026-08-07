#ifndef TEENSY_ATTRIBUTES_H
#define TEENSY_ATTRIBUTES_H

#define DMAMEM __attribute__((section(".dmabuffers"), used))
#define FASTRUN __attribute__((section(".fastrun")))
#define PROGMEM __attribute__((section(".progmem")))
#define FLASHMEM __attribute__((section(".flashmem")))

#endif
