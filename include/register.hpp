#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <common.hpp>
#include <string>
#include <bit>

template <typename T>
class Register {
protected:
    T value;
    u8 idx;

public:
    Register(u8 id, T val = 0) : value(val), idx(id) { }
    virtual ~Register() = default;

    void setValue(T val) {
        value = val;
    }

    T getValue() const {
        return value; 
    }

    virtual std::string getName() const {
        return "x" + std::to_string(idx);
    }
};

class SpecialRegister : public Register<u64> {
private:
    std::string name;

public:
    SpecialRegister(u8 id, std::string nm = "", u64 val = 0) : Register(id, val), name(nm) { }

    string getName() const override {
        return name;
    }
};

class VectorRegister : public Register<u128> {
private:
    u8 prefix; // 0 -> double, 1 -> vector, 2 -> SVE

public:
    VectorRegister(u8 id, u128 val = 0, u8 mod = 2) : Register(id, val), prefix(mod) { }

    std::string getName() const override {
        if (prefix == 0) {
            return "d" + to_string(idx);
        } else if (prefix == 1) {
            return "v" + to_string(idx);
        } else {
            return "z" + to_string(idx);
        }
    }

    double getValue() const;
    void setValue(double val);
};

class PredicateRegister : public Register<u16> {
public:
    PredicateRegister(u8 id, u16 val = 0) : Register(id, val) { }

    std::string getName() const override {
        return "p" + std::to_string(idx);
    }
};

#endif