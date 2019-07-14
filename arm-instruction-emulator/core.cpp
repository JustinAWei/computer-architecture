#include "core.h"
#include "helpers.h"

core::core(uint64_t entry, uint64_t core_num): active(true), PC(entry), SP(0), PSTATE(0) {
    X[30] = core_num;
}

void core::MOVN() {
    uint64_t d, imm16, hw, pos, result;
    d = bit_range(instruction, 0, 4);
    hw = bit_range(instruction, 21, 22);
    imm16 = bit_range(instruction, 5, 20);
    pos = hw << 4;
    result = imm16 << pos;
    X[d] = ~result;
    PC += 4;
}

void core::STR_UOFF() {
    uint64_t n, t, scale, offset, imm12, address, data, datasize, k;
    scale = bit_range(instruction, 30, 31);
    imm12 = bit_range(instruction, 10, 21);
    offset = imm12 << scale;
    t = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    datasize = 8 << scale;
    if (n == 31) address = SP;
    else address = X[n];
    address += offset;
    data = X[t];

    k = datasize / 8;
    for (uint64_t i = 0; i < k; i++) {
        MEM[address + i] = bit_range(data, 0, 7);
        if (address + i == magic_address) printf("%c", MEM[magic_address]);
        data >>= 8;
    }
    PC += 4;
}

bool core::execute() {
    if (!active) return false;

    // read instruction from memory little endian
    const int len = 4;
    uint8_t separate_instructions[len] = {MEM[PC + 3], MEM[PC + 2], MEM[PC + 1], MEM[PC]};
    instruction = merge(separate_instructions, len);

    uint32_t id = bit_range(instruction, 22, 31);
    // cout << hex << instruction << endl;
    if ((id | 0b0110000011) == 0b1111000011) ADRP();
    else if ((id | 0b1000000001) == 0b1001000101) ADD();
    else if ((id | 0b1000000001) == 0b1101001011) MOV();
    else if ((id | 0b1000000001) == 0b1001001011) MOVN();
    else if ((id | 0b0100000000) == 0b1111100101) LDR_UOFF();
    else if ((id | 0b0100000000) == 0b1111100001) LDR_SIGNED();
    else if (id == 0b0011100100) STRB();
    else if ((id | 0b0100000000) == 0b1111100100) STR_UOFF();
    else if (id == 0b0011100001 /*0*/ ) LDRB();
    else if ((id | 0b1000000011) == 0b1011010111) CBNZ();
    else if ((id | 0b1000000001) == 0b1111000101) SUBS();
    else if ((id | 0b1000000000) == 0b1010101000) ORR();
    else if ((id | 0b0000001111) == 0b0001011111) B();
    else if ((id | 0b1000000001) == 0b1001001001) AND();
    else if ((id | 0b1000000001) == 0b1101001101) LSR();
    else if ((id | 0b1000000000) == 0b1001101100) MADD();
    else if ((id | 0b1000000011) == 0b1000101111) ADD_SHIFTED();
    else if ((id | 0b1000000001) == 0b1101001101) UBFX();
    else if (id == 0b0011100101) LDRB_UOFF();
    else if ((id | 0b0000000011) == 0b0101010011) BCOND();
    else RET();
    // DUMP();
    return true;
}

void core::ADRP() {
    uint64_t d, immhi, immlo, imm, base;
    d = bit_range(instruction, 0, 4);
    immhi = bit_range(instruction, 5, 23);
    immlo = bit_range(instruction, 29, 30);

    // 19 + 2 + 12 = 33
    const int len_immhi_immlo_zeros12 = 33;

    // SignExtend(immhi:immlo:Zeros(12), 64)
    imm = sign_extend(((immhi << 2) | immlo) << 12, len_immhi_immlo_zeros12);

    // clear last 12
    base = PC & ~(0xFFF);
    X[d] = base + imm;
    PC += 4;
}

void core::ADD() {
    uint64_t d, n, shift, imm12, imm, operand1, result;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    shift = bit_range(instruction, 22, 23);
    imm12 = bit_range(instruction, 10, 21);

    if (!shift) imm = imm12;
    else if (shift == 1) imm = imm12 << 12;
    else reserved_value();

    operand1 = n == 31 ? SP : X[n];
    result = operand1 + imm;
    if (d == 31) SP = result;
    else X[d] = result;
    PC += 4;
}

void core::MOV() {
    MOVZ();
}

void core::MOVZ() {
    // wide immediate
    uint64_t d, hw, sf, pos, imm16, datasize, result;
    d = bit_range(instruction, 0, 4);
    hw = bit_range(instruction, 21, 22);
    sf = bit_range(instruction, 31);
    if (!sf && bit_range(hw, 1)) undefined();
    pos = hw << 4;
    imm16 = bit_range(instruction, 5, 20);
    datasize = sf ? 64 : 32;
    result = imm16 << pos;
    X[d] = bit_range(result, 0, datasize - 1);
    PC += 4;
}

void core::STRB() {
    uint64_t t, n, imm12, offset, address;
    t = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    offset = imm12 = bit_range(instruction, 10, 21);
    uint8_t data;
    if (n == 31) address = SP;
    else address = X[n];
    address += offset;
    data = X[t];
    MEM[address] = data;
    if (address == magic_address) printf("%c", data);
    PC += 4;
}

void core::LDR_UOFF() {
    uint64_t size, imm12, offset, scale;
    scale = size = bit_range(instruction, 30, 31);
    imm12 = bit_range(instruction, 10, 21);
    offset = imm12 << scale;
    LDR(false, false, scale, offset);
}

void core::LDR_SIGNED() {
    uint64_t imm9, size, scale, offset;
    scale = size = bit_range(instruction, 30, 31);
    imm9 = bit_range(instruction, 12, 20);
    const int len = 9;
    offset = sign_extend(imm9, len);
    LDR(true, bit_range(instruction, 10, 11) == 0b01, scale, offset);
}

void core::LDR(bool wback, bool postindex, uint64_t scale, uint64_t offset) {
    uint64_t t, n, address, data=0, datasize, regsize, wb_unknown = false, size = scale;
    //unsigned offset
    t = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    if (wback && t == n && n != 31) undefined();

    datasize = 8 << scale;
    regsize = (size == 0b11) ? 64 : 32;
    if (n == 31) address = SP;
    else address = X[n];

    if (!postindex) address += offset;
    uint64_t k = datasize/8;
    for (uint64_t i = 0; i < k; i++)
        data = (data << 8) | MEM[address + k - 1 - i];
    X[t] = bit_range(data, 0, regsize - 1);
    if (wback) {
        if(wb_unknown) undefined();
        else if(postindex) address += offset;
        if (n==31) SP = address;
        else X[n] = address;
    }
    PC += 4;
}

void core::LDRB() {
    // does not implement WRITEBACK!
    // preindex
    uint64_t t, n, imm9, offset, address;
    t = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    imm9 = bit_range(instruction, 12, 20);
    const int len_imm9 = 9;

    // SignExtend(imm9)
    offset = sign_extend(imm9, len_imm9);

    uint8_t data;
    if (n == 31) address = SP;
    else address = X[n];
    address += offset;
    X[t] = data = MEM[address];
    X[n] = address;
    PC += 4;
}

void core::CBNZ() {
    uint64_t sf, t, imm19, datasize, offset, operand1;
    sf = bit_range(instruction, 31);
    t = bit_range(instruction, 0, 4);
    imm19 = bit_range(instruction, 5, 23);
    datasize = sf ? 64 : 32;
    // 19 + 2 = 21
    const int len_imm19_00 = 21;

    // SignExtend(imm19:00, 64)
    offset = sign_extend(imm19 << 2, len_imm19_00);
    operand1 = bit_range(X[t], 0, datasize - 1);
    if (operand1 != 0) PC += offset;
    else PC += 4;
}

void core::SUBS() {
    uint64_t d, n, imm12, imm, shift, sf, result, operand1, operand2, nzcv;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    imm12 = bit_range(instruction, 10, 21);
    shift = bit_range(instruction, 22, 23);
    sf = bit_range(instruction, 31);

    if (shift == 0) imm = imm12;
    else if (shift == 1) imm = imm12 << 12;
    else reserved_value();
    operand1 = n == 31 ? SP : X[n];
    operand2 = ~imm;

    if (sf) tie(result, nzcv) = add_with_carry(operand1, operand2, 1);
    else tie(result, nzcv) = add_with_carry(uint32_t(operand1), uint32_t(operand2), 1);

    PSTATE = nzcv;
    X[d] = result;
    PC += 4;
}

void core::ORR() {
    uint64_t d, n, m, shift, imm6, sf, datasize, shift_amount, operand1, operand2;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    m = bit_range(instruction, 16, 20);
    shift = bit_range(instruction, 22, 23);
    imm6 = bit_range(instruction, 10, 15);
    sf = bit_range(instruction, 31);
    datasize = sf ? 64 : 32;
    shift_amount = imm6;
    operand1 = X[n];
    operand2 = shift_reg(X[m], shift, shift_amount, datasize);

    X[d] = operand1 | operand2;
    PC += 4;
}

void core::B() {
    uint64_t imm26, offset;
    imm26 = bit_range(instruction, 0, 25);
    const int len = 28;
    offset = sign_extend(imm26 << 2, len);
    PC += offset;
}

void core::AND() {
    uint64_t d, n, imm, N, imms, immr, sf, datasize, result, operand1;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    N = bit_range(instruction, 22);
    imms = bit_range(instruction, 10, 15);
    immr = bit_range(instruction, 16, 21);
    sf = bit_range(instruction, 31);
    datasize = sf ? 64 : 32;
    tie(imm, ignore) = decode_bitmasks(N, imms, immr, true);
    imm = bit_range(imm, 0, datasize - 1);
    operand1 = X[n];
    result = operand1 & imm;
    if (d == 31) SP = result;
    else X[d] = result;
    PC += 4;
}

void core::LSR() {
    // semantics
    // lsr is special case of ubfm
    UBFM();
}

void core::MADD() {
    uint64_t d, n, m, a, destsize, sf, op1, op2, op3, result;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    m = bit_range(instruction, 16, 20);
    a = bit_range(instruction, 10, 14);
    sf = bit_range(instruction, 31);
    destsize = sf ? 64 : 32;

    op1 = bit_range(X[n], 0, destsize - 1);
    op2 = bit_range(X[m], 0, destsize - 1);
    op3 = bit_range(X[a], 0, destsize - 1);

    result = op3 + (op1 * op2);
    X[d] = bit_range(result, 0, destsize - 1);
    PC += 4;
}

void core::ADD_SHIFTED() {
    uint64_t d, n, m, imm6, shift, sf, datasize, shift_amount, result, op1, op2;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    m = bit_range(instruction, 16, 20);
    sf = bit_range(instruction, 31);
    shift = bit_range(instruction, 22, 23);
    imm6 = bit_range(instruction, 10, 15);
    datasize = sf ? 64 : 32;

    if (shift == 0b11) undefined();
    if (sf == 0 && bit_range(imm6, 5, 5) == 1) undefined();

    shift_amount = imm6;
    op1 = X[n];
    op2 = shift_reg(X[m], shift, shift_amount, datasize);
    result = op1 + op2;
    result = bit_range(result, 0, datasize - 1);
    X[d] = result;
    PC += 4;
}

void core::UBFX() {
    // semantics
    // ubfx is special case of ubfm
    UBFM();
}

void core::UBFM() {
    uint64_t d, n, R, wmask, tmask, imms, immr, N, sf, datasize, src, bot;
    sf = bit_range(instruction, 31);
    datasize = sf ? 64 : 32;
    d = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    imms = bit_range(instruction, 10, 15);
    immr = bit_range(instruction, 16, 21);
    N = bit_range(instruction, 22);

    if (sf == 1 && N != 1) undefined();
    if (sf == 0 && (N != 0 || bit_range(immr, 5) != 0)) undefined();

    R = immr;
    tie(wmask, tmask) = decode_bitmasks(N, imms, immr, false);
    wmask = bit_range(wmask, 0, datasize - 1);
    tmask = bit_range(tmask, 0, datasize - 1);
    src = X[n];
    bot = ror(src, R, datasize) & wmask;
    X[d] = bot & tmask;
    PC += 4;
}

void core::LDRB_UOFF() {
    uint64_t imm12, n, t, offset, address, data;
    imm12 = bit_range(instruction, 10, 21);
    t = bit_range(instruction, 0, 4);
    n = bit_range(instruction, 5, 9);
    offset = imm12;

    if (n == 31) address = SP;
    else address = X[n];

    // not postindex
    address += offset;
    data = MEM[address];
    X[t] = data;
    PC += 4;
}

void core::BCOND() {
    uint64_t imm19, cond, offset;
    imm19 = bit_range(instruction, 5, 23);
    cond = bit_range(instruction, 0, 3);
    const int len = 21;
    offset = sign_extend(imm19 << 2, len);
    if (condition_holds(cond, PSTATE)) PC += offset;
    else PC += 4;
}

void core::DUMP() {
    printf("instruction %x at %lx\n", instruction, PC);
    for (int i = 0; i < 31; i++)
        printf("X%02d : %016lX\n", i, X[i]);
    printf("XSP : %016lX\n", X[31]);
}

void core::RET() {
    active = false;
}
