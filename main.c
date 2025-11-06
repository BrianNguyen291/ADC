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

// External interrupt handler declaration
extern __interrupt void adca1_isr(void);

// Global variables for Task 1 (max/min tracking)
uint16_t adcResultMin = 0xFFFF;      // Minimum ADC result
uint16_t adcResultMax = 0x0000;     // Maximum ADC result
uint16_t currentAdcResult = 0;       // Current ADC reading

// Global variables for Task 3 (PWM duty cycle control)
uint16_t epwmPeriod = 1000;          // EPWM period value
uint16_t epwmCompareValue = 0;      // EPWM compare value (duty cycle)

// Reference voltage mode (change this to test Task 1)
// Set to ADC_REFERENCE_INTERNAL for internal reference
// Set to ADC_REFERENCE_EXTERNAL for external reference
ADC_ReferenceMode refMode = ADC_REFERENCE_INTERNAL;

// Function prototypes
void configureEPWM1(void);
void configureADC(void);
void configureADCInterrupt(void);
void delay_us(uint32_t count);

//
// Main function
//
void main(void)
{
    // Initialize device clock and peripherals
    Device_init();
    
    // Initialize GPIO pins
    Device_initGPIO();
    
    // Initialize interrupt controller
    Interrupt_initModule();
    
    // Initialize interrupt vector table
    Interrupt_initVectorTable();
    
    // Configure EPWM1 to generate SOCA trigger
    configureEPWM1();
    
    // Configure ADC module
    configureADC();
    
    // Configure ADC interrupt
    configureADCInterrupt();
    
    // Register ADC interrupt handler
    Interrupt_register(INT_ADCA1, &adca1_isr);
    Interrupt_enable(INT_ADCA1);
    
    // Enable global interrupts
    EINT;  // Enable global interrupts
    ERTM;  // Enable real-time interrupt
    
    // Start EPWM time-base counter
    EPWM_startTimeBaseCounter(EPWM1_BASE);
    
    // Main loop
    while(1)
    {
        // Main loop - ADC conversions happen in interrupt
        // Max/min values and duty cycle are updated automatically
        
        // Add delay or other processing here if needed
        delay_us(1000);
    }
}

//
// Configure EPWM1 to generate SOCA trigger for ADC
//
void configureEPWM1(void)
{
    // Set time-base period (adjust for desired PWM frequency)
    // For 100MHz SYSCLK and 10kHz PWM: period = 10000
    // For this example, using 1000 counts
    epwmPeriod = 1000;
    EPWM_setTimeBasePeriod(EPWM1_BASE, epwmPeriod);
    
    // Set initial compare value (0% duty cycle initially)
    epwmCompareValue = 0;
    EPWM_setCounterCompareValue(EPWM1_BASE, 
                                EPWM_COUNTER_COMPARE_A, 
                                epwmCompareValue);
    
    // Configure shadow register load mode (load on period for glitch-free updates)
    EPWM_setCounterCompareShadowLoadMode(EPWM1_BASE,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_SHADOW_LOAD_MODE_PERIOD);
    
    // Enable ADC trigger SOCA
    EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
    
    // Set ADC trigger event prescaler (1 = every SOC event)
    EPWM_setADCTriggerEventPrescale(EPWM1_BASE, EPWM_SOC_A, 1);
    
    // Set ADC trigger source (trigger when counter equals CMPA)
    EPWM_setADCTriggerSource(EPWM1_BASE, 
                             EPWM_SOC_A, 
                             EPWM_SOC_TBCTR_CMPA);
    
    // Set time-base counter mode (up counting)
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP);
    
    // Enable time-base counter
    EPWM_enableTimeBaseCounter(EPWM1_BASE);
    
    // Set time-base counter to zero initially
    EPWM_setTimeBaseCounter(EPWM1_BASE, 0);
}

//
// Configure ADC module
//
void configureADC(void)
{
    // Enable ADC module
    ADC_enableModule(ADCA_BASE);
    
    // Enable ADC converter (power up analog circuitry)
    ADC_enableConverter(ADCA_BASE);
    
    // Wait for ADC to power up (minimum 500us required)
    // Assuming SYSCLK = 100MHz, delay ~50000 cycles
    delay_us(500);
    
    // Set reference voltage mode
    // For Task 1: Test with both INTERNAL and EXTERNAL
    // Change refMode variable to test different references
    ADC_setVREF(ADCA_BASE, refMode, ADC_REFERENCE_3_3V);
    
    // Load offset trims for all ADC instances
    ADC_setOffsetTrimAll(refMode, ADC_REFERENCE_3_3V);
    
    // Setup SOC0 to trigger from EPWM1 SOCA
    // Channel: ADC_CH_ADCIN0 (adjust pin number as needed for your board)
    // For F280049C 100-pin, check schematic for ADC input pins
    ADC_setupSOC(ADCA_BASE,
                 ADC_SOC_NUMBER0,              // SOC number 0
                 ADC_TRIGGER_EPWM1_SOCA,       // Trigger from EPWM1 SOCA
                 ADC_CH_ADCIN0,                // ADC input channel (adjust as needed)
                 10);                          // Sample window (SYSCLK cycles)
    
    // Configure interrupt pulse mode (pulse at end of conversion)
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);
}

//
// Configure ADC interrupt
//
void configureADCInterrupt(void)
{
    // Set interrupt source for SOC0
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    
    // Enable ADC interrupt
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    
    // Disable continuous mode (interrupt fires once per conversion)
    ADC_disableContinuousMode(ADCA_BASE, ADC_INT_NUMBER1);
}

//
// Simple delay function (approximate microsecond delay)
// Adjust based on your SYSCLK frequency
//
void delay_us(uint32_t count)
{
    volatile uint32_t i;
    for(i = 0; i < count; i++)
    {
        // Assuming ~100MHz SYSCLK, adjust loop count as needed
        // For 1us at 100MHz: ~100 cycles
        __asm("    NOP");
        __asm("    NOP");
        __asm("    NOP");
        __asm("    NOP");
        __asm("    NOP");
    }
}

