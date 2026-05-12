#ifndef __RC522_H
#define __RC522_H

#include <stdint.h>

#define RC522_SPI         SPI1
#define RC522_NSS_PIN     GPIO_PIN_4
#define RC522_NSS_PORT    GPIOA
#define RC522_RST_PIN     GPIO_PIN_8
#define RC522_RST_PORT    GPIOA

void RC522_Init(void);
void RC522_HardReset(void);
uint8_t RC522_CheckCard(uint32_t *uid_out);

#endif
