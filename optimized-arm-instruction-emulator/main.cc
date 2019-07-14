#include <cstdio>
#include <cstdlib>

#include "mem.h"
#include "elf.h"

#include "core.h"

#include <vector>
#include <iostream>
#include <assert.h>

using namespace std;
uint32_t MEM[memsize+1];
unordered_map<uint64_t, uint32_t> VMEM;

void mem_write8(uint64_t addr, uint8_t data) {
    int off = 0;
    while(addr %4 != 0) {
        addr--;
        off++;
    }
    uint32_t *ptr = (addr < memsize) ? &MEM[addr] : &VMEM[addr];
    *ptr |= data<<(off*8);
}

int main(int argc, const char * argv[]) {
    const int num_cores = 1;
    vector <core> cores;
    bool active = true;

    const char *fileName = argv[1];
    uint64_t entry = loadElf(fileName);

    for (int i = 0; i < num_cores; i++)
        cores.push_back(core(entry, i));

    while (active) {
        active = false;
        for (int i = 0; i < num_cores; i++)
            active |= cores[i].execute();
    }
}
