#include <register.hpp>

void Register::setValue(u64 val, bool isSp) {
    if (idx == 31 && isSp == false) [[unlikely]] {
        return;
    }

    value = val;
}

u64 Register::getValue(bool isSp) const {
    if (idx == 31 && isSp == false) [[unlikely]] {
        return 0;
    }

    return value;
}