#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <common.hpp>
#include <string>

using namespace std;

class Register {
protected:
    u64 value;
    u8 idx;

public:
    Register(u8 id, u64 val = 0) : value(val), idx(id) { }
    virtual ~Register() = default;

    void setValue(u64 val, bool isSp = false);
    u64 getValue(bool isSp = false) const;

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

#endif