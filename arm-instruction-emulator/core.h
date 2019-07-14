#ifndef _CORE_H_
#define _CORE_H_
#include <unordered_map>
using namespace std;
extern unordered_map <uint64_t, uint8_t> MEM;
const uint64_t magic_address = 0xFFFFFFFFFFFFFFFFULL;

class core {
public:
    core(uint64_t entry, uint64_t core_num);
    bool execute();

private:
    bool active;
    uint64_t PC, SP, PSTATE, X[32] = {0};
    uint32_t instruction;

    void ADRP();
    void ADD();
    void STRB();
    void LDRB();
    void LDR_SIGNED();
    void LDR_UOFF();
    void LDR(bool wback, bool postindex, uint64_t scale, uint64_t offset);
    void MOV();
    void MOVZ();
    void CBNZ();

    void STR_UOFF();
    void SUBS();
    void ORR();
    void B();
    void AND();
    void LSR();
    void MADD();
    void ADD_SHIFTED();
    void UBFX();
    void UBFM();
    void LDRB_UOFF();
    void BCOND();
    void MOVN();
    void DUMP();
    void RET();
};
#endif

