#ifndef _HELPERS_H_
#define _HELPERS_H_
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;

void undefined();
void reserved_value();

uint32_t merge(uint8_t *instructions, const int &len);

uint64_t ones(uint64_t len);
uint64_t bit_range(uint64_t instruction, int b, int e);
uint64_t bit_range(uint64_t instruction, int i);
uint64_t sign_extend(uint64_t n, const int &len);


pair <uint32_t, uint32_t> add_with_carry(uint32_t x, uint32_t y, uint32_t carry);
pair <uint64_t, uint64_t> add_with_carry(uint64_t op1, uint64_t op2, uint64_t carry);

uint64_t shift_reg(uint64_t result, uint64_t shift, uint64_t amount, uint64_t datasize);
uint64_t asr(uint64_t result, uint64_t amount);
uint64_t ror(uint64_t x, uint64_t shift, uint64_t n);

bool condition_holds(uint64_t cond, uint64_t PSTATE);

int highest_set_bit(uint64_t x);
uint64_t replicate(uint64_t x, uint64_t len);
pair<uint64_t, uint64_t> decode_bitmasks(uint64_t immN, uint64_t imms, uint64_t immr, bool immediate);

#endif
