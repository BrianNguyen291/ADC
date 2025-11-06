//
// main.c - ADC/PWM Homework Implementation for F280049C 100-pin
//
// Tasks:
// 1. EPWM SOC triggers ADC SOC, record max/min values
// 2. Low-pass filter impact observation
// 3. ADC result controls EPWM duty cycle (0V->0%, 3.3V->100%)
// 4. Temperature sensor conversion (optional)
//

#include "driverlib.h"
#include "device.h"

// Forward declaration of interrupt handler
extern interrupt void EPWM1_ISR(void);

void main(void)
{
    InitSysCtrl();   // 系統時脈
    Init_GPIO();     // 你專案裡原本的 GPIO init
    Init_PIE();
    
    // 註冊 EPWM1 中斷處理函數
    EALLOW;
    PieVectTable.EPWM1_INT = &EPWM1_ISR;
    EDIS;
    
    Init_ePWM();
    Init_ePWM_CC();
    Init_ePWM_AQ();
    Init_ePWM_DB_ET();
    Init_ADC();
    Init_ADCSOC();

    // 把 GPIO0 設成 EPWM1A 輸出
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO0   = 1;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0  = 1;   // 1 → EPWM1A
    GpioCtrlRegs.GPACSEL1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO0   = 0;
    EDIS;

    while(1)
    {
        // main loop
    }
}