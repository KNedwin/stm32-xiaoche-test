#ifndef __RFID_UART_H
#define __RFID_UART_H

#include "stm32f1xx_hal.h"

#define CARD_FLAG_NONE     0
#define CARD_FLAG_EXIST    1
#define CARD_FLAG_WAIT     2
#define CARD_FLAG_RESDATA  3
#define CARD_FLAG_LEDLIGHT 4

#define CHINESE_DATA_SIZE  60

void    RFID_UART_Init(void);
void    RFID_UART_RxCallback(uint8_t byte);
uint8_t RFID_GetCardFlag(void);
void    RFID_SetCardFlag(uint8_t flag);
void    RFID_GetBlockData(uint8_t *buf);
void    RFID_SendReadCard(void);
void    RFID_SendReadBlock(uint8_t block);
uint8_t RFID_GetChineseBlockNum(void);
void    RFID_SetChineseBlockNum(int8_t n);
uint8_t *RFID_GetChineseData(void);
void    RFID_AppendChineseData(const uint8_t *src, uint8_t len);
void    RFID_ClearChineseData(void);

#endif
