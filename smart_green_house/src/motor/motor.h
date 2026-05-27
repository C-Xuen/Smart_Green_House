#ifndef MOTOR_H
#define MOTOR_H

void motor_init(void);
void fan_on(void);
void fan_off(void);
void pump_on(void);
void pump_off(void);
int  fan_is_manual(void);
int  pump_is_manual(void);
void fan_set_manual(int v);
void pump_set_manual(int v);

#endif
