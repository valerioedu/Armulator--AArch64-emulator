#include <register.hpp>

using namespace std;

double VectorRegister::getValue() const {
    return bit_cast<double>(static_cast<u64>(value & UINT64_MAX));
}

void VectorRegister::setValue(double val) {
    value = static_cast<u128>(std::bit_cast<u64>(val));
}