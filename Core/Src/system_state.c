#include "system_state.h"

SystemState_t system_state = {
    .mode = SYSTEM_MODE_RUN,
    .motor_direction = 0,
    .motor_speed = 500,
};

osMutexId state_mutex_handle;
osMutexDef(state_mutex);

void SystemState_Init(void)
{
    state_mutex_handle = osMutexCreate(osMutex(state_mutex));
}

void SystemState_SetMode(SystemMode_t mode)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.mode = mode;
    osMutexRelease(state_mutex_handle);
}

SystemMode_t SystemState_GetMode(void)
{
    SystemMode_t m;
    osMutexWait(state_mutex_handle, osWaitForever);
    m = system_state.mode;
    osMutexRelease(state_mutex_handle);
    return m;
}

void SystemState_SetMotorDirection(uint8_t direction)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.motor_direction = direction;
    osMutexRelease(state_mutex_handle);
}

uint8_t SystemState_GetMotorDirection(void)
{
    uint8_t d;
    osMutexWait(state_mutex_handle, osWaitForever);
    d = system_state.motor_direction;
    osMutexRelease(state_mutex_handle);
    return d;
}

void SystemState_SetMotorSpeed(uint16_t speed)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.motor_speed = speed;
    osMutexRelease(state_mutex_handle);
}

uint16_t SystemState_GetMotorSpeed(void)
{
    uint16_t s;
    osMutexWait(state_mutex_handle, osWaitForever);
    s = system_state.motor_speed;
    osMutexRelease(state_mutex_handle);
    return s;
}
