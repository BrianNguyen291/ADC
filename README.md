# ADC/PWM Homework - F280049C 100-pin Implementation

## Overview

This project implements an ADC-to-PWM control system on the F280049C microcontroller. The system continuously samples an analog input voltage and adjusts the PWM duty cycle proportionally. The implementation uses register-level programming for direct hardware control.

## System Architecture

### Signal Flow
1. **ePWM1** generates a 100kHz PWM signal (10μs period)
2. **EPWM1 SOCA** triggers ADC conversion every PWM cycle
3. **ADC** samples **ADCIN3** (channel A3) with 12-bit resolution
4. **EPWM1_ISR** interrupt reads ADC result and updates PWM duty cycle
5. **EPWM1A** output (GPIO0) reflects the adjusted duty cycle

### Key Specifications
- **PWM Frequency**: 100 kHz (10 μs period)
- **PWM Period**: 1000 counts (TBPRD = 999)
- **ADC Resolution**: 12-bit (0-4095 counts)
- **ADC Reference**: Configurable (Internal or External 3.3V)
- **ADC Input Channel**: ADCIN3 (A3)
- **ADC Sample Rate**: 100 kHz (triggered by EPWM1 SOCA)
- **ADC Clock**: 50 MHz (SYSCLK/2)
- **Sample Window**: 360 ns (ACQPS = 35)

## Files

- `main.c` - Main program with initialization sequence and GPIO configuration
- `int.c` - Interrupt service routines and hardware initialization functions
- `HOMEWORK_GUIDE.md` - Detailed implementation guide and explanations
- `TODO.md` - Implementation checklist and testing guide

## Hardware Setup

### F280049C 100-pin LaunchPad

1. **ADC Input Pin**: Connect your signal source to **ADCIN3 (A3)**
   - Check your board schematic for the exact pin number
   - Input voltage range: 0V to 3.3V

2. **Reference Voltage**:
   - Configurable via `#define USE_INTERNAL_REF` in `int.c`
   - **External reference** (default): `#define USE_INTERNAL_REF 0`
   - **Internal reference**: Change to `#define USE_INTERNAL_REF 1`
   - Rebuild project after changing the define
   - Used for Task 1 comparison between internal and external reference

3. **EPWM Output**: 
   - **GPIO0** is configured as **EPWM1A** output
   - Connect oscilloscope or load to GPIO0 to observe PWM signal

4. **Low-Pass Filter** (Optional for Task 2):
   - Install 330pF capacitor between ADC input and GND
   - Connect through 0-ohm resistor if needed

## Code Structure

### Initialization Sequence (`main.c`)

The initialization follows this order:
1. `InitSysCtrl()` - System clock configuration
2. `Init_GPIO()` - GPIO initialization (must be provided separately)
3. `Init_PIE()` - PIE interrupt controller setup
4. **Interrupt handler registration** - Register `EPWM1_ISR` in PIE vector table
5. `Init_ePWM()` - ePWM1 time base configuration
6. `Init_ePWM_CC()` - Compare register configuration
7. `Init_ePWM_AQ()` - Action qualifier configuration
8. `Init_ePWM_DB_ET()` - Dead-band and event trigger setup
9. `Init_ADC()` - ADC module initialization
10. `Init_ADCSOC()` - ADC start-of-conversion configuration
11. GPIO0 configuration as EPWM1A output

### Key Functions (`int.c`)

#### `Init_ePWM()`
- Enables ePWM1 clock
- Sets time base clock to 100 MHz (SYSCLK)
- Configures up-count mode
- Sets period to 999 (1000 counts = 10 μs = 100 kHz)
- Enables shadow mode for period register

#### `Init_ePWM_CC()`
- Sets initial compare value to 50% duty cycle
- Configures shadow mode for compare registers
- Loads shadow registers when TBCTR = 0

#### `Init_ePWM_AQ()`
- Configures EPWM1A output:
  - **Set** when TBCTR = 0 (ZRO event)
  - **Clear** when TBCTR = CMPA (CAU event)
- This creates a standard PWM waveform

#### `Init_ePWM_DB_ET()`
- Disables dead-band (not needed for single output)
- Configures interrupt trigger at TBCTR = 0 (every PWM cycle)
- Configures SOCA trigger at TBCTR = 0 (triggers ADC)

#### `Init_ADC()`
- Sets external 3.3V reference
- Configures ADC clock to 50 MHz
- Enables ADC module
- Waits 1ms for ADC power-up

#### `Init_ADCSOC()`
- Configures SOC3:
  - Trigger source: EPWM1 SOCA (trigger value = 5)
  - Input channel: ADCIN3 (CHSEL = 3)
  - Sample window: 360 ns (ACQPS = 35)
- Disables ADC interrupt (uses ePWM interrupt instead)

#### `Init_PIE()`
- Initializes PIE controller
- Enables EPWM1 interrupt (Group 3, INTx1)
- Enables CPU interrupt INT3

#### `EPWM1_ISR()`
- Reads ADC result from `ADCRESULT3`
- **Task 1**: Tracks maximum and minimum ADC values in global variables
- **Task 3**: Calculates new compare value: `CMPA = (ADC_value × TBPRD) >> 12`
- Updates CMPA register (shadow mode, loads at next TBCTR = 0)
- Clears interrupt flags

#### Global Variables (`int.c`)
- `adcResultMin` - Minimum ADC value recorded (Task 1)
- `adcResultMax` - Maximum ADC value recorded (Task 1)
- `USE_INTERNAL_REF` - Define to switch between internal/external reference (0 = external, 1 = internal)

## Duty Cycle Calculation

The PWM duty cycle is linearly proportional to the ADC input voltage:

```
Duty Cycle = (ADC_value / 4095) × 100%
CMPA = (ADC_value × TBPRD) >> 12
```

**Mapping:**
- **0V input** (ADC = 0) → **0% duty cycle** (CMPA = 0)
- **1.65V input** (ADC = 2048) → **50% duty cycle** (CMPA = 500)
- **3.3V input** (ADC = 4095) → **100% duty cycle** (CMPA = 999)

## Configuration Options

### Changing ADC Input Channel

To use a different ADC channel, modify `Init_ADCSOC()` in `int.c`:

```c
// Change CHSEL to your desired channel (0-15)
AdcaRegs.ADCSOC3CTL.bit.CHSEL = 3;  // Change 3 to your channel number
```

### Changing PWM Frequency

To change PWM frequency, modify `Init_ePWM()` in `int.c`:

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

To switch between internal and external reference, modify the define in `int.c`:

```c
// At top of int.c (line 15):
#define USE_INTERNAL_REF 0  // 0 = External, 1 = Internal

// Then rebuild the project
```

## Task Implementation Status

### Task 1: EPWM SOC Triggers ADC SOC and Max/Min Tracking
- ✅ EPWM1 SOCA triggers ADC SOC3 every PWM cycle (100 kHz)
- ✅ ADC samples ADCIN3 channel
- ✅ **Max/min tracking implemented** - Global variables `adcResultMin` and `adcResultMax` track values
- ✅ **Reference voltage comparison** - Configurable via `USE_INTERNAL_REF` define
- ✅ Interrupt handler registered in PIE vector table
- 📝 **Testing**: Use debugger to monitor max/min values, test with both reference modes

### Task 2: Low-Pass Filter Impact
- ✅ Hardware setup required (install 330pF capacitor)
- ✅ Code remains unchanged, filter affects hardware response
- 📝 Compare ADC results with/without filter using oscilloscope or debugger
- 📝 Observe smoother ADC readings with filter installed

### Task 3: ADC Result Controls PWM Duty Cycle
- ✅ Fully implemented in `EPWM1_ISR()`
- ✅ Linear mapping: 0V → 0%, 3.3V → 100%
- ✅ Updates every PWM cycle (100 kHz update rate)
- ✅ Shadow register mode prevents glitches during duty cycle updates

## Usage

1. **Prerequisites**:
   - Code Composer Studio (CCS) or compatible IDE
   - C2000Ware SDK with F280049C support
   - F280049C LaunchPad board

2. **Build Steps**:
   - Include `driverlib.h` and `device.h` from C2000Ware
   - Ensure `Init_GPIO()` function is defined (or remove if not needed)
   - Compile project
   - Flash to F280049C LaunchPad

3. **Testing**:
   - Connect signal source (0-3.3V) to ADCIN3
   - Connect oscilloscope to GPIO0 (EPWM1A)
   - Observe PWM duty cycle changing with input voltage
   - Use debugger to monitor:
     - `AdcaResultRegs.ADCRESULT3` - Current ADC reading
     - `EPwm1Regs.CMPA.bit.CMPA` - Current PWM duty cycle
     - `adcResultMin` - Minimum ADC value (Task 1)
     - `adcResultMax` - Maximum ADC value (Task 1)

## Debugging Tips

1. **No PWM Output**:
   - Verify GPIO0 is configured correctly
   - Check ePWM clock is enabled (`CpuSysRegs.PCLKCR2.bit.EPWM1`)
   - Verify time base is running (check `EPwm1Regs.TBCTR` in debugger)

2. **ADC Not Converting**:
   - Verify EPWM1 is generating SOCA events
   - Check ADC clock is enabled (`CpuSysRegs.PCLKCR13.bit.ADC_A`)
   - Verify SOC3 configuration matches trigger source

3. **Duty Cycle Not Updating**:
   - Check interrupt is firing (set breakpoint in `EPWM1_ISR()`)
   - Verify shadow mode is configured correctly
   - Check ADC result register is updating

4. **Wrong ADC Values**:
   - Verify reference voltage configuration (`USE_INTERNAL_REF` define)
   - Check input voltage is within 0-3.3V range
   - Verify correct ADC channel is selected (CHSEL = 3)

5. **Max/Min Values Not Updating**:
   - Verify interrupt is firing (set breakpoint in `EPWM1_ISR()`)
   - Check that global variables `adcResultMin` and `adcResultMax` are accessible
   - Ensure interrupt handler is registered in `main.c`

## Important Notes

- **Shadow Registers**: Compare values update at TBCTR = 0 to prevent glitches
- **Interrupt Timing**: ISR executes every 10 μs, so keep ISR code short
- **ADC Latency**: ADC conversion completes before next interrupt (360ns << 10μs)
- **Register Access**: Always use `EALLOW`/`EDIS` when modifying protected registers
- **Interrupt Acknowledgment**: Must clear both ePWM and PIE interrupt flags
- **Max/Min Tracking**: Values are updated in ISR, accessible as global variables for debugging
- **Reference Voltage**: Change `USE_INTERNAL_REF` define and rebuild to test different reference modes
- **Interrupt Registration**: Interrupt handler must be registered in PIE vector table before enabling interrupts

## References

- F280049C Technical Reference Manual (SPRUHX5)
- C2000Ware SDK Documentation
- `HOMEWORK_GUIDE.md` for detailed explanations and theory

## License

This code is provided for educational purposes as part of a homework assignment.
