//
// Created by Christopher Yarp on 2019-03-27.
//

#include <iostream>
#include <string>
#include <memory>
#include <cstdint>

int main(int argc, char* argv[]) {
    void *ptr;
    std::cout << "sizeof(ptr):                " << sizeof(ptr) << std::endl;
    std::cout << "sizeof(size_t):             " << sizeof(size_t) << std::endl;
    std::cout << std::endl;
    std::cout << "sizeof(bool):               " << sizeof(bool) << std::endl;
    std::cout << "sizeof(char):               " << sizeof(char) << std::endl;
    std::cout << "sizeof(short):              " << sizeof(short) << std::endl;
    std::cout << "sizeof(int):                " << sizeof(int) << std::endl;
    std::cout << "sizeof(long):               " << sizeof(long) << std::endl;
    std::cout << "sizeof(long long):          " << sizeof(long long) << std::endl;
    std::cout << "sizeof(float):              " << sizeof(float) << std::endl;
    std::cout << "sizeof(double):             " << sizeof(double) << std::endl;
    std::cout << std::endl;
    std::cout << "sizeof(signed char):        " << sizeof(signed char) << std::endl;
    std::cout << "sizeof(signed short):       " << sizeof(signed short) << std::endl;
    std::cout << "sizeof(signed int):         " << sizeof(signed int) << std::endl;
    std::cout << "sizeof(signed long):        " << sizeof(signed long) << std::endl;
    std::cout << "sizeof(signed long long):   " << sizeof(signed long long) << std::endl;
    std::cout << std::endl;
    std::cout << "sizeof(unsigned char):      " << sizeof(unsigned char) << std::endl;
    std::cout << "sizeof(unsigned short):     " << sizeof(unsigned short) << std::endl;
    std::cout << "sizeof(unsigned int):       " << sizeof(unsigned int) << std::endl;
    std::cout << "sizeof(unsigned long):      " << sizeof(unsigned long) << std::endl;
    std::cout << "sizeof(unsigned long long): " << sizeof(unsigned long long) << std::endl;
    std::cout << std::endl;
    std::cout << "sizeof(int8_t):             " << sizeof(int8_t) << std::endl;
    std::cout << "sizeof(int16_t):            " << sizeof(int16_t) << std::endl;
    std::cout << "sizeof(int32_t):            " << sizeof(int32_t) << std::endl;
    std::cout << "sizeof(int64_t):            " << sizeof(int64_t) << std::endl;
    std::cout << std::endl;
    std::cout << "sizeof(uint8_t):            " << sizeof(uint8_t) << std::endl;
    std::cout << "sizeof(uint16_t):           " << sizeof(uint16_t) << std::endl;
    std::cout << "sizeof(uint32_t):           " << sizeof(uint32_t) << std::endl;
    std::cout << "sizeof(uint64_t):           " << sizeof(uint64_t) << std::endl;
}