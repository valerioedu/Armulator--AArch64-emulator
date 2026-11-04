#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <print>
#include <utility>
#include <cstring>

using u8  = uint8_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u64 = uint64_t;
using i64 = int64_t;

using namespace std;

constexpr u64 memOffset = 0x100000;

u32 extractBits(u32 inst, u8 high, u8 low) {
    u8 count = high - low + 1;
    if (count >= 32) return inst >> low;
    return (inst >> low) & ((1u << count) - 1);
}

i64 signExtend(u32 val, u32 bits) {
	u32 shift = 64 - bits;
	return (i64(val) << shift) >> shift;
}

class Register {
protected:
    u64 value;
    u8 idx;

public:
    Register(u8 id, u64 val = 0) : value(val), idx(id) { }

    virtual ~Register() = default;

    void setValue(u64 val, bool isSp = false) {
        if (idx == 31 && isSp == false) [[unlikely]] {
            return;
        }

        value = val;
    }

    u64 getValue(bool isSp = false) const {
        if (idx == 31 && isSp == false) [[unlikely]] {
            return 0;
        }

        return value;
    }

    virtual string getName() const {
        return "x" + to_string(idx);
    }
};

class SpecialRegister : public Register {
private:
    string name;

public:
    SpecialRegister(u8 id, string nm = "", u64 val = 0) : Register(id, val), name(nm) { }

    string getName() const override {
        return name;
    }
};

class Core {
private:
    static inline u32 idx = 0;
    static ifstream stream;
    static vector<byte> memory;
    static array<byte, 32768> romMemory;
    enum class ExceptionLevel {
        EL0,
        EL1,
        EL2,
        EL3
    } exceptionLevel;

public:
    vector<Register> regs;
    SpecialRegister pc{33, "pc", 0};
    SpecialRegister NZCV{33,"NZCV", 0};
    SpecialRegister RVBAR_EL3{33, "RVBAR_EL3", 0};
    SpecialRegister SP_EL[4]{{33, "SP_EL0", 0}, {33, "SP_EL1", 0}, {33, "SP_EL2", 0}, {33, "SP_EL3", 0}};

    Core(/*const string &filepath, */size_t memSize = 268'435'456) /*: stream(filepath, ios::binary | ios::in)*/ {
        regs.reserve(32);
        for (int i = 0; i < 32; i++) {
            regs.emplace_back(i, 0);
        }

        memory.resize(memSize, byte{0});
        memory[0x2000] = byte{42};
        RVBAR_EL3.setValue(u64(&romMemory[0]));
        exceptionLevel = ExceptionLevel::EL3;
        idx++;
    }

    ~Core() {
        regs.clear();
        if (--idx == 0) {
            memory.clear();
        }
    }

    template <typename T>
    T read(u64 addr) const {
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
    void write(u64 addr, T val) {
        if (addr + sizeof(T) <= memory.size() && addr + sizeof(T) >= addr) [[likely]] {
            memcpy(&memory[addr], &val, sizeof(T));
        } else if (addr >= memOffset && (addr - memOffset + sizeof(T)) <= memory.size()) {
            memcpy((&memory[addr - memOffset]), &val, sizeof(T));
        }
    }

    SpecialRegister& currentSP() {
        return SP_EL[to_underlying(exceptionLevel)];
    }

    u64 getReg(u8 idx, bool isSp = false) {
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

    void setReg(u8 idx, u64 val, bool isSp = false) {
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

    void execute() {
        u32 raw = read<u32>(pc.getValue());
        if (extractBits(raw, 30, 26) == 0b00101) {
            u32 op = extractBits(raw, 31, 31);
            u32 imm26 = extractBits(raw, 25, 0);
            i64 offset = signExtend(imm26, 26) * 4;
            u64 target = u64(i64(pc.getValue()) + offset);

            if (op == 1) {
                regs[30].setValue(pc.getValue() + 4);
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

            if (op == 1 && s == 1 && rd == 31) {
                u32 value = 0;
                u64 rnVal = regs[rn].getValue();
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
            } else if (op == 0 && s == 0 && imm12 == 0 && (rd == 31 || rn == 31)) {
                regs[rd].setValue(regs[rn].getValue());
                pc.setValue(pc.getValue() + 4);
                return;
            }

            u64 res = (op == 1) ? (regs[rn].getValue() - imm12)
                                : (regs[rn].getValue() + imm12);

            if (!sf) [[unlikely]] {
                res &= 0xFFFF'FFFFULL;
            }

            regs[rd].setValue(res);

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
                if (opc == 1) {
                    u64 r{};
                    if (size == 3) [[likely]] {
                        r = read<u64>(u64(regs[rn].getValue() + offset));
                    } else if (size == 2) {
                        r = read<u32>(u64(regs[rn].getValue() + offset));
                    } else if (size == 1) {
                        r = read<u16>(u64(regs[rn].getValue() + offset));
                    } else if (size == 0) {
                        r = read<u8>(u64(regs[rn].getValue() + offset));
                    }

                    if (rt != 31) [[likely]] {
                        regs[rt].setValue(r);
                    }
                } else {
                    u64 rtVal = (rt == 31) ? 0ULL : regs[rt].getValue();
                    u64 addr = u64(regs[rn].getValue() + offset);
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

    void dumpRegisters() const {
        cout << hex << showbase << endl;
        for (int i = 0; i < 32; i += 2) {
            println("{:<4}: {:#x}\t{:<4}: {:#x}",
                 regs[i].getName(), regs[i].getValue(),
                 regs[i + 1].getName(), regs[i + 1].getValue());
        }

        cout << endl << "PC: " << pc.getValue() << endl;

        cout << dec << noshowbase;
    }

    void dumpMemory(size_t start = 0, size_t end = size_t(-1)) const {
        if (end == size_t(-1)) {
            end = memory.size();
        }

        for (size_t i = start; i < end; i++) {
            println("{:<#04x}: {:#04x}", i, std::to_underlying(memory[i]));
        }
    }
};

ifstream            Core::stream{};
vector<byte>        Core::memory;
array<byte, 32768>  Core::romMemory;

int main(int argc, char *argv[]) {
    Core core{};
    core.pc.setValue(memOffset);
    core.write<u32>(memOffset, 0x9101E128);
    core.write<u32>(memOffset + (sizeof(u32)), 0x9100020F);
    core.write<u32>(memOffset + (sizeof(u32) * 2), 0x9100020F);
    core.write<u32>(memOffset + (sizeof(u32) * 3), 0x9100020F);
    core.regs[9].setValue(1234);
    core.regs[16].setValue(64);
    core.regs[15].setValue(0x2000);
    core.regs[12].setValue(42);
    for (int i = 0; i < 4; i++) {
        core.execute();
        core.dumpRegisters();
    }

    core.dumpMemory(0x1FFD, 0x2010);
    return 0;
}