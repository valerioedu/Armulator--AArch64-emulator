#include <common.hpp>
#include <register.hpp>
#include <core.hpp>

using namespace std;

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