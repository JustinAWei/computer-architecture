#include <cstdio>
#include <cstdlib>

#include "mem.h"
#include "elf.h"

#include "helpers.h"
#include "core.h"

#include <vector>
#include <iostream>
#include <assert.h>

using namespace std;
unordered_map <uint64_t, uint8_t> MEM;

void mem_write8(uint64_t addr, uint8_t data) {
    MEM[addr] = data;
}

int main(int argc, const char * argv[]) {
    const int num_cores = 1;
    vector <core> cores;
    bool active = true;

    const char *fileName = argv[1];
    uint64_t entry = loadElf(fileName);

    // initialize cores
    for (int i = 0; i < num_cores; i++)
        cores.push_back(core(entry, i));

    while (active) {
        active = false;
        for (int i = 0; i < num_cores; i++)
            active |= cores[i].execute();
    }
    return 0;
}
