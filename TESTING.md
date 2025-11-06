# Testing Document - ADC/PWM Homework
## F280049C 100-pin LaunchPad

This document provides comprehensive testing procedures for all homework tasks.

---

## Table of Contents

1. [Pre-Testing Setup](#pre-testing-setup)
2. [Task 1: ADC/PWM Triggering and Max/Min Tracking](#task-1-adcpwm-triggering-and-maxmin-tracking)
3. [Task 2: Low-Pass Filter Impact](#task-2-low-pass-filter-impact)
4. [Task 3: ADC to PWM Duty Cycle Control](#task-3-adc-to-pwm-duty-cycle-control)
5. [Troubleshooting](#troubleshooting)
6. [Test Results Template](#test-results-template)

---

## Pre-Testing Setup

### Hardware Requirements

- [ ] F280049C LaunchPad board
- [ ] USB cable for programming
- [ ] Oscilloscope (for PWM output verification)
- [ ] Function generator or potentiometer (for ADC input)
- [ ] Multimeter (for voltage measurement)
- [ ] 330pF capacitor (for Task 2)
- [ ] Soldering equipment (for Task 2)

### Software Requirements

- [ ] Code Composer Studio (CCS) installed
- [ ] C2000Ware SDK with F280049C support
- [ ] Project compiled and ready to flash
- [ ] Debugger connection established

### Initial Hardware Connections

1. **Power**: Connect USB cable to LaunchPad
2. **ADC Input**: Connect signal source to **ADCIN3** pin
   - Check board schematic for exact pin number
   - Input range: 0V to 3.3V
3. **PWM Output**: Connect oscilloscope probe to **GPIO0** (EPWM1A)
4. **Ground**: Ensure common ground between signal source and board

### Code Configuration Check

Before testing, verify in `int.c`:
- [ ] `#define USE_INTERNAL_REF 0` (for external reference testing)
- [ ] ADC channel set to ADCIN3 (CHSEL = 3)
- [ ] Interrupt handler registered in `main.c`

---

## Task 1: ADC/PWM Triggering and Max/Min Tracking

### Objective
Verify that EPWM1 SOCA triggers ADC conversions, and that max/min values are tracked correctly. Compare results with internal vs external reference voltage.

### Test 1.1: Verify EPWM Triggers ADC

#### Setup
1. Flash the code to the board
2. Start debugger session
3. Set breakpoint in `EPWM1_ISR()` function
4. Run the program

#### Procedure
1. **Verify interrupt is firing**:
   - Program should hit breakpoint repeatedly
   - Check interrupt frequency: Should fire every 10 μs (100 kHz)
   - If breakpoint doesn't hit, see Troubleshooting section

2. **Verify ADC is converting**:
   - In debugger, add to Watch Window:
     - `AdcaResultRegs.ADCRESULT3`
   - Step through ISR and observe ADC result value
   - Value should be between 0-4095 (12-bit ADC)

3. **Verify EPWM is running**:
   - In Watch Window, monitor:
     - `EPwm1Regs.TBCTR` - Should be counting 0-999
     - `EPwm1Regs.TBPRD` - Should be 999
   - On oscilloscope, verify PWM signal on GPIO0

#### Expected Results
- ✅ Interrupt fires every 10 μs
- ✅ ADC result register updates with new values
- ✅ PWM signal visible on oscilloscope (100 kHz, variable duty cycle)

#### Pass/Fail Criteria
- [ ] Interrupt fires consistently
- [ ] ADC result register shows valid values (0-4095)
- [ ] PWM output is present on GPIO0

---

### Test 1.2: Max/Min Value Tracking (External Reference)

#### Setup
1. Ensure `#define USE_INTERNAL_REF 0` in `int.c`
2. Rebuild and flash code
3. Connect variable voltage source to ADCIN3 (0V to 3.3V)
4. Start debugger session

#### Procedure
1. **Add variables to Watch Window**:
   - `adcResultMin`
   - `adcResultMax`
   - `AdcaResultRegs.ADCRESULT3`

2. **Test with 0V input**:
   - Set input voltage to 0V
   - Wait 1-2 seconds
   - Record `adcResultMin` and `adcResultMax`
   - Expected: `adcResultMin` should be close to 0

3. **Test with 3.3V input**:
   - Set input voltage to 3.3V
   - Wait 1-2 seconds
   - Record `adcResultMin` and `adcResultMax`
   - Expected: `adcResultMax` should be close to 4095

4. **Test with varying voltage**:
   - Slowly vary input from 0V to 3.3V and back
   - Observe `adcResultMin` and `adcResultMax` updating
   - Verify min decreases and max increases appropriately

#### Expected Results
- ✅ `adcResultMin` tracks lowest ADC value seen
- ✅ `adcResultMax` tracks highest ADC value seen
- ✅ Values update in real-time as input changes
- ✅ At 0V: `adcResultMin` ≈ 0-50 (allowing for noise)
- ✅ At 3.3V: `adcResultMax` ≈ 4045-4095 (allowing for noise)

#### Pass/Fail Criteria
- [ ] `adcResultMin` updates when lower values are detected
- [ ] `adcResultMax` updates when higher values are detected
- [ ] Values are within expected range for given input voltage

#### Test Data
| Input Voltage | Expected ADC | Actual ADC | adcResultMin | adcResultMax |
|--------------|--------------|------------|--------------|--------------|
| 0.0V         | 0-50         |            |              |              |
| 1.65V        | ~2048        |            |              |              |
| 3.3V         | 4045-4095    |            |              |              |

---

### Test 1.3: Internal Reference Comparison

#### Setup
1. Change `#define USE_INTERNAL_REF 1` in `int.c`
2. Rebuild and flash code
3. Use same test setup as Test 1.2

#### Procedure
1. **Repeat Test 1.2 procedure** with internal reference
2. **Record max/min values** for same input voltages
3. **Compare results** with external reference test

#### Expected Results
- ✅ ADC conversions work with internal reference
- ✅ Max/min tracking works correctly
- ✅ Values may differ slightly from external reference (due to reference accuracy)

#### Comparison Table
| Input Voltage | External Ref ADC | Internal Ref ADC | Difference |
|--------------|------------------|------------------|------------|
| 0.0V         |                  |                  |            |
| 1.65V        |                  |                  |            |
| 3.3V         |                  |                  |            |

#### Pass/Fail Criteria
- [ ] Internal reference works correctly
- [ ] Max/min tracking works with internal reference
- [ ] Results documented for comparison

---

## Task 2: Low-Pass Filter Impact

### Objective
Observe the impact of adding a 330pF capacitor filter on ADC conversion results.

### Test 2.1: ADC Without Filter

#### Setup
1. Ensure no capacitor is installed on board
2. Connect function generator or noisy signal source to ADCIN3
3. Use external reference (`#define USE_INTERNAL_REF 0`)
4. Start debugger session

#### Procedure
1. **Apply clean DC voltage** (e.g., 1.65V):
   - Monitor `AdcaResultRegs.ADCRESULT3` in Watch Window
   - Record average value and observe variations
   - Note any noise or fluctuations

2. **Apply square wave** (if function generator available):
   - Frequency: 1 kHz, amplitude: 0V to 3.3V
   - Observe ADC readings
   - Note response time and any overshoot/ringing

3. **Apply noisy signal** (if available):
   - Add high-frequency noise to DC signal
   - Observe ADC readings
   - Record noise level in ADC readings

#### Expected Results
- ✅ ADC readings may show noise/fluctuations
- ✅ Fast signal changes may cause overshoot
- ✅ High-frequency noise visible in ADC results

#### Test Data
| Signal Type | Average ADC | Noise Level | Notes |
|------------|-------------|-------------|-------|
| Clean DC 1.65V |             |             |       |
| Square Wave 1kHz |            |             |       |
| Noisy Signal    |             |             |       |

---

### Test 2.2: ADC With Filter

#### Setup
1. **Install 330pF capacitor**:
   - Locate capacitor footprint for ADCIN3 on board (check schematic)
   - Solder 330pF capacitor between ADC input and GND
   - Verify 0-ohm resistor is installed (R14, R16, R17, R18, R20, or R21)
2. Use same test setup as Test 2.1

#### Procedure
1. **Repeat Test 2.1 procedure** with capacitor installed
2. **Compare results** with Test 2.1 (without filter)

#### Expected Results
- ✅ ADC readings should be smoother (less noise)
- ✅ Fast signal changes may have slower response
- ✅ High-frequency noise should be reduced
- ✅ Overshoot/ringing should be reduced

#### Comparison Table
| Signal Type | Without Filter ADC | With Filter ADC | Improvement |
|------------|-------------------|-----------------|-------------|
| Clean DC 1.65V |                   |                 |             |
| Square Wave 1kHz |                  |                 |             |
| Noisy Signal    |                   |                 |             |

#### Pass/Fail Criteria
- [ ] Filter reduces noise in ADC readings
- [ ] Filter smooths out fast signal changes
- [ ] Trade-off: Slower response to fast changes is acceptable

---

## Task 3: ADC to PWM Duty Cycle Control

### Objective
Verify that ADC input voltage controls PWM duty cycle linearly: 0V → 0%, 3.3V → 100%.

### Test 3.1: Duty Cycle at 0V Input

#### Setup
1. Connect 0V (GND) to ADCIN3
2. Connect oscilloscope to GPIO0 (EPWM1A)
3. Start debugger session

#### Procedure
1. **Measure PWM signal**:
   - Set oscilloscope to 10 μs/div (for 100 kHz signal)
   - Measure duty cycle
   - Record high time and period

2. **Verify in debugger**:
   - Check `EPwm1Regs.CMPA.bit.CMPA` in Watch Window
   - Expected: Should be close to 0

3. **Calculate duty cycle**:
   - Duty cycle = (CMPA / TBPRD) × 100%
   - Expected: ~0%

#### Expected Results
- ✅ PWM duty cycle ≈ 0%
- ✅ CMPA value ≈ 0-10 (allowing for noise)
- ✅ Oscilloscope shows mostly low signal

#### Pass/Fail Criteria
- [ ] Duty cycle < 5%
- [ ] CMPA < 50

---

### Test 3.2: Duty Cycle at 1.65V Input (50%)

#### Setup
1. Connect 1.65V to ADCIN3
2. Connect oscilloscope to GPIO0
3. Start debugger session

#### Procedure
1. **Measure PWM signal**:
   - Measure duty cycle on oscilloscope
   - Record high time and period

2. **Verify in debugger**:
   - Check `EPwm1Regs.CMPA.bit.CMPA`
   - Expected: Should be close to 500 (50% of 1000)

3. **Calculate duty cycle**:
   - Expected: ~50%

#### Expected Results
- ✅ PWM duty cycle ≈ 50% (±2%)
- ✅ CMPA value ≈ 480-520
- ✅ Oscilloscope shows 50% duty cycle

#### Pass/Fail Criteria
- [ ] Duty cycle between 48% and 52%
- [ ] CMPA between 480 and 520

---

### Test 3.3: Duty Cycle at 3.3V Input (100%)

#### Setup
1. Connect 3.3V to ADCIN3
2. Connect oscilloscope to GPIO0
3. Start debugger session

#### Procedure
1. **Measure PWM signal**:
   - Measure duty cycle on oscilloscope
   - Record high time and period

2. **Verify in debugger**:
   - Check `EPwm1Regs.CMPA.bit.CMPA`
   - Expected: Should be close to 999 (100% of 1000)

3. **Calculate duty cycle**:
   - Expected: ~100%

#### Expected Results
- ✅ PWM duty cycle ≈ 100% (>95%)
- ✅ CMPA value ≈ 990-999
- ✅ Oscilloscope shows mostly high signal

#### Pass/Fail Criteria
- [ ] Duty cycle > 95%
- [ ] CMPA > 990

---

### Test 3.4: Linear Relationship Verification

#### Setup
1. Connect variable voltage source to ADCIN3
2. Connect oscilloscope to GPIO0
3. Connect multimeter to measure input voltage

#### Procedure
1. **Test multiple voltage points**:
   - Measure input voltage with multimeter
   - Measure PWM duty cycle with oscilloscope
   - Record CMPA value from debugger
   - Test at: 0V, 0.5V, 1.0V, 1.5V, 2.0V, 2.5V, 3.0V, 3.3V

2. **Calculate expected vs actual**:
   - Expected duty cycle = (Input Voltage / 3.3V) × 100%
   - Compare with measured duty cycle

#### Expected Results
- ✅ Linear relationship between input voltage and duty cycle
- ✅ Small deviations acceptable (±2-3%)
- ✅ Smooth transition as voltage changes

#### Test Data Table
| Input Voltage | Expected Duty % | Measured Duty % | CMPA Value | Error % |
|--------------|-----------------|-----------------|------------|---------|
| 0.0V         | 0%              |                 |            |         |
| 0.5V         | 15.2%           |                 |            |         |
| 1.0V         | 30.3%           |                 |            |         |
| 1.5V         | 45.5%           |                 |            |         |
| 2.0V         | 60.6%           |                 |            |         |
| 2.5V         | 75.8%           |                 |            |         |
| 3.0V         | 90.9%           |                 |            |         |
| 3.3V         | 100%            |                 |            |         |

#### Pass/Fail Criteria
- [ ] Linear relationship maintained across full range
- [ ] Error < 5% at all test points
- [ ] Smooth response to voltage changes

---

### Test 3.5: Dynamic Response

#### Setup
1. Connect function generator to ADCIN3
2. Connect oscilloscope to GPIO0
3. Set function generator to triangle wave: 0V to 3.3V, 1 Hz

#### Procedure
1. **Observe PWM response**:
   - Monitor PWM duty cycle on oscilloscope
   - Verify duty cycle follows input voltage
   - Check for any delays or overshoot

2. **Test with faster signal**:
   - Increase frequency to 10 Hz, 100 Hz
   - Observe if PWM can track the signal
   - Note any limitations

#### Expected Results
- ✅ PWM duty cycle tracks input voltage smoothly
- ✅ Response is fast enough for low-frequency signals
- ✅ Update rate is 100 kHz (10 μs), so should track signals up to ~1 kHz

#### Pass/Fail Criteria
- [ ] PWM tracks input at 1 Hz
- [ ] PWM tracks input at 10 Hz
- [ ] Some delay acceptable at higher frequencies

---

## Troubleshooting

### Problem: Interrupt Not Firing

**Symptoms**:
- Breakpoint in `EPWM1_ISR()` never hits
- PWM output not present
- ADC values not updating

**Solutions**:
1. Verify interrupt handler is registered in `main.c`:
   ```c
   PieVectTable.EPWM1_INT = &EPWM1_ISR;
   ```

2. Check PIE configuration:
   - `PieCtrlRegs.PIEIER3.bit.INTx1 = 1` (enabled)
   - `IER |= M_INT3` (CPU interrupt enabled)

3. Verify ePWM is running:
   - Check `EPwm1Regs.TBCTR` is counting
   - Verify clock is enabled: `CpuSysRegs.PCLKCR2.bit.EPWM1 = 1`

4. Check interrupt flags:
   - Verify `EPwm1Regs.ETSEL.bit.INTEN = 1`
   - Check if interrupt flag is stuck: `EPwm1Regs.ETFLG.bit.INT`

---

### Problem: ADC Not Converting

**Symptoms**:
- `ADCRESULT3` shows 0 or constant value
- ADC values don't change with input

**Solutions**:
1. Verify ADC clock is enabled:
   - `CpuSysRegs.PCLKCR13.bit.ADC_A = 1`

2. Check SOC configuration:
   - `AdcaRegs.ADCSOC3CTL.bit.TRIGSEL = 5` (EPWM1 SOCA)
   - `AdcaRegs.ADCSOC3CTL.bit.CHSEL = 3` (ADCIN3)

3. Verify EPWM SOCA is generating:
   - Check `EPwm1Regs.ETSEL.bit.SOCAEN = 1`
   - Verify SOCA event: `EPwm1Regs.ETSEL.bit.SOCASEL = 1`

4. Check input voltage:
   - Measure with multimeter
   - Ensure 0V to 3.3V range
   - Verify connection to correct pin

---

### Problem: PWM Duty Cycle Not Updating

**Symptoms**:
- PWM duty cycle is constant
- CMPA value doesn't change

**Solutions**:
1. Verify interrupt is firing (see above)

2. Check shadow register configuration:
   - `EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW`
   - `EPwm1Regs.CMPCTL.bit.LOADAMODE = 0` (load at TBCTR=0)

3. Verify CMPA is being updated in ISR:
   - Set breakpoint in ISR
   - Step through and verify CMPA calculation

4. Check for overflow in calculation:
   - Verify `(c * EPwm1Regs.TBPRD) >> 12` calculation
   - Ensure `c` is valid ADC value (0-4095)

---

### Problem: Max/Min Values Not Updating

**Symptoms**:
- `adcResultMin` stays at 0xFFFF
- `adcResultMax` stays at 0x0000

**Solutions**:
1. Verify interrupt is firing

2. Check global variables are accessible:
   - Add to Watch Window
   - Verify they're not optimized away

3. Verify comparison logic in ISR:
   - Check `if (c < adcResultMin)` condition
   - Check `if (c > adcResultMax)` condition

4. Reset values manually in debugger:
   - Set `adcResultMin = 0xFFFF`
   - Set `adcResultMax = 0x0000`
   - Let program run and observe

---

### Problem: Wrong ADC Values

**Symptoms**:
- ADC values don't match expected for input voltage
- Values are off by large amount

**Solutions**:
1. Verify reference voltage:
   - Check `USE_INTERNAL_REF` define
   - Verify reference is properly configured

2. Check input voltage:
   - Measure with multimeter
   - Verify connection

3. Verify ADC channel:
   - Check `CHSEL = 3` for ADCIN3
   - Verify correct pin on board

4. Check for noise:
   - Add filter capacitor (Task 2)
   - Verify clean power supply

---

## Test Results Template

### Test Summary

**Date**: _______________  
**Tester**: _______________  
**Board Serial Number**: _______________  
**Code Version**: _______________

### Task 1 Results

| Test | Status | Notes |
|------|--------|-------|
| 1.1 EPWM Triggers ADC | ☐ Pass ☐ Fail | |
| 1.2 Max/Min Tracking (External) | ☐ Pass ☐ Fail | |
| 1.3 Internal Reference | ☐ Pass ☐ Fail | |

**Max/Min Values (External Reference)**:
- Min at 0V: _______
- Max at 3.3V: _______

**Max/Min Values (Internal Reference)**:
- Min at 0V: _______
- Max at 3.3V: _______

### Task 2 Results

| Test | Status | Notes |
|------|--------|-------|
| 2.1 Without Filter | ☐ Pass ☐ Fail | |
| 2.2 With Filter | ☐ Pass ☐ Fail | |

**Filter Impact**: 
- Noise reduction: ☐ Improved ☐ No change
- Response time: ☐ Slower ☐ Same

### Task 3 Results

| Test | Status | Notes |
|------|--------|-------|
| 3.1 0V Input (0%) | ☐ Pass ☐ Fail | |
| 3.2 1.65V Input (50%) | ☐ Pass ☐ Fail | |
| 3.3 3.3V Input (100%) | ☐ Pass ☐ Fail | |
| 3.4 Linear Relationship | ☐ Pass ☐ Fail | |
| 3.5 Dynamic Response | ☐ Pass ☐ Fail | |

**Linearity Error**: Maximum error = _______%

### Overall Results

- **Task 1**: ☐ Complete ☐ Incomplete
- **Task 2**: ☐ Complete ☐ Incomplete  
- **Task 3**: ☐ Complete ☐ Incomplete

**Issues Encountered**:
_________________________________________________
_________________________________________________
_________________________________________________

**Sign-off**: _______________

---

## Notes

- All tests should be performed in a controlled environment
- Allow system to stabilize before taking measurements
- Record all measurements with appropriate precision
- Take multiple measurements and average for accuracy
- Document any deviations from expected results
- Keep oscilloscope screenshots for documentation

---

**End of Testing Document**

