#ifndef CORE_HPP
#define CORE_HPP

#include <common.hpp>
#include <register.hpp>
#include <string>
#include <vector>
#include <array>
#include <fstream>

using GPRegister = Register<u64>;

constexpr u64 memOffset = 0x100000;

class Core {
private:
    static inline u32 idx = 0;
    static std::ifstream stream;
    static std::vector<std::byte> memory;
    static std::array<std::byte, 32768> romMemory;
    enum class ExceptionLevel {
        EL0,
        EL1,
        EL2,
        EL3
    } exceptionLevel;

    u64 getReg(u8 idx, bool isSp = false);
    void setReg(u8 idx, u64 val, bool isSp = false);

    void executeBranch(u32 raw);
    void executeLoadStore(u32 raw);
    void executeMove(u32 raw);
    void executeArithmetic(u32 raw);
    void executeBitfield(u32 raw);
    void executeSystem(u32 raw);
    void executeSIMD(u32 raw);
    void executeCSelection(u32 raw);

public:
    std::vector<GPRegister> regs;
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
    const SpecialRegister& currentSP() const;
    void execute();
    void dumpRegisters() const;
    void dumpMemory(size_t start = 0, size_t end = size_t(-1)) const;
};

#endif