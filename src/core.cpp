#include <core.hpp>
#include <utility>
#include <iostream>
#include <print>

using namespace std;

ifstream            Core::stream{};
vector<byte>        Core::memory;
array<byte, 32768>  Core::romMemory;

Core::Core(size_t memSize) {
    regs.reserve(32);
    for (int i = 0; i < 32; i++) {
        regs.emplace_back(i, 0);
    }

    if (idx == 0) {
        memory.resize(memSize, byte{0});
        memory[0x2000] = byte{42};
    }

    RVBAR_EL3.setValue(0ULL);
    exceptionLevel = ExceptionLevel::EL3;
    idx++;
}

Core::~Core() {
    regs.clear();
    if (--idx == 0) {
        memory.clear();
    }
}

template <typename T>
T Core::read(u64 addr) const {
    T val{};
    if (addr + sizeof(T) <= memory.size() && addr + sizeof(T) >= addr) [[likely]] {
        memcpy(&val, &memory[addr], sizeof(T));
    } else {
        if (addr >= memOffset && (addr - memOffset + sizeof(T)) <= memory.size()) {
            memcpy(&val, (&memory[addr - memOffset]), sizeof(T));
        } else {
            val = static_cast<T>(-1);
        }
    }

    return val;
}

template <typename T>
void Core::write(u64 addr, T val) {
    if (addr + sizeof(T) <= memory.size() && addr + sizeof(T) >= addr) [[likely]] {
        memcpy(&memory[addr], &val, sizeof(T));
    } else if (addr >= memOffset && (addr - memOffset + sizeof(T)) <= memory.size()) {
        memcpy((&memory[addr - memOffset]), &val, sizeof(T));
    }
}

SpecialRegister& Core::currentSP() {
    return SP_EL[to_underlying(exceptionLevel)];
}

const SpecialRegister& Core::currentSP() const {
    return SP_EL[to_underlying(exceptionLevel)];
}

u64 Core::getReg(u8 idx, bool isSp) {
    if (idx == 31) [[unlikely]] {
        return isSp ? currentSP().getValue() : 0ULL;
    }

    if (idx < 31) [[likely]] {
        return regs[idx].getValue();
    } else {
        //TODO: panic function
        exit(1);
        return u64(-1);
    }
}

void Core::setReg(u8 idx, u64 val, bool isSp) {
    if (idx == 31) [[unlikely]] {
        if (isSp) {
            currentSP().setValue(val);
        }

        return;
    }

    if (idx < 31) [[likely]] {
        regs[idx].setValue(val);
    }
}

void Core::executeBranch(u32 raw) {

}

void Core::executeLoadStore(u32 raw) {

}

void Core::executeMove(u32 raw) {

}

void Core::executeArithmetic(u32 raw) {

}

void Core::executeBitfield(u32 raw) {

}

void Core::executeSystem(u32 raw) {

}

void Core::executeSIMD(u32 raw) {

}

void Core::executeCSelection(u32 raw) {

}

void Core::execute() {
    u32 raw = read<u32>(pc.getValue());
    if (extractBits(raw, 30, 26) == 0b00101) {
        u32 op = extractBits(raw, 31, 31);
        u32 imm26 = extractBits(raw, 25, 0);
        i64 offset = signExtend(imm26, 26) * 4;
        u64 target = u64(i64(pc.getValue()) + offset);

        if (op == 1) {
            setReg(30, pc.getValue() + 4, false);
        }

        pc.setValue(target);
        return;
    }

    if (extractBits(raw, 28, 23) == 0b100010) {
        bool sf = extractBits(raw, 31, 31) == 1;
        u32 op = extractBits(raw, 30, 30);
        u32 s = extractBits(raw, 29, 29);
        u32 shift = extractBits(raw, 22, 22) * 12;
        u64 imm12 = extractBits(raw, 21, 10) << shift;
        u32 rn = extractBits(raw, 9, 5);
        u32 rd = extractBits(raw, 4, 0);

        u64 rnVal = getReg(rn, true);
        bool rdIsSp = (s == 0);

        if (op == 1 && s == 1 && rd == 31) {
            u32 value = 0;
            if (sf) {
                u64 res = rnVal - imm12;
                if ((res >> 63) & 1) {
                    value |= (1u << 31);
                }

                if (res == 0) {
                    value |= (1u << 30);
                }

                if (rnVal >= imm12) {
                    value |= (1u << 29);
                }

                if (((i64(rnVal) ^ i64(imm12)) & (i64(rnVal) ^ i64(res))) < 0) {
                    value |= (1u << 28);
                }
            } else {
                u32 rnVal32 = static_cast<u32>(rnVal);
                u32 imm12_32 = static_cast<u32>(imm12);
                u32 res = rnVal32 - imm12_32;
                if ((res >> 31) & 1) {
                    value |= (1u << 31);
                }

                if (res == 0) {
                    value |= (1u << 30);
                }

                if (rnVal32 >= imm12_32) {
                    value |= (1u << 29);
                }

                if (((int32_t(rnVal32) ^ int32_t(imm12_32)) & (int32_t(rnVal32) ^ int32_t(res))) < 0) {
                    value |= (1u << 28);
                }
            }

            NZCV.setValue(value);
            pc.setValue(pc.getValue() + 4);
            return;
        }

        u64 res = (op == 1) ? (rnVal - imm12) : (rnVal + imm12);
        if (!sf) [[unlikely]] {
            res &= 0xFFFF'FFFFULL;
        }

        setReg(rd, res, rdIsSp);

        pc.setValue(pc.getValue() + 4);
        return;
    }

    if (extractBits(raw, 29, 24) == 0b111001) {
        u32 size = extractBits(raw, 31, 30);
        u32 opc = extractBits(raw, 23, 22);
        u32 imm12 = extractBits(raw, 21, 10);
        u32 rn = extractBits(raw, 9, 5);
        u32 rt = extractBits(raw, 4, 0);

        if (opc <= 1) {
            u32 offset = imm12 << size;
            u64 addr = getReg(rn, true) + offset;

            if (opc == 1) {
                u64 r{};
                if (size == 3) [[likely]] {
                    r = read<u64>(addr);
                } else if (size == 2) {
                    r = read<u32>(addr);
                } else if (size == 1) {
                    r = read<u16>(addr);
                } else if (size == 0) {
                    r = read<u8>(addr);
                }

                setReg(rt, r, false);
            } else {
                u64 rtVal = getReg(rt, false);
                if (size == 3) [[likely]] {
                    write<u64>(addr, rtVal);
                } else if (size == 2) {
                    write<u32>(addr, static_cast<u32>(rtVal));
                } else if (size == 1) {
                    write<u16>(addr, static_cast<u16>(rtVal));
                } else if (size == 0) {
                    write<u8>(addr, static_cast<u8>(rtVal));
                }
            }
        }

        pc.setValue(pc.getValue() + 4);
        return;
    }
}

void Core::dumpRegisters() const {
    cout << hex << showbase << endl;
    for (int i = 0; i < 30; i += 2) {
        println("{:<4}: {:#x}\t{:<4}: {:#x}",
             regs[i].getName(), regs[i].getValue(),
             regs[i + 1].getName(), regs[i + 1].getValue());
    }

    println("{:<4}: {:#x}\t{:<4}: {:#x}",
             regs[30].getName(), regs[30].getValue(),
             currentSP().getName(), currentSP().getValue());

    cout << endl << "PC: " << pc.getValue() << endl;

    cout << dec << noshowbase;
}

void Core::dumpMemory(size_t start, size_t end) const {
    if (end == size_t(-1)) {
        end = memory.size();
    }

    for (size_t i = start; i < end; i++) {
        println("{:<#04x}: {:#04x}", i, to_underlying(memory[i]));
    }
}