# ADC/PWM Homework Implementation Guide
## Texas Instruments F280049C LaunchPad

This guide explains how to implement the homework tasks using register-level programming for the F280049C microcontroller.

---

## System Overview

The implementation uses:
- **ePWM1** to generate a 100 kHz PWM signal and trigger ADC conversions
- **EPWM1 SOCA** triggers **ADC SOC3** every PWM cycle
- **ADCIN3** (channel A3) is sampled with 12-bit resolution
- **EPWM1_ISR** interrupt reads ADC result and updates PWM duty cycle
- **EPWM1A** output (GPIO0) reflects the adjusted duty cycle

### Key Specifications
- **PWM Frequency**: 100 kHz (10 μs period, TBPRD = 999)
- **ADC Resolution**: 12-bit (0-4095 counts)
- **ADC Reference**: External 3.3V (configurable to internal)
- **ADC Input Channel**: ADCIN3 (A3)
- **ADC Sample Rate**: 100 kHz (triggered by EPWM1 SOCA)
- **ADC Clock**: 50 MHz (SYSCLK/2)
- **Sample Window**: 360 ns (ACQPS = 35)

---

## Task 1: ADC/PWM Triggering and Measurement

### Objective
Use EPWM1 SOCA to trigger ADC SOC3 to sample ADCIN3 pin. In the EPWM1 interrupt subroutine, record max/min values and compare internal vs external reference voltage.

### Implementation Steps

#### 1. Configure ePWM1 Time Base (`Init_ePWM()`)

```c
void Init_ePWM(void)
{
    EALLOW;
    // Enable CPU clock to ePWM
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;

    // TBCLK = SYSCLK = 100 MHz -> T_TBCLK = 10 ns
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;

    // Free run mode (for debug)
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 2;

    // Counter mode: Up-count
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;

    // Clear counter
    EPwm1Regs.TBCTR = 0;

    // Period setting: PRD = 999 -> (999+1)*10 ns = 10 us -> 100 kHz
    EPwm1Regs.TBPRD = 999;

    // Use Shadow mode to update TBPRD
    EPwm1Regs.TBCTL.bit.PRDLD = 0;
    EPwm1Regs.TBCTL2.bit.PRDLDSYNC = 0;  // Load when TBCTR = 0
    EDIS;
}
```

#### 2. Configure ePWM1 Compare Registers (`Init_ePWM_CC()`)

```c
void Init_ePWM_CC(void)
{
    EALLOW;
    // Set CMPA to half period -> 50% duty cycle initially
    EPwm1Regs.CMPA.bit.CMPA = 0.5 * (EPwm1Regs.TBPRD);

    // CMPB same as CMPA
    EPwm1Regs.CMPB.bit.CMPB = EPwm1Regs.CMPA.bit.CMPA;

    // Use shadow mode for updates
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    // Shadow load condition: Load when TBCTR = 0
    EPwm1Regs.CMPCTL.bit.LOADASYNC = 0;
    EPwm1Regs.CMPCTL.bit.LOADBSYNC = 0;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = 0;
    EDIS;
}
```

#### 3. Configure ePWM1 Action Qualifier (`Init_ePWM_AQ()`)

```c
void Init_ePWM_AQ(void)
{
    EALLOW;
    // ZRO -> set EPWM1A high
    EPwm1Regs.AQCTLA.bit.ZRO = 2;  // set EPWM1A high
    // CAU -> clear EPWM1A low
    EPwm1Regs.AQCTLA.bit.CAU = 1;  // clear EPWM1A low
    EPwm1Regs.AQCTLA.bit.CBU = 0;
    EPwm1Regs.AQCTLA.bit.PRD = 0;

    // Software force reload condition
    EPwm1Regs.AQSFRC.bit.RLDCSF = 0;

    // No software force output
    EPwm1Regs.AQCSFRC.bit.CSFA = 0;
    EPwm1Regs.AQCSFRC.bit.CSFB = 0;
    EDIS;
}
```

#### 4. Configure ePWM1 Event Trigger and Dead-Band (`Init_ePWM_DB_ET()`)

```c
void Init_ePWM_DB_ET(void)
{
    EALLOW;
    // ------ DB submodule ------
    // No dead-band
    EPwm1Regs.DBCTL.bit.OUT_MODE = 0;

    // ------ ET submodule ------
    // Interrupt event condition: INTSEL = 1 -> Trigger when TBCTR = 0
    EPwm1Regs.ETSEL.bit.INTSEL = 1;
    // Enable EPWM1 interrupt
    EPwm1Regs.ETSEL.bit.INTEN = 1;
    // Clear interrupt flag once
    EPwm1Regs.ETCLR.bit.INT = 1;
    // Trigger once per event -> 10 us once -> 100 kHz
    EPwm1Regs.ETPS.bit.INTPSEL = 0;
    EPwm1Regs.ETPS.bit.INTPRD = 1;

    // SOCA event condition: TBCTR = 0
    EPwm1Regs.ETSEL.bit.SOCASEL = 1;
    // Enable EPWM1 SOCA
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;
    // One SOCA per event -> 10 us once
    EPwm1Regs.ETPS.bit.SOCPSSEL = 0;
    EPwm1Regs.ETPS.bit.SOCAPRD = 1;
    EDIS;
}
```

#### 5. Configure ADC Module (`Init_ADC()`)

```c
void Init_ADC(void)
{
    // Set ADCA to use external 3.3V reference
    // For internal reference, change to:
    // SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
    SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);

    EALLOW;
    // Enable ADCA clock
    CpuSysRegs.PCLKCR13.bit.ADC_A = 1;

    // ADCCLK = SYSCLK / 2 = 50 MHz
    AdcaRegs.ADCCTL2.bit.PRESCALE = 2;

    // Generate EOC pulse when conversion completes (for interrupt)
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    // Enable ADCA module
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    // ADC needs time to wake up
    DELAY_US(1000);
    EDIS;
}
```

#### 6. Configure ADC Start-of-Conversion (`Init_ADCSOC()`)

```c
void Init_ADCSOC(void)
{
    EALLOW;
    // Set SOC0~SOC15 to high priority
    AdcaRegs.ADCSOCPRICTL.bit.SOCPRIORITY = 0x10;

    // Use ADCA SOC3
    // Trigger source: 5 -> EPWM1 SOCA
    AdcaRegs.ADCSOC3CTL.bit.TRIGSEL = 5;
    // Sample source: ADCIN3 (A3)
    AdcaRegs.ADCSOC3CTL.bit.CHSEL = 3;
    // Sample time: ACQPS = 35 -> (35+1)*10 ns = 360 ns
    AdcaRegs.ADCSOC3CTL.bit.ACQPS = 35;

    // Disable ADCAINT1 interrupt, use ePWM interrupt instead
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 0;
    EDIS;
}
```

#### 7. Configure PIE Interrupt Controller (`Init_PIE()`)

```c
void Init_PIE(void)
{
    DINT;  // Disable CPU global interrupt

    // Enable PIE
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;

    // Set PIE control to default
    InitPieCtrl();

    // Clear CPU interrupt flags
    IER = 0x0000;
    IFR = 0x0000;

    // Build PIE vector table
    InitPieVectTable();

    // Enable PIE group 3, INTx1 (EPWM1 interrupt)
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;

    // Enable CPU INT3
    IER |= M_INT3;

    EINT;  // Enable CPU global interrupt
    ERTM;  // Enable real-time interrupt
}
```

#### 8. Global Variables for Max/Min Tracking

Add these global variables to track min/max values:

```c
// Global variables for Task 1
uint16_t adcResultMin = 0xFFFF;  // Start with maximum value
uint16_t adcResultMax = 0x0000;  // Start with minimum value
```

#### 9. EPWM1 Interrupt Service Routine

```c
interrupt void EPWM1_ISR(void)
{
    unsigned long long c = 0;  // Declare to avoid multiplication overflow

    EALLOW;
    // Read ADCA result (SOC3 -> ADCRESULT3)
    c = AdcaResultRegs.ADCRESULT3;

    // Track minimum value (for Task 1)
    if (c < adcResultMin)
    {
        adcResultMin = (uint16_t)c;
    }

    // Track maximum value (for Task 1)
    if (c > adcResultMax)
    {
        adcResultMax = (uint16_t)c;
    }

    // Adjust PWM duty based on ADC value (for Task 3)
    // ADC is 12-bit -> divide by 2^12 (=4096) -> use shift >> 12
    EPwm1Regs.CMPA.bit.CMPA = (c * EPwm1Regs.TBPRD) >> 12;

    // Clear ePWM1 interrupt flag
    EPwm1Regs.ETCLR.bit.INT = 1;
    // Clear PIE group3 ACK
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    EDIS;
}
```

#### 10. Register Interrupt Handler in main()

The interrupt handler must be registered in the PIE vector table. Add this to your main function or initialization:

```c
// Register EPWM1 interrupt handler
EALLOW;
PieVectTable.EPWM1_INT = &EPWM1_ISR;
EDIS;
```

### Comparison Notes for Task 1
- **Internal Reference**: Factory calibrated, typically 3.3V
- **External Reference**: Uses VREFHI pin, more stable but requires external source
- To test internal reference, change `Init_ADC()`:
  ```c
  SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
  ```
- Compare `adcResultMax` and `adcResultMin` values between the two configurations
- External reference usually provides better accuracy and stability

---

## Task 2: Low-Pass Filter Impact

### Objective
Use the low-pass filter circuit space on the development board to observe the impact of a filter capacitor on ADC conversion.

### Hardware Setup

1. **Locate DNP Capacitor Footprints**: 
   - On the F280049C LaunchPad, find capacitor footprints C26-C31 (330pF each)
   - These are marked as "DNP" (Do Not Populate) on the board

2. **Install Filter Capacitor**:
   - For ADCIN3, locate the corresponding capacitor footprint (check schematic)
   - Install a 330pF capacitor between the ADC input pin and GND
   - The capacitor connects through a 0-ohm resistor (R14, R16, R17, R18, R20, or R21)

3. **Signal Source**:
   - Apply a signal source with varying frequency to ADCIN3
   - Use a function generator or potentiometer with noise

### Code Changes

**No code changes required!** The filter affects the hardware signal, so the same code works for both cases.

### Testing Procedure

1. **Without Filter**:
   - Measure ADC results with oscilloscope
   - Apply a square wave with fast edges
   - Observe ADC results may show noise/ripple

2. **With Filter**:
   - Install 330pF capacitor
   - Apply the same signal
   - Observe ADC results should be smoother
   - Response may be slower to fast changes

### Observations

- **Without filter**: ADC may show noise/ripple from high-frequency components
- **With filter**: ADC results should be smoother, but may have slower response to fast changes
- **330pF capacitor**: Creates a low-pass filter with RC time constant based on source impedance
- **Cutoff frequency**: Depends on source impedance and capacitor value
- **Trade-off**: Better noise rejection vs. slower response time

---

## Task 3: PWM Duty Cycle Control from ADC

### Objective
Use ADCRESULT3 to adjust EPWM1 duty cycle. When input is 0V → duty cycle = 0%, when input is 3.3V → duty cycle = 100%.

### Implementation

The implementation is already included in the `EPWM1_ISR()` function shown in Task 1. The key calculation is:

```c
// Linear mapping: 0V (ADC=0) -> 0% duty, 3.3V (ADC=4095) -> 100% duty
// Duty cycle = (ADC_result / 4095) * 100%
// Compare value = (ADC_result * TBPRD) / 4095
EPwm1Regs.CMPA.bit.CMPA = (c * EPwm1Regs.TBPRD) >> 12;
```

### Duty Cycle Calculation

The PWM duty cycle is linearly proportional to the ADC input voltage:

```
Duty Cycle = (ADC_value / 4095) × 100%
CMPA = (ADC_value × TBPRD) >> 12
```

**Mapping:**
- **0V input** (ADC = 0) → **0% duty cycle** (CMPA = 0)
- **1.65V input** (ADC = 2048) → **50% duty cycle** (CMPA = 500)
- **3.3V input** (ADC = 4095) → **100% duty cycle** (CMPA = 999)

### Verification Steps

1. **Input 0V**: 
   - ADC ≈ 0 → CMPA = 0 → Duty cycle = 0%
   - Verify with oscilloscope on GPIO0 (EPWM1A)

2. **Input 1.65V**: 
   - ADC ≈ 2048 → CMPA ≈ 500 → Duty cycle ≈ 50%
   - Verify with oscilloscope

3. **Input 3.3V**: 
   - ADC ≈ 4095 → CMPA = 999 → Duty cycle = 100%
   - Verify with oscilloscope

### Important Notes

- **Shadow Registers**: CMPA uses shadow mode, so updates occur at TBCTR = 0 (glitch-free)
- **Update Rate**: Duty cycle updates every 10 μs (100 kHz)
- **Resolution**: 12-bit ADC provides 4096 steps for duty cycle control
- **Linear Mapping**: The relationship is perfectly linear across the full range

---

## Complete Implementation Structure

### File: `main.c`

```c
#include "driverlib.h"
#include "device.h"

// External function declarations (from int.c)
extern void Init_PIE(void);
extern void Init_ePWM(void);
extern void Init_ePWM_CC(void);
extern void Init_ePWM_AQ(void);
extern void Init_ePWM_DB_ET(void);
extern void Init_ADC(void);
extern void Init_ADCSOC(void);

// External interrupt handler (from int.c)
extern interrupt void EPWM1_ISR(void);

void main(void)
{
    InitSysCtrl();   // System clock
    Init_GPIO();     // GPIO initialization (must be provided)
    Init_PIE();
    Init_ePWM();
    Init_ePWM_CC();
    Init_ePWM_AQ();
    Init_ePWM_DB_ET();
    Init_ADC();
    Init_ADCSOC();

    // Register EPWM1 interrupt handler
    EALLOW;
    PieVectTable.EPWM1_INT = &EPWM1_ISR;
    EDIS;

    // Configure GPIO0 as EPWM1A output
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // 1 -> EPWM1A
    GpioCtrlRegs.GPACSEL1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0;
    EDIS;

    while(1)
    {
        // Main loop - all processing in interrupt
    }
}
```

### File: `int.c`

Contains all the initialization functions and interrupt service routine as shown in the task sections above.

---

## Configuration Options

### Changing ADC Input Channel

To use a different ADC channel, modify `Init_ADCSOC()`:

```c
// Change CHSEL to your desired channel (0-15)
AdcaRegs.ADCSOC3CTL.bit.CHSEL = 3;  // Change 3 to your channel number
```

### Changing PWM Frequency

To change PWM frequency, modify `Init_ePWM()`:

```c
// For 10 kHz: TBPRD = 9999 (10,000 counts × 10ns = 100μs)
EPwm1Regs.TBPRD = 9999;

// For 1 kHz: TBPRD = 99999 (100,000 counts × 10ns = 1ms)
EPwm1Regs.TBPRD = 99999;
```

**Formula:** `TBPRD = (SYSCLK / desired_frequency) - 1`

### Changing ADC Sample Window

To adjust ADC acquisition time, modify `Init_ADCSOC()`:

```c
// ACQPS = (desired_time_ns / 10ns) - 1
// For 500ns: ACQPS = 49
AdcaRegs.ADCSOC3CTL.bit.ACQPS = 35;  // Current: 360ns
```

### Changing Reference Voltage

To switch between internal and external reference, modify `Init_ADC()`:

```c
// For internal reference:
SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);

// For external reference:
SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);
```

---

## Testing Checklist

- [ ] Task 1: EPWM1 triggers ADC SOC3, max/min values recorded
- [ ] Task 1: Internal reference tested
- [ ] Task 1: External reference tested
- [ ] Task 1: Results compared between reference modes
- [ ] Task 2: Filter capacitor installed (330pF)
- [ ] Task 2: Impact of filter observed (with/without capacitor)
- [ ] Task 3: Duty cycle scales from 0% to 100% with ADC input
- [ ] Task 3: Linear mapping verified (0V→0%, 3.3V→100%)
- [ ] Task 3: PWM output verified on GPIO0 with oscilloscope

---

## Important Notes

1. **Pin Configuration**: 
   - ADC input: **ADCIN3** (check board schematic for exact pin number)
   - PWM output: **GPIO0** configured as **EPWM1A**

2. **Reference Voltage**: 
   - Ensure external reference is properly connected if using external mode
   - Internal reference is factory calibrated

3. **Interrupt Timing**: 
   - ISR executes every 10 μs, so keep ISR code short
   - All processing must complete before next interrupt

4. **Shadow Registers**: 
   - CMPA uses shadow mode for glitch-free PWM updates
   - Shadow registers load when TBCTR = 0

5. **ADC Resolution**: 
   - F280049C has 12-bit ADC (0-4095 counts)
   - Full scale corresponds to reference voltage (3.3V)

6. **Register Access**: 
   - Always use `EALLOW`/`EDIS` when modifying protected registers
   - Interrupt acknowledgment: Must clear both ePWM and PIE interrupt flags

---

## Troubleshooting

1. **No PWM Output**:
   - Verify GPIO0 is configured correctly
   - Check ePWM clock is enabled (`CpuSysRegs.PCLKCR2.bit.EPWM1`)
   - Verify time base is running (check `EPwm1Regs.TBCTR` in debugger)

2. **ADC Not Converting**:
   - Verify EPWM1 is generating SOCA events
   - Check ADC clock is enabled (`CpuSysRegs.PCLKCR13.bit.ADC_A`)
   - Verify SOC3 configuration matches trigger source (TRIGSEL = 5)

3. **Duty Cycle Not Updating**:
   - Check interrupt is firing (set breakpoint in `EPWM1_ISR()`)
   - Verify shadow mode is configured correctly
   - Check ADC result register is updating (`AdcaResultRegs.ADCRESULT3`)

4. **Wrong ADC Values**:
   - Verify reference voltage configuration
   - Check input voltage is within 0-3.3V range
   - Verify correct ADC channel is selected (CHSEL = 3)

5. **Interrupt Not Firing**:
   - Verify PIE is enabled and configured
   - Check interrupt handler is registered in vector table
   - Verify interrupt flags are being cleared properly

---

## Additional Resources

- **C2000Ware SDK**: `device_support/f28004x/`
- **ADC Examples**: `device_support/f28004x/examples/adc/`
- **EPWM Examples**: `device_support/f28004x/examples/epwm/`
- **Device Datasheet**: F280049C Technical Reference Manual (SPRUHX5)
- **Register Reference**: F28004x Peripheral Reference Guide

---

Good luck with your homework!
