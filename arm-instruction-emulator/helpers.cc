#include "helpers.h"
void undefined() {
    printf("UNDEFINED\n");
}

void reserved_value() {
    printf("RESERVED VALUE\n");
}

uint64_t ones(uint64_t len) {
    if (len == 64) return ~0;
    else return ((uint64_t(1) << (len)) - 1);
}

uint64_t bit_range(uint64_t instruction, int b, int e) {
    return (instruction >> b) & ones(e - b + 1);
}

uint64_t bit_range(uint64_t instruction, int i) {
    return bit_range(instruction, i, i);
}

uint64_t sign_extend(uint64_t n, const int & len) {
    return ((uint64_t(1) << (len - 1)) & n) ? n | ~ones(len) : n;
}

uint32_t merge(uint8_t *instructions, const int & len) {
    uint32_t merged_instruction = 0;
    for (int i = 0; i < len; i++)
        merged_instruction = merged_instruction << 8 | instructions[i];
    return merged_instruction;
}

pair < uint64_t, uint64_t > add_with_carry(uint64_t x, uint64_t y, uint64_t carry) {
    uint64_t unsigned_sum = uint64_t(x) + uint64_t(y) + uint64_t(carry);
    int64_t signed_sum = int64_t(x) + int64_t(y) + int64_t(carry);
    uint64_t result = unsigned_sum;
    int n = bit_range(result, 63);
    int z = !result;
    int c = unsigned_sum < y || unsigned_sum < x;
    int v = (int64_t(x) < 0 && int64_t(y) < 0 && signed_sum >= 0) ||
            (int64_t(x) >= 0 && int64_t(y) >= 0 && signed_sum < 0);
    uint64_t nzcv = (n << 3) | (z << 2) | (c << 1) | v;
    return {result, nzcv};
}

pair < uint32_t, uint32_t > add_with_carry(uint32_t x, uint32_t y, uint32_t carry) {
    uint32_t unsigned_sum = uint32_t(x) + uint32_t(y) + uint32_t(carry);
    int32_t signed_sum = int32_t(x) + int32_t(y) + int32_t(carry);
    uint32_t result = unsigned_sum;
    int n = bit_range(result, 63);
    int z = !result;
    int c = unsigned_sum < y || unsigned_sum < x;
    int v = (int32_t(x) < 0 && int32_t(y) < 0 && signed_sum >= 0) ||
            (int32_t(x) >= 0 && int32_t(y) >= 0 && signed_sum < 0);
    uint32_t nzcv = (n << 3) | (z << 2) | (c << 1) | v;
    return {result, nzcv};
}

uint64_t asr(uint64_t result, uint64_t amount, uint64_t datasize) {
    uint64_t sign = (uint64_t(1) << (datasize-1)) & result;
    result = (result >> amount);
    if(sign) result |= (ones(amount) << (datasize-amount));
    return result;
}

uint64_t ror(uint64_t x, uint64_t shift, uint64_t n) {
    uint64_t m = shift % n;
    uint64_t result = (x >> m) | (x << (n - m));
    return result;
}

uint64_t shift_reg(uint64_t result, uint64_t shift, uint64_t amount, uint64_t datasize) {
    result = bit_range(result, 0, datasize-1);
    if (shift == 0b00) result <<= amount;
    if (shift == 0b01) result >>= amount;
    if (shift == 0b10) result = asr(result, amount, datasize);
    if (shift == 0b11) result = ror(result, amount, datasize);
    result = bit_range(result, 0, datasize-1);
    return result;
}

bool condition_holds(uint64_t cond, uint64_t PSTATE) {
    bool result;
    uint64_t x = bit_range(cond, 1, 3);
    int N, Z, C, V;
    V = bit_range(PSTATE, 0);
    C = bit_range(PSTATE, 1);
    Z = bit_range(PSTATE, 2);
    N = bit_range(PSTATE, 3);
    if (x == 0b000) result = Z;
    else if (x == 0b001) result = C;
    else if (x == 0b010) result = N;
    else if (x == 0b011) result = V;
    else if (x == 0b100) result = C && !Z;
    else if (x == 0b101) result = N == V;
    else if (x == 0b110) result = (N == V) && !Z;
    else if (x == 0b111) result = true;
    if ((cond & 1) && cond != 0b1111) result = !result;
    return result;
}

int highest_set_bit(uint64_t x) {
    for (int i = 63; i >= 0; i--)
        if (x & uint64_t(1) << i) return i;
    return -1;
}

uint64_t replicate(uint64_t x, uint64_t len) {
    uint64_t ans = 0, t = 64 / len;
    for (uint64_t i = 0; i < t; i++)
        ans = (ans << len) | x;
    return ans;
}

pair<uint64_t, uint64_t> decode_bitmasks(uint64_t immN, uint64_t imms, uint64_t immr, bool immediate) {
    uint64_t tmask, wmask, levels, len, S, R, diff, d, welen, telen, esize;
    len = highest_set_bit((immN << 6) | bit_range(~imms, 0, 5));
    if (len < 1) undefined();
    levels = ones(len);
    if (immediate && (imms & levels) == levels) undefined();

    S = imms & levels;
    R = immr & levels;
    diff = S - R;
    d = bit_range(diff, 0, len - 1);

    esize = 1 << len;
    welen = ones(S + 1);
    telen = ones(d + 1);
    wmask = replicate(ror(welen, R, esize), esize);
    tmask = replicate(telen, esize);
    return {wmask, tmask};
}
