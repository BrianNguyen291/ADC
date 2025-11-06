# What You Need to Do to Complete the Homework

Based on your current code, here's what's **missing** and what you need to **add**:

## ✅ What's Already Done

1. ✅ **EPWM1 SOCA triggers ADC SOC3** - Working correctly
2. ✅ **ADC samples ADCIN3** - Configured correctly  
3. ✅ **Task 3: ADC controls PWM duty cycle** - Implemented in ISR
4. ✅ **All hardware initialization** - Complete

## ❌ What's Missing

### 1. Task 1: Max/Min Value Tracking

**Problem**: The `EPWM1_ISR()` doesn't track maximum and minimum ADC values.

**What to add**:
- Global variables for `adcResultMin` and `adcResultMax`
- Logic in `EPWM1_ISR()` to update these values

**Location**: Add to `int.c`

### 2. Task 1: Internal vs External Reference Comparison

**Problem**: Only external reference is hardcoded. No way to compare with internal reference.

**What to add**:
- Option to switch between internal/external reference
- Or create two separate test configurations

**Location**: Modify `Init_ADC()` in `int.c`

### 3. Interrupt Handler Registration

**Problem**: The interrupt handler `EPWM1_ISR` is not registered in the PIE vector table.

**What to add**:
- Register the interrupt handler in `main.c`

**Location**: Add to `main.c` after PIE initialization

---

## 📝 Detailed Changes Needed

### Change 1: Add Global Variables for Max/Min Tracking

**File**: `int.c`  
**Location**: Add at the top of the file (after includes)

```c
// Global variables for Task 1: Max/Min tracking
uint16_t adcResultMin = 0xFFFF;  // Start with maximum value
uint16_t adcResultMax = 0x0000;  // Start with minimum value
```

### Change 2: Update EPWM1_ISR to Track Max/Min

**File**: `int.c`  
**Location**: Modify `EPWM1_ISR()` function (around line 177)

**Current code** (lines 177-194):
```c
interrupt void EPWM1_ISR(void)
{
    unsigned long long c = 0;
    EALLOW;
    c = AdcaResultRegs.ADCRESULT3;
    EPwm1Regs.CMPA.bit.CMPA = (c * EPwm1Regs.TBPRD) >> 12;
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    EDIS;
}
```

**Add max/min tracking** (insert after reading ADC value):
```c
interrupt void EPWM1_ISR(void)
{
    unsigned long long c = 0;
    EALLOW;
    c = AdcaResultRegs.ADCRESULT3;
    
    // Task 1: Track max/min values
    if (c < adcResultMin)
    {
        adcResultMin = (uint16_t)c;
    }
    if (c > adcResultMax)
    {
        adcResultMax = (uint16_t)c;
    }
    
    // Task 3: Adjust PWM duty based on ADC value
    EPwm1Regs.CMPA.bit.CMPA = (c * EPwm1Regs.TBPRD) >> 12;
    
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    EDIS;
}
```

### Change 3: Register Interrupt Handler

**File**: `main.c`  
**Location**: Add after `Init_PIE()` and before GPIO configuration

**Add this code** (after line 18):
```c
    Init_PIE();
    
    // Register EPWM1 interrupt handler
    EALLOW;
    PieVectTable.EPWM1_INT = &EPWM1_ISR;
    EDIS;
    
    Init_ePWM();
```

**Note**: You'll also need to add a function declaration at the top of `main.c`:
```c
// Forward declaration
extern interrupt void EPWM1_ISR(void);
```

### Change 4: Make Reference Voltage Configurable (Optional but Recommended)

**File**: `int.c`  
**Location**: Modify `Init_ADC()` function

**Option A**: Add a parameter to the function
```c
void Init_ADC(uint16_t useInternalRef)
{
    // Set reference voltage
    if (useInternalRef)
    {
        SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
    }
    else
    {
        SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);
    }
    // ... rest of function
}
```

**Option B**: Use a compile-time define (simpler)
```c
// At top of int.c, add:
#define USE_INTERNAL_REF 0  // Set to 1 for internal, 0 for external

// In Init_ADC():
void Init_ADC(void)
{
    // Set ADCA reference voltage
    #if USE_INTERNAL_REF
        SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
    #else
        SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);
    #endif
    // ... rest of function
}
```

**Option C**: Comment/uncomment (simplest for testing)
Just change line 111 in `int.c`:
```c
// For external reference (current):
SetVREF(ADC_ADCA, ADC_EXTERNAL, ADC_VREF3P3);

// For internal reference (change to):
SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);
```

---

## 🧪 Testing Checklist

After making the changes:

- [ ] **Task 1**: Verify max/min values are being tracked (use debugger to watch `adcResultMin` and `adcResultMax`)
- [ ] **Task 1**: Test with external reference, record max/min values
- [ ] **Task 1**: Test with internal reference, record max/min values  
- [ ] **Task 1**: Compare the results between internal and external reference
- [ ] **Task 2**: Install 330pF capacitor on board (hardware change)
- [ ] **Task 2**: Compare ADC results with and without filter
- [ ] **Task 3**: Verify PWM duty cycle changes with ADC input (0V → 0%, 3.3V → 100%)
- [ ] **Interrupt**: Verify interrupt is firing (set breakpoint in `EPWM1_ISR`)

---

## 📊 How to View Results

### Using Debugger (Code Composer Studio):
1. Add `adcResultMin` and `adcResultMax` to watch window
2. Set breakpoint in `EPWM1_ISR()` to verify it's running
3. Monitor `AdcaResultRegs.ADCRESULT3` to see current ADC value
4. Monitor `EPwm1Regs.CMPA.bit.CMPA` to see PWM duty cycle

### Using Oscilloscope:
1. Connect oscilloscope to GPIO0 (EPWM1A output)
2. Apply different voltages to ADCIN3 (0V, 1.65V, 3.3V)
3. Verify duty cycle changes proportionally

---

## 🎯 Summary

**Minimum changes required**:
1. ✅ Add max/min global variables
2. ✅ Add max/min tracking in ISR
3. ✅ Register interrupt handler in main.c

**Recommended for Task 1 completion**:
4. ✅ Make reference voltage configurable (or test both manually)

**Task 2** (Hardware):
- Install 330pF capacitor on board (no code changes needed)

**Task 3**:
- ✅ Already implemented!

