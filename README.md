# ADC/PWM Homework - F280049C 100-pin Implementation

## Files

- `main.c` - Main program with initialization and configuration
- `int.c` - Interrupt service routines for ADC
- `HOMEWORK_GUIDE.md` - Detailed implementation guide

## Hardware Setup

### F280049C 100-pin LaunchPad

1. **ADC Input Pin**: Connect your signal source to the ADC input pin
   - Default configuration uses `ADC_CH_ADCIN0`
   - Check your board schematic for the exact pin number
   - Common pins: ADCINA0, ADCINA1, etc.

2. **Reference Voltage** (for Task 1):
   - **Internal Reference**: No external connection needed
   - **External Reference**: Connect external reference to VREFHI pin

3. **Low-Pass Filter** (Task 2):
   - Install 330pF capacitor (C26-C31) between ADC input and GND
   - Connect through 0-ohm resistor (R14, R16, R17, etc.)

4. **EPWM Output**: EPWM1A output pin (check schematic for exact pin)

## Configuration

### Changing ADC Input Channel

In `main.c`, modify the `configureADC()` function:

```c
ADC_setupSOC(ADCA_BASE,
             ADC_SOC_NUMBER0,
             ADC_TRIGGER_EPWM1_SOCA,
             ADC_CH_ADCIN0,    // Change this to your pin (e.g., ADC_CH_ADCIN1)
             10);
```

### Testing Internal vs External Reference (Task 1)

In `main.c`, change the reference mode:

```c
// For internal reference:
ADC_ReferenceMode refMode = ADC_REFERENCE_INTERNAL;

// For external reference:
ADC_ReferenceMode refMode = ADC_REFERENCE_EXTERNAL;
```

### EPWM Frequency Calculation

Default period is 1000 counts. For different frequencies:

```c
// For 10kHz PWM at 100MHz SYSCLK:
epwmPeriod = 10000;

// For 1kHz PWM at 100MHz SYSCLK:
epwmPeriod = 100000;
```

## Task Implementation

### Task 1: ADC/PWM Triggering and Measurement
- ✅ EPWM1 SOCA triggers ADC conversion
- ✅ Max/min values tracked in `adcResultMin` and `adcResultMax`
- ✅ Reference voltage mode configurable

### Task 2: Low-Pass Filter Impact
- ✅ Install capacitor on board
- ✅ Compare ADC results with/without filter
- ✅ Code remains the same, filter affects hardware

### Task 3: PWM Duty Cycle Control
- ✅ ADC result (0-4095) maps to duty cycle (0%-100%)
- ✅ 0V input → 0% duty cycle
- ✅ 3.3V input → 100% duty cycle
- ✅ Linear mapping: `compare = (ADC * period) / 4095`

### Task 4: Temperature Sensor (Optional)
- 📝 Template code provided in `int.c` (commented)
- 📝 Requires additional SOC configuration in `main.c`

## Usage

1. **Include C2000Ware**: Ensure C2000Ware SDK is in your project path
2. **Include Headers**: `driverlib.h` and `device.h` must be available
3. **Build Project**: Compile with Code Composer Studio or your IDE
4. **Flash Device**: Program the F280049C LaunchPad
5. **Monitor Variables**: Use debugger to view `adcResultMin`, `adcResultMax`, `currentAdcResult`

## Key Variables

- `adcResultMin` - Minimum ADC value recorded (Task 1)
- `adcResultMax` - Maximum ADC value recorded (Task 1)
- `currentAdcResult` - Latest ADC reading
- `epwmPeriod` - EPWM period value
- `epwmCompareValue` - Current EPWM compare value (duty cycle)

## Notes

- ADC resolution: 12-bit (0-4095 counts)
- Reference voltage: 3.3V (internal or external)
- EPWM default period: 1000 counts
- Sample window: 10 SYSCLK cycles
- Interrupt: ADCA1 (for ADC interrupt 1)

## Troubleshooting

1. **No ADC conversions**: Check EPWM is running and generating SOCA
2. **Wrong ADC values**: Verify reference voltage configuration
3. **Duty cycle not updating**: Check shadow register load mode
4. **Interrupt not firing**: Verify interrupt enable and registration

## References

- F280049C Technical Reference Manual
- C2000Ware SDK Examples
- `HOMEWORK_GUIDE.md` for detailed explanations

