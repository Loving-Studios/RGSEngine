#pragma once

#include <random>
#include <cstdint>

class UIDGenerator {
public:
    static uint64_t GenerateUID() {
        static std::mt19937_64 gen(std::random_device{}());
        static std::uniform_int_distribution<uint64_t> dis;

        return dis(gen);
    }
};