//
// int.c - Interrupt Service Routines for ADC/PWM Homework
// F280049C 100-pin device
//

#include "driverlib.h"
#include "device.h"

// Global variables for Task 1: Max/Min tracking
uint16_t adcResultMin = 0xFFFF;  // Start with maximum value
uint16_t adcResultMax = 0x0000;  // Start with minimum value

// Reference voltage selection: 0 = External, 1 = Internal
// Change this to test different reference voltages for Task 1
#define USE_INTERNAL_REF 0

void Init_ePWM(void)
{
    EALLOW;
    // 啟用 CPU 給 ePWM 的 clock
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM1    = 1;

    // TBCLK = SYSCLK = 100 MHz -> T_TBCLK = 10 ns
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm1Regs.TBCTL.bit.CLKDIV    = 0;

    // 自由運行，debug 不要卡住
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 2;

    // 計數模式：上數
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;

    // 計數器清 0
    EPwm1Regs.TBCTR = 0;

    // 週期設定：PRD = 999 → (999+1)*10 ns = 10 us → 100 kHz
    EPwm1Regs.TBPRD = 999;

    // 用 Shadow mode 去更新 TBPRD
    EPwm1Regs.TBCTL.bit.PRDLD   = 0;
    EPwm1Regs.TBCTL2.bit.PRDLDSYNC = 0;   // TBCTR = 0 才 load

    EDIS;
}

void Init_ePWM_CC(void)
{
    EALLOW;
    // CMPA 設成一半的週期 → duty = 50%
    EPwm1Regs.CMPA.bit.CMPA = 0.5 * (EPwm1Regs.TBPRD);

    // CMPB 跟 CMPA 一樣
    EPwm1Regs.CMPB.bit.CMPB = EPwm1Regs.CMPA.bit.CMPA;

    // 用 shadow mode 更新
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    // shadow 的觸發條件：TBCTR = 0 時才 load
    EPwm1Regs.CMPCTL.bit.LOADASYNC = 0;
    EPwm1Regs.CMPCTL.bit.LOADBSYNC = 0;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = 0;
    EDIS;
}

void Init_ePWM_AQ(void)
{
    EALLOW;
    // 下數模式註解裡是說：TBCTR=0 時 set，高點(CMPA) 時 clear
    // 這裡實作的是：ZRO → set，CAU → clear
    EPwm1Regs.AQCTLA.bit.ZRO = 2;  // set EPWM1A high
    EPwm1Regs.AQCTLA.bit.CAU = 1;  // clear EPWM1A low
    EPwm1Regs.AQCTLA.bit.CBU = 0;
    EPwm1Regs.AQCTLA.bit.PRD = 0;

    // 軟體強制更新的 reload 條件
    EPwm1Regs.AQSFRC.bit.RLDCSF = 0;

    // 不用軟體強制輸出
    EPwm1Regs.AQCSFRC.bit.CSFA = 0;
    EPwm1Regs.AQCSFRC.bit.CSFB = 0;
    EDIS;
}

void Init_ePWM_DB_ET(void)
{
    EALLOW;
    // ------ DB submodule ------
    // 不用 dead-band
    EPwm1Regs.DBCTL.bit.OUT_MODE = 0;

    // ------ ET submodule ------
    // 中斷事件條件：INTSEL = 1 → TBCTR=0 時觸發
    EPwm1Regs.ETSEL.bit.INTSEL = 1;
    // 開 EPWM1 中斷
    EPwm1Regs.ETSEL.bit.INTEN  = 1;
    // 先清一次中斷旗標
    EPwm1Regs.ETCLR.bit.INT    = 1;
    // 每 1 次事件觸發一次 → 10 us 一次 → 100 kHz
    EPwm1Regs.ETPS.bit.INTPSEL = 0;
    EPwm1Regs.ETPS.bit.INTPRD  = 1;

    // SOCA 事件條件：TBCTR=0
    EPwm1Regs.ETSEL.bit.SOCASEL = 1;
    // 開 EPWM1 SOCA
    EPwm1Regs.ETSEL.bit.SOCAEN  = 1;
    // 每 1 次事件出一個 SOCA → 10 us 一次
    EPwm1Regs.ETPS.bit.SOCPSSEL = 0;
    EPwm1Regs.ETPS.bit.SOCAPRD  = 1;

    EDIS;
}

void Init_ADC(void)
{
    // 設定 ADCA 參考電壓 (Task 1: 可切換內部/外部參考)
    #if USE_INTERNAL_REF
        SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
    #else
        SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);
    #endif

    EALLOW;
    // 開 ADCA clock
    CpuSysRegs.PCLKCR13.bit.ADC_A = 1;

    // ADCCLK = SYSCLK / 2 = 50 MHz
    AdcaRegs.ADCCTL2.bit.PRESCALE = 2;

    // 轉換完成時產生 EOC pulse (給中斷用)
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    // 開啟 ADCA 模組
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    // ADC 醒來要一點時間
    DELAY_US(1000);
    EDIS;
}

void Init_ADCSOC(void)
{
    EALLOW;
    // 把 SOC0~SOC15 都設成 high priority
    AdcaRegs.ADCSOCPRICTL.bit.SOCPRIORITY = 0x10;

    // 用 ADCA 的 SOC3
    // 觸發來源：5 → EPWM1 SOCA
    AdcaRegs.ADCSOC3CTL.bit.TRIGSEL = 5;
    // 採樣來源：ADCIN3 (A3)
    AdcaRegs.ADCSOC3CTL.bit.CHSEL   = 3;
    // 採樣時間：ACQPS = 35 → (35+1)*10 ns = 360 ns
    AdcaRegs.ADCSOC3CTL.bit.ACQPS   = 35;

    // 這裡關掉 ADCAINT1 中斷，因為用 ePWM 的中斷
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 0;
    EDIS;
}

void Init_PIE(void)
{
    DINT;  // 關 CPU 全域中斷

    // 開啟 PIE
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;

    // 把 PIE control 設成預設值
    InitPieCtrl();

    // 清 CPU 中斷旗標
    IER = 0x0000;
    IFR = 0x0000;

    // 建立 PIE 向量表
    InitPieVectTable();

    // 開 PIE group 3, INTx1 (EPWM1 中斷通常掛這裡)
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;

    // 開 CPU 的 INT3
    IER |= M_INT3;

    EINT;  // 開啟 CPU 全域中斷
    ERTM;  // 開啟即時中斷
}

interrupt void EPWM1_ISR(void)
{
    unsigned long long c = 0;  // 先宣告避免乘法溢位

    EALLOW;
    // 讀 ADCA 結果 (SOC3 → ADCRESULT3)
    c = AdcaResultRegs.ADCRESULT3;

    // Task 1: 記錄最大/最小值
    if (c < adcResultMin)
    {
        adcResultMin = (uint16_t)c;
    }
    if (c > adcResultMax)
    {
        adcResultMax = (uint16_t)c;
    }

    // Task 3: 依 ADC 值調整 PWM duty
    // ADC 是 12-bit → 除以 2^12 (=4096) → 用位移 >> 12
    EPwm1Regs.CMPA.bit.CMPA = (c * EPwm1Regs.TBPRD) >> 12;

    // 清 ePWM1 中斷旗標
    EPwm1Regs.ETCLR.bit.INT = 1;
    // 清 PIE group3 的 ACK
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    EDIS;
}