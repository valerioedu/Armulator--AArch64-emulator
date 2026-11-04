#ifndef CORE_HPP
#define CORE_HPP

#include <common.hpp>
#include <register.hpp>
#include <string>
#include <vector>
#include <array>
#include <fstream>

using namespace std;

constexpr u64 memOffset = 0x100000;

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

    Core(size_t memSize = 268'435'456);
    ~Core();

    template <typename T>
    T read(u64 addr) const;

    template <typename T>
    void write(u64 addr, T val);

    SpecialRegister& currentSP();
    u64 getReg(u8 idx, bool isSp = false);
    void setReg(u8 idx, u64 val, bool isSp = false);
    void execute();
    void dumpRegisters() const;
    void dumpMemory(size_t start = 0, size_t end = size_t(-1)) const;
};

#endif