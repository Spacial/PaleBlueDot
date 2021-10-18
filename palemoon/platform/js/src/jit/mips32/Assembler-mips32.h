/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_mips32_Assembler_mips32_h
#define jit_mips32_Assembler_mips32_h

#include "jit/mips-shared/Assembler-mips-shared.h"

#include "jit/mips32/Architecture-mips32.h"

namespace js {
namespace jit {

static constexpr Register CallTempReg4 = t4;
static constexpr Register CallTempReg5 = t5;

static constexpr Register CallTempNonArgRegs[] = { t0, t1, t2, t3, t4 };
static const uint32_t NumCallTempNonArgRegs = mozilla::ArrayLength(CallTempNonArgRegs);

class ABIArgGenerator
{
    unsigned usedArgSlots_;
    unsigned firstArgFloatSize_;
    // Note: This is not compliant with the system ABI.  The Lowering phase
    // expects to lower an MWasmParameter to only one register.
    bool useGPRForFloats_;
    ABIArg current_;

  public:
    ABIArgGenerator();
    ABIArg next(MIRType argType);
    ABIArg& current() { return current_; }

    void enforceO32ABI() {
        useGPRForFloats_ = true;
    }

    uint32_t stackBytesConsumedSoFar() const {
        if (usedArgSlots_ <= 4)
            return ShadowStackSpace;

        return usedArgSlots_ * sizeof(intptr_t);
    }
};

static constexpr Register ABINonArgReg0 = t0;
static constexpr Register ABINonArgReg1 = t1;
static constexpr Register ABINonArgReg2 = t2;
static constexpr Register ABINonArgReturnReg0 = t0;
static constexpr Register ABINonArgReturnReg1 = t1;

// TLS pointer argument register for WebAssembly functions. This must not alias
// any other register used for passing function arguments or return values.
// Preserved by WebAssembly functions.
static constexpr Register WasmTlsReg = s5;

// Registers used for asm.js/wasm table calls. These registers must be disjoint
// from the ABI argument registers, WasmTlsReg and each other.
static constexpr Register WasmTableCallScratchReg = ABINonArgReg0;
static constexpr Register WasmTableCallSigReg = ABINonArgReg1;
static constexpr Register WasmTableCallIndexReg = ABINonArgReg2;

static constexpr Register JSReturnReg_Type = a3;
static constexpr Register JSReturnReg_Data = a2;
static constexpr Register64 ReturnReg64(InvalidReg, InvalidReg);
static constexpr FloatRegister ReturnFloat32Reg = { FloatRegisters::f0, FloatRegister::Single };
static constexpr FloatRegister ReturnDoubleReg = { FloatRegisters::f0, FloatRegister::Double };
static constexpr FloatRegister ScratchFloat32Reg = { FloatRegisters::f18, FloatRegister::Single };
static constexpr FloatRegister ScratchDoubleReg = { FloatRegisters::f18, FloatRegister::Double };
static constexpr FloatRegister SecondScratchFloat32Reg = { FloatRegisters::f16, FloatRegister::Single };
static constexpr FloatRegister SecondScratchDoubleReg = { FloatRegisters::f16, FloatRegister::Double };

// Registers used in the GenerateFFIIonExit Disable Activation block.
// None of these may be the second scratch register (t8).
static constexpr Register WasmIonExitRegReturnData = JSReturnReg_Data;
static constexpr Register WasmIonExitRegReturnType = JSReturnReg_Type;

static constexpr FloatRegister f0  = { FloatRegisters::f0, FloatRegister::Double };
static constexpr FloatRegister f2  = { FloatRegisters::f2, FloatRegister::Double };
static constexpr FloatRegister f4  = { FloatRegisters::f4, FloatRegister::Double };
static constexpr FloatRegister f6  = { FloatRegisters::f6, FloatRegister::Double };
static constexpr FloatRegister f8  = { FloatRegisters::f8, FloatRegister::Double };
static constexpr FloatRegister f10 = { FloatRegisters::f10, FloatRegister::Double };
static constexpr FloatRegister f12 = { FloatRegisters::f12, FloatRegister::Double };
static constexpr FloatRegister f14 = { FloatRegisters::f14, FloatRegister::Double };
static constexpr FloatRegister f16 = { FloatRegisters::f16, FloatRegister::Double };
static constexpr FloatRegister f18 = { FloatRegisters::f18, FloatRegister::Double };
static constexpr FloatRegister f20 = { FloatRegisters::f20, FloatRegister::Double };
static constexpr FloatRegister f22 = { FloatRegisters::f22, FloatRegister::Double };
static constexpr FloatRegister f24 = { FloatRegisters::f24, FloatRegister::Double };
static constexpr FloatRegister f26 = { FloatRegisters::f26, FloatRegister::Double };
static constexpr FloatRegister f28 = { FloatRegisters::f28, FloatRegister::Double };
static constexpr FloatRegister f30 = { FloatRegisters::f30, FloatRegister::Double };

// MIPS CPUs can only load multibyte data that is "naturally"
// four-byte-aligned, sp register should be eight-byte-aligned.
static constexpr uint32_t ABIStackAlignment = 8;
static constexpr uint32_t JitStackAlignment = 8;

static constexpr uint32_t JitStackValueAlignment = JitStackAlignment / sizeof(Value);
static_assert(JitStackAlignment % sizeof(Value) == 0 && JitStackValueAlignment >= 1,
  "Stack alignment should be a non-zero multiple of sizeof(Value)");

// TODO this is just a filler to prevent a build failure. The MIPS SIMD
// alignment requirements still need to be explored.
// TODO Copy the static_asserts from x64/x86 assembler files.
static constexpr uint32_t SimdMemoryAlignment = 8;
static constexpr uint32_t WasmStackAlignment = SimdMemoryAlignment;

// Does this architecture support SIMD conversions between Uint32x4 and Float32x4?
static constexpr bool SupportsUint32x4FloatConversions = false;

// Does this architecture support comparisons of unsigned integer vectors?
static constexpr bool SupportsUint8x16Compares = false;
static constexpr bool SupportsUint16x8Compares = false;
static constexpr bool SupportsUint32x4Compares = false;

static constexpr Scale ScalePointer = TimesFour;

class Assembler : public AssemblerMIPSShared
{
  public:
    Assembler()
      : AssemblerMIPSShared()
    { }

    static Condition UnsignedCondition(Condition cond);
    static Condition ConditionWithoutEqual(Condition cond);

    // MacroAssemblers hold onto gcthings, so they are traced by the GC.
    void trace(JSTracer* trc);

    static uintptr_t GetPointer(uint8_t*);

  protected:
    // This is used to access the odd register form the pair of single
    // precision registers that make one double register.
    FloatRegister getOddPair(FloatRegister reg) {
        MOZ_ASSERT(reg.isDouble());
        return reg.singleOverlay(1);
    }

  public:
    void Bind(uint8_t* rawCode, CodeOffset* label, const void* address);

    static void TraceJumpRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader);
    static void TraceDataRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader);

    static uint32_t InstructionImmediateSize() {
        return 2 * sizeof(uint32_t);
    }
    static uint32_t ExtractLuiOriValue(Instruction* inst0, Instruction* inst1);
    static void PatchDataWithValueCheck(CodeLocationLabel label, ImmPtr newValue,
                                        ImmPtr expectedValue);
    static void PatchDataWithValueCheck(CodeLocationLabel label, PatchedImmPtr newValue,
                                        PatchedImmPtr expectedValue);

    static void PatchInstructionImmediate(uint8_t* code, PatchedImmPtr imm);
    static uint32_t ExtractInstructionImmediate(uint8_t* code);

    static void ToggleCall(CodeLocationLabel inst_, bool enabled);
}; // Assembler

static const uint32_t NumIntArgRegs = 4;

static inline bool
GetIntArgReg(uint32_t usedArgSlots, Register* out)
{
    if (usedArgSlots < NumIntArgRegs) {
        *out = Register::FromCode(a0.code() + usedArgSlots);
        return true;
    }
    return false;
}

// Get a register in which we plan to put a quantity that will be used as an
// integer argument. This differs from GetIntArgReg in that if we have no more
// actual argument registers to use we will fall back on using whatever
// CallTempReg* don't overlap the argument registers, and only fail once those
// run out too.
static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register* out)
{
    // NOTE: We can't properly determine which regs are used if there are
    // float arguments. If this is needed, we will have to guess.
    MOZ_ASSERT(usedFloatArgs == 0);

    if (GetIntArgReg(usedIntArgs, out))
        return true;
    // Unfortunately, we have to assume things about the point at which
    // GetIntArgReg returns false, because we need to know how many registers it
    // can allocate.
    usedIntArgs -= NumIntArgRegs;
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;
}

static inline uint32_t
GetArgStackDisp(uint32_t usedArgSlots)
{
    MOZ_ASSERT(usedArgSlots >= NumIntArgRegs);
    // Even register arguments have place reserved on stack.
    return usedArgSlots * sizeof(intptr_t);
}

} // namespace jit
} // namespace js

#endif /* jit_mips32_Assembler_mips32_h */