#include "rfid_uart.h"

extern UART_HandleTypeDef huart1;

static uint8_t  rx_start = 0;
static uint8_t  rx_len = 0;
static uint8_t  rx_idx = 0;
static uint8_t  rx_buf[32];

static volatile uint8_t card_flag = CARD_FLAG_NONE;

static uint8_t  chinese_data[CHINESE_DATA_SIZE];
static int8_t   chinese_block_num = 0;

/* --- Protocol helpers --- */

static uint8_t CalcChecksum(const uint8_t *dat, uint8_t num)
{
    uint8_t b = 0;
    for (uint8_t i = 0; i < num; i++) b ^= dat[i];
    return b;
}

static void UartSendFrame(const uint8_t *payload, uint8_t cnt)
{
    uint8_t frame[40];
    uint8_t fi = 0;
    frame[fi++] = 0x7F;
    for (uint8_t i = 0; i < cnt; i++) {
        frame[fi++] = payload[i];
        if (payload[i] == 0x7F) {
            frame[fi++] = 0x7F;
        }
    }
    HAL_UART_Transmit(&huart1, frame, fi, HAL_MAX_DELAY);
}

static void SetBaud115200(void)
{
    uint8_t payload[] = {0x05, 0x00, 0xAC, 0x00, 0x01, 0xC2, 0x00};
    payload[6] = CalcChecksum(payload, 6);
    UartSendFrame(payload, 7);
    HAL_Delay(50);

    huart1.Init.BaudRate = 115200;
    HAL_UART_Init(&huart1);
}

/* --- Public API --- */

void RFID_UART_Init(void)
{
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);

    SetBaud115200();

    card_flag = CARD_FLAG_NONE;
    chinese_block_num = 0;

    uint8_t dummy;
    HAL_UART_Receive_IT(&huart1, &dummy, 1);
}

void RFID_UART_RxCallback(uint8_t byte)
{
    if (rx_start == 0) {
        if (byte == 0x7F) {
            rx_start = 1;
            rx_len = 0;
            rx_idx = 0;
        }
        return;
    }

    if (rx_len == 0) {
        if (byte < 0x7F) {
            rx_len = byte;
        }
        return;
    }

    if (rx_len > 1) {
        rx_len--;
        rx_buf[rx_idx++] = byte;
    } else {
        rx_len = 0;
        rx_start = 0;

        uint8_t ok = (rx_buf[rx_idx] == CalcChecksum(rx_buf, rx_idx));
        rx_idx = 0;

        if (!ok) return;

        uint8_t code = rx_buf[1];
        uint8_t status = rx_buf[2];

        if (code == 0x90) {
            if (status == 0x00) {
                if (card_flag != CARD_FLAG_LEDLIGHT) {
                    card_flag = CARD_FLAG_EXIST;
                }
            } else {
                if (card_flag != CARD_FLAG_LEDLIGHT) {
                    card_flag = CARD_FLAG_NONE;
                }
            }
        } else if (code == 0x91) {
            if (status == 0x00) {
                card_flag = CARD_FLAG_RESDATA;
            } else {
                card_flag = CARD_FLAG_NONE;
            }
        } else {
            if (card_flag != CARD_FLAG_LEDLIGHT) {
                card_flag = CARD_FLAG_NONE;
            }
        }
    }
}

uint8_t RFID_GetCardFlag(void)  { return card_flag; }
void    RFID_SetCardFlag(uint8_t flag) { card_flag = flag; }

void RFID_GetBlockData(uint8_t *buf)
{
    for (uint8_t i = 0; i < 16; i++) {
        buf[i] = rx_buf[9 + i];
    }
}

void RFID_SendReadCard(void)
{
    uint8_t payload[] = {0x03, 0x00, 0x10, 0x00};
    payload[3] = CalcChecksum(payload, 3);
    UartSendFrame(payload, 4);
}

void RFID_SendReadBlock(uint8_t block)
{
    uint8_t payload[] = {0x04, 0x00, 0x11, block, 0x00};
    payload[4] = CalcChecksum(payload, 4);
    UartSendFrame(payload, 5);
}

uint8_t RFID_GetChineseBlockNum(void) { return (uint8_t)chinese_block_num; }

void RFID_SetChineseBlockNum(int8_t n) { chinese_block_num = n; }

uint8_t *RFID_GetChineseData(void) { return chinese_data; }

void RFID_AppendChineseData(const uint8_t *src, uint8_t len)
{
    uint8_t pos = (chinese_block_num > 0) ? (uint8_t)(chinese_block_num * 16) : 0;
    if (pos + len > CHINESE_DATA_SIZE) len = CHINESE_DATA_SIZE - pos;
    for (uint8_t i = 0; i < len; i++) {
        chinese_data[pos + i] = src[i];
    }
}

void RFID_ClearChineseData(void)
{
    for (uint8_t i = 0; i < CHINESE_DATA_SIZE; i++) {
        chinese_data[i] = 0;
    }
}
