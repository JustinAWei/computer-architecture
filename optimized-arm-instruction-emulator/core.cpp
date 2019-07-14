#include "core.h"
#include <functional>
function <void()> cache[memsize + 1];
bool cached[memsize + 1];
#include "helpers.h"

inline void undefined() {
    printf("UNDEFINED\n");
}

inline void reserved_value() {
    printf("RESERVED VALUE\n");
}

inline uint64_t ones(uint64_t len) {
    if (len == 64) return ~0;
    else return ((uint64_t(1) << (len)) - 1);
}

inline uint64_t bits(uint64_t instruction, int b, int e) {
    return (instruction >> b) & ones(e - b + 1);
}

inline uint64_t bits(uint64_t instruction, int i) {
    return bits(instruction, i, i);
}

inline uint64_t sign_extend(uint64_t n,
    const int & len) {
    return ((uint64_t(1) << (len - 1)) & n) ? n | ~ones(len) : n;
}

template < typename U, typename S > pair < U, U > add_with_carry(U x, U y, int carry) {
    U unsigned_sum = U(x) + U(y) + U(carry);
    S signed_sum = S(x) + S(y) + S(carry);
    int n = bits(unsigned_sum, 63);
    int z = !unsigned_sum;
    int c = unsigned_sum < y || unsigned_sum < x;
    int v = (S(x) < 0 && S(y) < 0 && signed_sum >= 0) ||
        (S(x) >= 0 && S(y) >= 0 && signed_sum < 0);
    U nzcv = (n << 3) | (z << 2) | (c << 1) | v;
    return {
        unsigned_sum,
        nzcv
    };
}

template < typename T > T asr(T result, T amount, int datasize) {
    T sign = (uint64_t(1) << (datasize - 1)) & result;
    result = (result >> amount);
    if (sign) result |= (ones(amount) << (datasize - amount));
    return result;
}

template < typename T > T ror(T x, T shift, T n) {
    T m = shift % n;
    T result = (x >> m) | (x << (n - m));
    return result;
}

template < typename T > T shift_reg(T result, T shift, T amount, int datasize) {
    if (shift == 0b00) result <<= amount;
    if (shift == 0b01) result >>= amount;
    if (shift == 0b10) result = asr < T > (result, amount, datasize);
    if (shift == 0b11) result = ror < T > (result, amount, datasize);
    return result;
}

inline bool condition_holds(uint64_t cond, uint64_t PSTATE) {
    bool result;
    uint64_t x = bits(cond, 1, 3);
    int N, Z, C, V;
    V = bits(PSTATE, 0);
    C = bits(PSTATE, 1);
    Z = bits(PSTATE, 2);
    N = bits(PSTATE, 3);
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

inline int highest_set_bit(uint64_t x) {
    for (int i = 63; i >= 0; i--)
        if (x & uint64_t(1) << i) return i;
    return -1;
}

inline uint64_t replicate(uint64_t x, uint64_t len) {
    uint64_t ans = 0, t = 64 / len;
    for (uint64_t i = 0; i < t; i++)
        ans = (ans << len) | x;
    return ans;
}

inline pair < uint64_t, uint64_t > decode_bitmasks(uint64_t immN, uint64_t imms, uint64_t immr, bool immediate) {
    uint64_t tmask, wmask, levels, len, S, R, diff, d, welen, telen, esize;
    len = highest_set_bit((immN << 6) | bits(~imms, 0, 5));
    if (len < 1) undefined();
    levels = ones(len);
    if (immediate && (imms & levels) == levels) undefined();

    S = imms & levels;
    R = immr & levels;
    diff = S - R;
    d = bits(diff, 0, len - 1);

    esize = 1 << len;
    welen = ones(S + 1);
    telen = ones(d + 1);
    wmask = replicate(ror(welen, R, esize), esize);
    tmask = replicate(telen, esize);
    return {wmask, tmask};
}

core::core(uint64_t entry, uint64_t core_num): active(true), PC(entry), SP(0), PSTATE(0) {
    X[30] = core_num;
}

inline void core::MOVN() {
    uint64_t d, imm16, hw, pos, result;
    d = bits(instruction, 0, 4);
    hw = bits(instruction, 21, 22);
    imm16 = bits(instruction, 5, 20);
    pos = hw << 4;
    result = ~(imm16 << pos);
    if (PC < memsize && !cached[PC]) {
        cached[PC] = true;
        cache[PC] = [d, result, this]() {
            X[d] = result;
            PC += 4;
        };
    }
    else {
        X[d] = result;
        PC += 4;
    }
}

inline void core::STR_UOFF() {
    uint64_t n, t, scale, offset, imm12, datasize, *address_ptr, *data_ptr;
    scale = bits(instruction, 30, 31);
    imm12 = bits(instruction, 10, 21);
    offset = imm12 << scale;
    t = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    datasize = 8 << scale;
    if (n == 31) address_ptr = & SP;
    else address_ptr = & X[n];
    data_ptr = & X[t];
    if (PC < memsize && !cached[PC]) {
        cached[PC] = true;
        if (datasize == 64) {
            cache[PC] = [offset, address_ptr, data_ptr, this]() {
                uint64_t address = *address_ptr + offset;

                uint32_t *dest = (address < memsize) ? & MEM[address] : & VMEM[address];
                *dest = bits(*data_ptr, 0, 31);

                dest = (address + 4 < memsize) ? & MEM[address + 4] : & VMEM[address + 4];
                *dest = bits(*data_ptr, 32, 63);
                PC += 4;
            };
        } else {
            cache[PC] = [offset, address_ptr, data_ptr, this]() {
                uint64_t address = *address_ptr + offset;
                uint32_t *dest = (address < memsize) ? & MEM[address] : & VMEM[address];
                *dest = bits(*data_ptr, 0, 31);
                PC += 4;
            };
        }
    }
    else {
        if (datasize == 64) {
                uint64_t address = *address_ptr + offset;

                uint32_t *dest = (address < memsize) ? & MEM[address] : & VMEM[address];
                *dest = bits(*data_ptr, 0, 31);

                dest = (address + 4 < memsize) ? & MEM[address + 4] : & VMEM[address + 4];
                *dest = bits(*data_ptr, 32, 63);
                PC += 4;
        } else {
                uint64_t address = *address_ptr + offset;
                uint32_t *dest = (address < memsize) ? & MEM[address] : & VMEM[address];
                *dest = bits(*data_ptr, 0, 31);
                PC += 4;
        }
    }
}

bool core::execute() {
    instruction = (PC < memsize) ? MEM[PC] : VMEM[PC];
    // cout << hex << PC << " " << instruction << endl;
    if (PC < memsize && cached[PC]) {
        cache[PC]();
        return true;
    }
    uint64_t id = bits(instruction, 24, 28);
    switch (id) {
    case 0b11001:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_LDRBUO) == instruction && (neg | MASK_LDRBUON) == neg) LDRB_UOFF();
            else if ((instruction | MASK_LDRI) == instruction && (neg | MASK_LDRIN) == neg) LDR_UOFF();
            else if ((instruction | MASK_STR) == instruction && (neg | MASK_STRN) == neg) STR_UOFF();
            else STRB();
            break;
        }
    case 0b11000:
        {
            LDRB();
            break;
        }
    case 0b10101:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_B) == instruction && (neg | MASK_BN) == neg) B();
            else CBNZ();
            break;
        }
    case 0b10100:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_BNE) == instruction && (neg | MASK_BNEN) == neg) BCOND();
            else if ((instruction | MASK_CBZ) == instruction && (neg | MASK_CBZN) == neg) CBZ();
            else B();
            break;
        }
    case 0b10110:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_RET) == instruction && (neg | MASK_RETN) == neg) return false;
            else B();
        }
    case 0b10111:
        {
            B();
        }
    case 0b11011:
        {
            MADD();
            break;
        }
    case 0b01010:
        {
            ORR();
            break;
        }
    case 0b01011:
        {
            ADD_SHIFTED();
            break;
        }
    case 0b10001:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_ADDI) == instruction && (neg | MASK_ADDIN) == neg) ADD();
            else if ((instruction | MASK_SUB) == instruction && (neg | MASK_SUBN) == neg) SUB();
            else SUBS();
            break;
        }
    case 0b10000:
        {
            ADRP();
            break;
        }
    case 0b10010:
        {
            uint64_t neg = ~instruction;
            if ((instruction | MASK_AND) == instruction && (neg | MASK_ANDN) == neg) AND();
            else if ((instruction | MASK_MOVWI) == instruction && (neg | MASK_MOVWIN) == neg) MOV();
            else if ((instruction | MASK_MOVIWI) == instruction && (neg | MASK_MOVIWIB) == neg) MOVN();
            break;
        }
    case 0b10011:
        {
            UBFM();
            break;
        }
    default:
        return false;
    }
    return true;
}

inline void core::SUB() {
    uint64_t d, n, shift, op2, imm = 0, imm12, sf, *data_ptr, *dest_ptr;
    sf = bits(instruction, 31);
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    imm12 = bits(instruction, 10, 21);
    shift = bits(instruction, 22, 23);
    if (shift == 0b00) imm = imm12;
    else if (shift == 0b01) imm = imm12 << 12;
    else reserved_value();
    op2 = ~imm;
    data_ptr = n == 31 ? & SP : & X[n];
    if (d == 31) dest_ptr = & SP;
    else dest_ptr = & X[d];
    uint64_t val = op2 + 1;
    if (sf) {
        cache[PC] = [data_ptr, dest_ptr, val, this]() {
            *dest_ptr = *data_ptr + val;
            PC += 4;
        };
    } else {
        cache[PC] = [data_ptr, dest_ptr, val, this]() {
            *dest_ptr = uint32_t(*data_ptr + val);
            PC += 4;
        };
    }
    cached[PC] = true;
}

inline void core::CBZ() {
    uint64_t t, offset, imm19;
    t = bits(instruction, 0, 4);
    imm19 = bits(instruction, 5, 23);
    const int len = 21;
    offset = sign_extend(imm19 << 2, len);

    if (!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [t, offset, this]() {
            if (!X[t]) PC += offset;
            else PC += 4;
        };
    }
}

inline void core::ADRP() {
    uint64_t d, immhi, immlo, imm, base;
    d = bits(instruction, 0, 4);
    immhi = bits(instruction, 5, 23);
    immlo = bits(instruction, 29, 30);

    const int len_immhi_immlo_zeros12 = 33;

    imm = sign_extend(((immhi << 2) | immlo) << 12, len_immhi_immlo_zeros12);

    base = PC & ~(0xFFF);
    uint64_t result = base + imm;
    if (!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [this, d, result]() {
            X[d] = result;
            PC += 4;
        };
    }
}

inline void core::ADD() {
    uint64_t d, n, shift, imm12, imm=0;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    shift = bits(instruction, 22, 23);
    imm12 = bits(instruction, 10, 21);

    if (!shift) imm = imm12;
    else if (shift == 1) imm = imm12 << 12;
    else reserved_value();

    if (!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [d,n, imm, this]() {
            uint64_t operand1 = n == 31 ? SP : X[n];
            uint64_t result = operand1 + imm;
            if (d == 31) SP = result;
            else X[d] = result;
            PC += 4;
        };
    }
}

inline void core::MOV() {
    MOVZ();
}

inline void core::MOVZ() {
    uint64_t d, hw, sf, pos, imm16, result;
    d = bits(instruction, 0, 4);
    hw = bits(instruction, 21, 22);
    sf = bits(instruction, 31);
    if (!sf && bits(hw, 1)) undefined();
    pos = hw << 4;
    imm16 = bits(instruction, 5, 20);
    result = imm16 << pos;
    if (!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [d, result, this]() {
                X[d] = result;
                PC += 4;
            };
        }
        else {
            cache[PC] = [d, result, this]() {
                X[d] = uint32_t(result);
                PC += 4;
            };
        }
    }
}

inline void core::STRB() {
    uint64_t t, n, offset;
    t = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    offset = bits(instruction, 10, 21);
    if (!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [t, n, offset, this]() {
            uint8_t data;
            uint64_t address;
            if (n == 31) address = SP;
            else address = X[n];
            address += offset;
            data = X[t];
            int off = 0;
            uint64_t addr32 = address;
            while (addr32 % 4 != 0) {
                addr32--;
                off++;
            }
            uint32_t *dest = (addr32 < memsize) ? & MEM[addr32] : & VMEM[addr32];
            *dest = (*dest & ~(0xFF << ((3 - off) *8))) | data << ((3 - off) *8);
            if (address == magic_address) printf("%c", data);
            PC += 4;
        };
    }
}

inline void core::LDR_UOFF() {
    uint64_t imm12, offset, scale;
    scale = bits(instruction, 30, 31);
    imm12 = bits(instruction, 10, 21);
    offset = imm12 << scale;
    LDR(false, false, scale, offset);
}

inline void core::LDR_SIGNED() {
    uint64_t imm9, size, scale, offset;
    scale = size = bits(instruction, 30, 31);
    imm9 = bits(instruction, 12, 20);
    const int len = 9;
    offset = sign_extend(imm9, len);
    LDR(true, bits(instruction, 10, 11) == 0b01, scale, offset);
}

inline void core::LDR(bool wback, bool postindex, uint64_t scale, uint64_t offset) {
    uint64_t t, n, datasize;
    t = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    if (wback && t == n && n != 31) undefined();

    datasize = 8 << scale;
    if (!cached[PC]) {
        if (datasize == 32) {
            cache[PC] = [n, t, offset, wback, postindex, this]() {
                uint64_t address;
                if (n == 31) address = SP;
                else address = X[n];
                if (!postindex) address += offset;
                uint32_t *dest = (address < memsize) ? & MEM[address] : & VMEM[address];
                X[t] = *dest;
                if (wback) {
                    if (postindex) address += offset;
                    if (n == 31) SP = address;
                    else X[n] = address;
                }
                PC += 4;

            };
        } else {

            cache[PC] = [n, t, offset, wback, postindex, this]() {
                uint64_t address;
                if (n == 31) address = SP;
                else address = X[n];

                if (!postindex) address += offset;
                uint32_t *lower_bits_ptr = (address < memsize) ? & MEM[address] : & VMEM[address];
                uint32_t *upper_bits_ptr = (address + 4 < memsize) ? & MEM[address + 4] : & VMEM[address + 4];
                X[t] = (uint64_t(*upper_bits_ptr) << 32) | *lower_bits_ptr;
                if (wback) {
                    if (postindex) address += offset;
                    if (n == 31) SP = address;
                    else X[n] = address;
                }
                PC += 4;
            };
        }
        cached[PC] = true;
    }
}

inline void core::LDRB() {
    uint64_t t, n, imm9, offset;
    t = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    imm9 = bits(instruction, 12, 20);
    const int len_imm9 = 9;

    offset = sign_extend(imm9, len_imm9);
    if(!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [n, t, offset, this]() {
            uint64_t address;
            if (n == 31) address = SP;
            else address = X[n];
            address += offset;
            uint64_t addr = address;
            int off = 0;
            while (addr % 4 != 0) {
                addr--;
                off++;
            }
            uint32_t *ptr = (addr < memsize) ? & MEM[addr] : & VMEM[addr];
            X[t] = bits(*ptr, (off) *8, (off) *8 + 7);
            X[n] = address;
            PC += 4;
        };
    }
}

inline void core::CBNZ() {
    uint64_t sf, t, imm19, datasize, offset, *data_ptr;
    sf = bits(instruction, 31);
    t = bits(instruction, 0, 4);
    imm19 = bits(instruction, 5, 23);
    datasize = sf ? 64 : 32;
    const int len_imm19_00 = 21;
    offset = sign_extend(imm19 << 2, len_imm19_00);
    data_ptr = & X[t];
    if (!cached[PC]) {
        cached[PC] = true;
        if (datasize == 64) {
            cache[PC] = [data_ptr, offset, this]() {
                if (uint64_t(*data_ptr)) PC += offset;
                else PC += 4;
            };
        } else {
            cache[PC] = [data_ptr, offset, this]() {
                if (uint32_t(*data_ptr)) PC += offset;
                else PC += 4;
            };
        }
    }
}

inline void core::SUBS() {
    uint64_t d, n, imm12, imm = 0, shift, sf, operand2;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    imm12 = bits(instruction, 10, 21);
    shift = bits(instruction, 22, 23);
    sf = bits(instruction, 31);

    if (shift == 0) imm = imm12;
    else if (shift == 1) imm = imm12 << 12;
    else reserved_value();

    operand2 = ~imm;
    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [n, d, operand2, this]() {
                uint64_t operand1 = n == 31 ? SP : X[n];
                tie(X[d], PSTATE) = add_with_carry < uint64_t, int64_t > (operand1, operand2, 1);
                PC += 4;
            };
        }
        else {
            cache[PC] = [n, d, operand2, this]() {
                uint32_t operand1 = n == 31 ? SP : X[n];
                tie(X[d], PSTATE) = add_with_carry < uint32_t, int32_t > (operand1, operand2, 1);
                PC += 4;
            };
        }
    }
}

inline void core::ORR() {
    uint64_t d, n, m, shift, imm6, sf, datasize, shift_amount;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    m = bits(instruction, 16, 20);
    shift = bits(instruction, 22, 23);
    imm6 = bits(instruction, 10, 15);
    sf = bits(instruction, 31);
    datasize = sf ? 64 : 32;
    shift_amount = imm6;

    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [n,d,m, shift, shift_amount, datasize, this]() {
                X[d] = X[n] | shift_reg < uint64_t > (X[m], shift, shift_amount, datasize);;
                PC += 4;
            };
        }
        else {
            cache[PC] = [n,d,m, shift, shift_amount, datasize, this]() {
                X[d] = X[n] | shift_reg < uint32_t > (X[m], shift, shift_amount, datasize);;
                PC += 4;
            };
        }
    }
}

inline void core::B() {
    uint64_t imm26, offset;
    imm26 = bits(instruction, 0, 25);
    const int len = 28;
    offset = sign_extend(imm26 << 2, len);

    if(!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [offset, this]() {
            PC += offset;
        };
    }
}

inline void core::AND() {
    uint64_t d, n, imm, N, imms, immr, sf;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    N = bits(instruction, 22);
    imms = bits(instruction, 10, 15);
    immr = bits(instruction, 16, 21);
    sf = bits(instruction, 31);
    tie(imm, ignore) = decode_bitmasks(N, imms, immr, true);

    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [n,d, imm, this]() {
                uint64_t result = X[n] & imm;
                if(d==31) SP = result;
                else X[d] = result;
                PC+=4;
            };

        }
        else {
            cache[PC] = [n,d, imm, this]() {
                uint32_t result = X[n] & uint32_t(imm);
                if(d==31) SP = result;
                else X[d] = result;
                PC+=4;
            };

        }
    }
}

inline void core::LSR() {
    UBFM();
}

inline void core::MADD() {
    uint64_t d, n, m, a, sf;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    m = bits(instruction, 16, 20);
    a = bits(instruction, 10, 14);
    sf = bits(instruction, 31);

    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [n,m,a,d,this]() {
                X[d] = X[a] + (X[n] *X[m]);
                PC += 4;
            };
        }
        else {
            cache[PC] = [n,m,a,d,this]() {
                X[d] = uint32_t(X[a] + (X[n] *X[m]));
                PC += 4;
            };
        }
    }
}

inline void core::ADD_SHIFTED() {
    uint64_t d, n, m, imm6, shift, sf, shift_amount;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    m = bits(instruction, 16, 20);
    sf = bits(instruction, 31);
    shift = bits(instruction, 22, 23);
    imm6 = bits(instruction, 10, 15);

    if (shift == 0b11) undefined();
    if (sf == 0 && bits(imm6, 5, 5) == 1) undefined();

    shift_amount = imm6;

    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {
            cache[PC] = [d,n,m,shift, shift_amount, this]() {
                X[d] = X[n] + shift_reg < uint64_t > (X[m], shift, shift_amount, 64);
                PC += 4;
            };
        }
        else {
            cache[PC] = [d,n,m,shift, shift_amount, this]() {
                X[d] = uint32_t(X[n] + shift_reg < uint32_t > (X[m], shift, shift_amount, 32));
                PC += 4;
            };
        }
    }
}

inline void core::UBFX() {
    UBFM();
}

inline void core::UBFM() {
    uint64_t d, n, wmask, tmask, imms, immr, N, sf;
    sf = bits(instruction, 31);
    // datasize = sf ? 64 : 32;
    d = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    imms = bits(instruction, 10, 15);
    immr = bits(instruction, 16, 21);
    N = bits(instruction, 22);

    if (sf == 1 && N != 1) undefined();
    if (sf == 0 && (N != 0 || bits(immr, 5) != 0)) undefined();

    tie(wmask, tmask) = decode_bitmasks(N, imms, immr, false);

    if(!cached[PC]) {
        cached[PC] = true;
        if(sf) {

            cache[PC] = [this, d, n, immr, wmask, tmask]() {
                X[d] = (ror<uint64_t>(X[n], immr, 64) & wmask) & tmask;
                PC += 4;
            };
        }
        else {
            cache[PC] = [this, d, n, immr, wmask, tmask]() {
                X[d] = uint32_t((ror<uint32_t>(X[n], immr, 32) & wmask) & tmask);
                PC += 4;
            };
        }
    }
}

inline void core::LDRB_UOFF() {
    uint64_t imm12, n, t, offset;
    imm12 = bits(instruction, 10, 21);
    t = bits(instruction, 0, 4);
    n = bits(instruction, 5, 9);
    offset = imm12;
    if(!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [n, t, offset, this]() {
            uint64_t address;
            if (n == 31) address = SP;
            else address = X[n];
            address += offset;
            uint64_t addr = address;
            int off = 0;
            while (addr % 4 != 0) {
                addr--;
                off++;
            }
            uint32_t *ptr = (addr < memsize) ? & MEM[addr] : & VMEM[addr];
            X[t] = bits(*ptr, (off) * 8, (off) * 8 + 7);
            PC += 4;
        };
    }

}

inline void core::BCOND() {
    uint64_t imm19, cond, offset;
    imm19 = bits(instruction, 5, 23);
    cond = bits(instruction, 0, 3);
    const int len = 21;
    offset = sign_extend(imm19 << 2, len);

    if(!cached[PC]) {
        cached[PC] = true;
        cache[PC] = [cond, offset, this]() {
            if (condition_holds(cond, PSTATE)) PC += offset;
            else PC += 4;
        };
    }
}

inline void core::DUMP() {
    printf("instruction %x at %lx\n", instruction, PC);
    for (int i = 0; i < 31; i++)
        printf("X%02d : %016lX\n", i, X[i]);
    printf("XSP : %016lX\n", X[31]);
}

inline void core::RET() {
    active = false;
}
