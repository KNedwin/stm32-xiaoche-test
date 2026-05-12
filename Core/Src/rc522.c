#include "rc522.h"
#include "stm32f1xx_hal.h"

/* RC522 register addresses */
#define REG_COMMAND      0x01
#define REG_COMIEN       0x02
#define REG_DIVIEN       0x03
#define REG_COMIRQ       0x04
#define REG_DIVIRQ       0x05
#define REG_ERROR        0x06
#define REG_FIFODATA     0x09
#define REG_FIFOLEVEL    0x0A
#define REG_CONTROL      0x0C
#define REG_BITFRAMING   0x0D
#define REG_COLL         0x0E
#define REG_MODE         0x11
#define REG_TXMODE       0x12
#define REG_RXMODE       0x13
#define REG_TXCONTROL    0x14
#define REG_TXAUTO       0x15
#define REG_TMODE        0x2A
#define REG_TPRESCALER   0x2B
#define REG_TRELOADH     0x2C
#define REG_TRELOADL     0x2D
#define REG_VERSION      0x37

/* Commands */
#define PCD_IDLE         0x00
#define PCD_CALCCRC      0x03
#define PCD_TRANSCEIVE   0x0C
#define PCD_SOFTRESET    0x0F

/* PICC commands */
#define PICC_REQIDL      0x26
#define PICC_ANTICOLL    0x93
#define PICC_SELECTTAG   0x93

extern SPI_HandleTypeDef hspi1;

static void RC522_WriteReg(uint8_t addr, uint8_t val)
{
    uint8_t buf[2];
    buf[0] = (addr << 1) & 0x7E;
    buf[1] = val;
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_SET);
}

static uint8_t RC522_ReadReg(uint8_t addr)
{
    uint8_t tx[2], rx[2];
    tx[0] = ((addr << 1) & 0x7E) | 0x80;
    tx[1] = 0x00;
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_SET);
    return rx[1];
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t val = RC522_ReadReg(reg);
    RC522_WriteReg(reg, val | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t val = RC522_ReadReg(reg);
    RC522_WriteReg(reg, val & ~mask);
}

static void RC522_FlushFIFO(void)
{
    RC522_WriteReg(REG_FIFOLEVEL, 0x80);
}

static uint8_t RC522_ToCard(uint8_t cmd, uint8_t *send, uint8_t send_len,
                            uint8_t *recv, uint8_t *recv_len)
{
    uint8_t irq = 0x00;
    uint8_t n;

    if (cmd == PCD_TRANSCEIVE) {
        irq = 0x30;
    }

    RC522_WriteReg(REG_COMIEN, 0x7F & ~0x20);
    RC522_WriteReg(REG_COMIRQ, 0x7F);
    RC522_FlushFIFO();
    RC522_SetBitMask(REG_FIFOLEVEL, 0x80);

    RC522_WriteReg(REG_COMMAND, PCD_IDLE);
    for (uint8_t i = 0; i < send_len; i++) {
        RC522_WriteReg(REG_FIFODATA, send[i]);
    }
    RC522_WriteReg(REG_COMMAND, cmd);
    if (cmd == PCD_TRANSCEIVE) {
        RC522_SetBitMask(REG_BITFRAMING, 0x80);
    }

    uint16_t timeout = 2000;
    do {
        n = RC522_ReadReg(REG_COMIRQ);
        timeout--;
    } while ((timeout > 0) && !(n & irq) && !(n & 0x01));

    RC522_ClearBitMask(REG_BITFRAMING, 0x80);

    if (timeout == 0 || (RC522_ReadReg(REG_ERROR) & 0x1B)) {
        return 0;
    }

    if (n & irq) {
        uint8_t status = RC522_ReadReg(REG_ERROR);
        if (status & 0x1B) return 0;
    }

    if (n & 0x01) return 0;

    uint8_t fifo_bytes = RC522_ReadReg(REG_FIFOLEVEL);
    if (*recv_len < fifo_bytes) fifo_bytes = *recv_len;
    for (uint8_t i = 0; i < fifo_bytes; i++) {
        recv[i] = RC522_ReadReg(REG_FIFODATA);
    }
    *recv_len = fifo_bytes;
    return 1;
}

void RC522_Init(void)
{
    RC522_HardReset();

    RC522_WriteReg(REG_TMODE, 0x8D);
    RC522_WriteReg(REG_TPRESCALER, 0x3E);
    RC522_WriteReg(REG_TRELOADL, 30);
    RC522_WriteReg(REG_TRELOADH, 0);

    RC522_WriteReg(REG_TXAUTO, 0x40);
    RC522_WriteReg(REG_MODE, 0x3D);

    RC522_ClearBitMask(REG_TXCONTROL, 0x03);

    RC522_WriteReg(REG_RXMODE, 0x00);
    RC522_WriteReg(REG_TXMODE, 0x00);
}

void RC522_HardReset(void)
{
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    RC522_WriteReg(REG_COMMAND, PCD_SOFTRESET);
    HAL_Delay(10);

    while (RC522_ReadReg(REG_COMMAND) & 0x10) { }
}

uint8_t RC522_CheckCard(uint32_t *uid_out)
{
    uint8_t recv[16];
    uint8_t recv_len;
    uint8_t send[7];

    /* REQA */
    send[0] = PICC_REQIDL;
    send[1] = 0x00;
    recv_len = 2;
    RC522_ClearBitMask(REG_COLL, 0x80);
    if (!RC522_ToCard(PCD_TRANSCEIVE, send, 1, recv, &recv_len)) {
        return 0;
    }
    if (recv_len != 2) return 0;

    /* ANTICOLL */
    send[0] = PICC_ANTICOLL;
    send[1] = 0x20;
    recv_len = 8;
    RC522_ClearBitMask(REG_COLL, 0x80);
    if (!RC522_ToCard(PCD_TRANSCEIVE, send, 2, recv, &recv_len)) {
        return 0;
    }
    if (recv_len != 5) return 0;

    /* Verify BCC */
    uint8_t bcc = recv[0] ^ recv[1] ^ recv[2] ^ recv[3];
    if (bcc != recv[4]) return 0;

    *uid_out = ((uint32_t)recv[0] << 24)
             | ((uint32_t)recv[1] << 16)
             | ((uint32_t)recv[2] << 8)
             |  (uint32_t)recv[3];

    /* SELECT */
    send[0] = PICC_SELECTTAG;
    send[1] = 0x70;
    for (uint8_t i = 0; i < 5; i++) send[i + 2] = recv[i];
    uint8_t crc_send[9];
    for (uint8_t i = 0; i < 7; i++) crc_send[i] = send[i];
    recv_len = 8;
    RC522_ClearBitMask(REG_COLL, 0x80);
    RC522_ToCard(PCD_TRANSCEIVE, crc_send, 7, recv, &recv_len);

    return 1;
}
