#ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#include "cmsis_os.h"

typedef enum {
    SYSTEM_MODE_RUN  = 0,
    SYSTEM_MODE_INIT = 1,
} SystemMode_t;

typedef struct {
    SystemMode_t mode;
    uint8_t      motor_direction;
    uint16_t     motor_speed;
} SystemState_t;

extern SystemState_t system_state;
extern osMutexId state_mutex_handle;

void SystemState_Init(void);
void SystemState_SetMode(SystemMode_t mode);
SystemMode_t SystemState_GetMode(void);
void SystemState_SetMotorDirection(uint8_t direction);
uint8_t SystemState_GetMotorDirection(void);
void SystemState_SetMotorSpeed(uint16_t speed);
uint16_t SystemState_GetMotorSpeed(void);

#endif
