//
// int.c - Interrupt Service Routines for ADC/PWM Homework
// F280049C 100-pin device
//

#include "driverlib.h"
#include "device.h"

// External global variables from main.c
extern uint16_t adcResultMin;
extern uint16_t adcResultMax;
extern uint16_t currentAdcResult;
extern uint16_t epwmPeriod;
extern uint16_t epwmCompareValue;

//
// ADC Interrupt Service Routine for ADCA1
// This ISR handles all tasks:
// - Task 1: Record max/min ADC values
// - Task 3: Update EPWM duty cycle based on ADC result
//
__interrupt void adca1_isr(void)
{
    uint16_t adcResult;
    uint16_t newCompareValue;
    
    // Read ADC conversion result from SOC0
    adcResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    currentAdcResult = adcResult;
    
    // ============================================
    // TASK 1: Track Maximum and Minimum Values
    // ============================================
    
    // Update minimum value
    if (adcResult < adcResultMin)
    {
        adcResultMin = adcResult;
    }
    
    // Update maximum value
    if (adcResult > adcResultMax)
    {
        adcResultMax = adcResult;
    }
    
    // ============================================
    // TASK 3: Adjust EPWM Duty Cycle from ADC
    // ============================================
    // Linear mapping:
    //   0V input (ADC = 0)     -> 0% duty cycle (CMPA = 0)
    //   3.3V input (ADC = 4095) -> 100% duty cycle (CMPA = period)
    //
    // Formula: duty_cycle = (ADC_result / 4095) * 100%
    //          compare_value = (ADC_result * period) / 4095
    
    // Calculate new compare value for EPWM duty cycle
    // Using 32-bit math to avoid overflow
    newCompareValue = (uint16_t)(((uint32_t)adcResult * (uint32_t)epwmPeriod) / 4095U);
    
    // Update EPWM compare value (shadow register will load on period)
    EPWM_setCounterCompareValue(EPWM1_BASE,
                                EPWM_COUNTER_COMPARE_A,
                                newCompareValue);
    
    epwmCompareValue = newCompareValue;
    
    // ============================================
    // Clear interrupt flags
    // ============================================
    
    // Clear ADC interrupt status
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    
    // Acknowledge interrupt group
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//
// Optional: Temperature Sensor Interrupt (Task 4)
// Uncomment and configure if implementing Task 4
//
/*
__interrupt void adca2_isr(void)
{
    uint16_t tempResult;
    int16_t tempCelsius;
    
    // Read temperature sensor ADC result
    // Note: Check datasheet for exact temperature sensor channel
    // For F280049C, temperature sensor is typically on a specific channel
    tempResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    
    // Convert ADC result to temperature in Celsius
    // Using internal reference with 2.5V nominal
    tempCelsius = ADC_getTemperatureC(tempResult,
                                     ADC_REFERENCE_INTERNAL,
                                     2.5f);
    
    // Process temperature reading here
    // e.g., store in global variable, send via UART, etc.
    
    // Clear interrupt flags
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER2);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
*/

