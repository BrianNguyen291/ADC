# ADC/PWM Homework Implementation Guide
## Texas Instruments F280049C LaunchPad

This guide explains how to implement the homework tasks using the C2000Ware SDK for the F280049C microcontroller.

---

## Task 1: ADC/PWM Triggering and Measurement

### Objective
Use EPWMx SOCx to trigger ADCx SOCx to sample ADCAINx pin. In the ADC interrupt subroutine, record max/min values and compare internal vs external reference voltage.

### Implementation Steps

#### 1. Configure EPWM to Generate SOC Events

```c
#include "driverlib.h"
#include "device.h"

// Configure EPWM1 to generate SOCA event
void configureEPWM1(void)
{
    // Set time-base period (adjust for desired PWM frequency)
    EPWM_setTimeBasePeriod(EPWM1_BASE, 1000);  // Example: 1000 counts
    
    // Set compare value to trigger SOCA (e.g., at 50% duty cycle)
    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, 500);
    
    // Enable SOCA on counter = CMPA
    EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
    EPWM_setADCTriggerEventPrescale(EPWM1_BASE, EPWM_SOC_A, 1);
    
    // Set SOCA trigger source (when counter equals CMPA)
    EPWM_setADCTriggerSource(EPWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_CMPA);
    
    // Set time-base counter mode
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP);
    
    // Enable time-base counter
    EPWM_enableTimeBaseCounter(EPWM1_BASE);
}
```

#### 2. Configure ADC with EPWM SOC Trigger

```c
void configureADC(void)
{
    // Enable ADC module
    ADC_enableModule(ADCA_BASE);
    
    // Set reference voltage (INTERNAL for Task 1a, EXTERNAL for Task 1b)
    // For internal reference:
    ADC_setVREF(ADCA_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    ADC_setOffsetTrimAll(ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    
    // For external reference (uncomment when testing Task 1b):
    // ADC_setVREF(ADCA_BASE, ADC_REFERENCE_EXTERNAL, ADC_REFERENCE_3_3V);
    // ADC_setOffsetTrimAll(ADC_REFERENCE_EXTERNAL, ADC_REFERENCE_3_3V);
    
    // Setup SOC0 to trigger from EPWM1 SOCA
    ADC_setupSOC(ADCA_BASE, 
                 ADC_SOC_NUMBER0,              // SOC number
                 ADC_TRIGGER_EPWM1_SOCA,      // Trigger from EPWM1 SOCA
                 ADC_CH_ADCIN0,               // Channel (adjust to your pin: ADCAINx)
                 10);                          // Sample window (SYSCLK cycles)
    
    // Enable ADC interrupt for SOC0
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);
}
```

#### 3. Global Variables for Max/Min Tracking

```c
// Global variables for Task 1
uint16_t adcResultMin = 0xFFFF;  // Start with maximum value
uint16_t adcResultMax = 0x0000;  // Start with minimum value
uint16_t currentAdcResult = 0;
```

#### 4. ADC Interrupt Service Routine

```c
__interrupt void adca1_isr(void)
{
    // Read ADC result
    currentAdcResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    
    // Track minimum value
    if (currentAdcResult < adcResultMin)
    {
        adcResultMin = currentAdcResult;
    }
    
    // Track maximum value
    if (currentAdcResult > adcResultMax)
    {
        adcResultMax = currentAdcResult;
    }
    
    // Clear interrupt flag
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    
    // Acknowledge interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
```

#### 5. Register Interrupt Handler

```c
void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    
    // Register ADC interrupt
    Interrupt_register(INT_ADCA1, &adca1_isr);
    Interrupt_enable(INT_ADCA1);
    
    // Configure EPWM and ADC
    configureEPWM1();
    configureADC();
    
    // Enable global interrupts
    EINT;
    ERTM;
    
    // Start EPWM
    EPWM_startTimeBaseCounter(EPWM1_BASE);
    
    while(1)
    {
        // Main loop - max/min values are updated in interrupt
        // You can add code here to display or log results
    }
}
```

### Comparison Notes
- **Internal Reference**: Typically 2.5V or 3.3V, factory calibrated
- **External Reference**: Uses VREFHI pin, more stable but requires external source
- Compare `adcResultMax` and `adcResultMin` values between the two configurations
- External reference usually provides better accuracy and stability

---

## Task 2: Low-Pass Filter Impact

### Objective
Use the low-pass filter circuit space on the development board to observe the impact of a filter capacitor on ADC conversion.

### Implementation

#### Hardware Setup
1. Locate the DNP (Do Not Populate) capacitor footprints on the board (C26-C31, 330pF each)
2. Connect a capacitor between GND and the ADC input pin through the 0-ohm resistor (R14, R16, R17, etc.)
3. Apply a signal source with varying frequency to ADCINx

#### Code Changes
```c
// Use the same ADC configuration as Task 1
// The filter will automatically affect the signal

// For testing, you can:
// 1. Apply a square wave with fast edges
// 2. Measure ADC results with and without capacitor
// 3. Observe how capacitor smooths out high-frequency noise

// Example: Compare results
uint16_t adcResultWithFilter = 0;
uint16_t adcResultWithoutFilter = 0;

// In your ISR or main loop, compare results
// The capacitor will reduce high-frequency noise and ripple
```

### Observations
- **Without filter**: ADC may show noise/ripple from high-frequency components
- **With filter**: ADC results should be smoother, but may have slower response to fast changes
- **330pF capacitor**: Creates a low-pass filter with RC time constant based on source impedance

---

## Task 3: PWM Duty Cycle Control from ADC

### Objective
Use ADCRESULTx to adjust EPWM duty cycle. When input is 0V → duty cycle = 0%, when input is 3.3V → duty cycle = 100%.

### Implementation

#### 1. Modify ADC Interrupt to Update PWM Duty Cycle

```c
// Global variable for EPWM period (set during initialization)
uint16_t epwmPeriod = 1000;  // Example: 1000 counts

__interrupt void adca1_isr(void)
{
    uint16_t adcResult;
    uint16_t epwmCompareValue;
    
    // Read ADC result (12-bit: 0-4095 for 3.3V reference)
    adcResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    
    // Linear mapping: 0V (ADC=0) → 0% duty, 3.3V (ADC=4095) → 100% duty
    // Duty cycle = (ADC_result / 4095) * 100%
    // Compare value = (ADC_result * period) / 4095
    epwmCompareValue = (adcResult * epwmPeriod) / 4095;
    
    // Update EPWM compare value (use shadow register for glitch-free update)
    EPWM_setCounterCompareValue(EPWM1_BASE, 
                                EPWM_COUNTER_COMPARE_A, 
                                epwmCompareValue);
    
    // Optional: Also track min/max for Task 1
    if (adcResult < adcResultMin) adcResultMin = adcResult;
    if (adcResult > adcResultMax) adcResultMax = adcResult;
    
    // Clear interrupt
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
```

#### 2. Configure EPWM with Shadow Register

```c
void configureEPWM1(void)
{
    // Set time-base period
    epwmPeriod = 1000;
    EPWM_setTimeBasePeriod(EPWM1_BASE, epwmPeriod);
    
    // Set compare value to 0 initially (0% duty cycle)
    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, 0);
    
    // Configure shadow register load mode (load on period)
    EPWM_setCounterCompareShadowLoadMode(EPWM1_BASE, 
                                         EPWM_COUNTER_COMPARE_A, 
                                         EPWM_SHADOW_LOAD_MODE_PERIOD);
    
    // Enable SOCA trigger
    EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
    EPWM_setADCTriggerSource(EPWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_CMPA);
    
    // Configure output pins (if needed)
    // EPWM_setActionQualifierAction(...);
    
    // Set counter mode
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP);
    
    // Enable counter
    EPWM_enableTimeBaseCounter(EPWM1_BASE);
}
```

#### 3. Verification
- **Input 0V**: ADC ≈ 0 → Compare value = 0 → Duty cycle = 0%
- **Input 1.65V**: ADC ≈ 2048 → Compare value ≈ 500 → Duty cycle ≈ 50%
- **Input 3.3V**: ADC ≈ 4095 → Compare value = period → Duty cycle = 100%

---

## Task 4: Temperature Sensor (Optional)

### Objective
Use EPWMx SOCx to trigger a TempSensor conversion.

### Implementation

```c
void configureTemperatureSensor(void)
{
    // Setup SOC for temperature sensor channel
    // Temperature sensor is typically on a dedicated ADC channel
    // Check device datasheet for exact channel number
    
    // Example: If temperature sensor is on ADC channel 15
    ADC_setupSOC(ADCA_BASE,
                 ADC_SOC_NUMBER1,              // Use different SOC number
                 ADC_TRIGGER_EPWM1_SOCA,      // Same EPWM trigger
                 ADC_CH_ADCIN15,              // Temperature sensor channel
                 10);                         // Sample window
    
    // Enable interrupt for temperature sensor SOC
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER2, ADC_SOC_NUMBER1);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER2);
}

__interrupt void adca2_isr(void)
{
    uint16_t tempResult;
    int16_t tempCelsius;
    
    // Read temperature sensor result
    tempResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    
    // Convert to temperature (using internal reference)
    tempCelsius = ADC_getTemperatureC(tempResult, 
                                      ADC_REFERENCE_INTERNAL, 
                                      2.5f);  // Reference voltage
    
    // Process temperature reading...
    
    // Clear interrupt
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER2);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
```

### Temperature Conversion Function
```c
// ADC_getTemperatureC() is available in C2000Ware
// Parameters:
// - tempResult: Raw ADC reading from temperature sensor
// - refMode: ADC_REFERENCE_INTERNAL or ADC_REFERENCE_EXTERNAL
// - vref: Reference voltage used (typically 2.5V or 3.3V)
```

---

## Complete Example Structure

```c
#include "driverlib.h"
#include "device.h"

// Global variables
uint16_t adcResultMin = 0xFFFF;
uint16_t adcResultMax = 0x0000;
uint16_t epwmPeriod = 1000;

// Function prototypes
void configureEPWM1(void);
void configureADC(void);
void configureADCInterrupt(void);

// Interrupt handlers
__interrupt void adca1_isr(void);

void main(void)
{
    // Initialize device
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    
    // Configure peripherals
    configureEPWM1();
    configureADC();
    configureADCInterrupt();
    
    // Register interrupt
    Interrupt_register(INT_ADCA1, &adca1_isr);
    Interrupt_enable(INT_ADCA1);
    
    // Enable interrupts
    EINT;
    ERTM;
    
    // Start EPWM
    EPWM_startTimeBaseCounter(EPWM1_BASE);
    
    // Main loop
    while(1)
    {
        // Add your code here
    }
}

void configureEPWM1(void)
{
    // EPWM configuration as shown above
}

void configureADC(void)
{
    // ADC configuration as shown above
}

void configureADCInterrupt(void)
{
    // Interrupt configuration
}

__interrupt void adca1_isr(void)
{
    // ISR implementation for Task 1 and Task 3
}
```

---

## Key C2000Ware Functions Reference

### ADC Functions
- `ADC_setupSOC()` - Configure Start-of-Conversion
- `ADC_setVREF()` - Set reference voltage (internal/external)
- `ADC_readResult()` - Read conversion result
- `ADC_setInterruptSource()` - Configure ADC interrupt
- `ADC_clearInterruptStatus()` - Clear interrupt flag
- `ADC_getTemperatureC()` - Convert temperature sensor reading

### EPWM Functions
- `EPWM_setTimeBasePeriod()` - Set PWM period
- `EPWM_setCounterCompareValue()` - Set duty cycle (compare value)
- `EPWM_enableADCTrigger()` - Enable SOC trigger
- `EPWM_setADCTriggerSource()` - Set SOC trigger source
- `EPWM_setCounterCompareShadowLoadMode()` - Configure shadow register

### Trigger Constants
- `ADC_TRIGGER_EPWM1_SOCA` - EPWM1 SOCA trigger
- `ADC_TRIGGER_EPWM1_SOCB` - EPWM1 SOCB trigger
- `ADC_TRIGGER_EPWMx_SOCA/SOCB` - For other EPWM modules

---

## Testing Checklist

- [ ] Task 1: EPWM triggers ADC, max/min values recorded
- [ ] Task 1: Internal reference tested
- [ ] Task 1: External reference tested
- [ ] Task 1: Results compared
- [ ] Task 2: Filter capacitor installed
- [ ] Task 2: Impact of filter observed
- [ ] Task 3: Duty cycle scales from 0% to 100% with ADC input
- [ ] Task 3: Linear mapping verified (0V→0%, 3.3V→100%)
- [ ] Task 4: Temperature sensor conversion triggered (optional)

---

## Notes

1. **Pin Configuration**: Check your board schematic for exact ADC input pins (ADCAINx)
2. **Reference Voltage**: Ensure external reference is properly connected if using external mode
3. **Interrupt Priority**: Configure interrupt priorities if using multiple interrupts
4. **Shadow Registers**: Use shadow registers for glitch-free PWM updates
5. **ADC Resolution**: F280049C has 12-bit ADC (0-4095 counts)

---

## Additional Resources

- C2000Ware SDK: `device_support/f28004x/`
- ADC Examples: `device_support/f28004x/examples/adc/`
- EPWM Examples: `device_support/f28004x/examples/epwm/`
- Device Datasheet: F280049C Technical Reference Manual

Good luck with your homework!

