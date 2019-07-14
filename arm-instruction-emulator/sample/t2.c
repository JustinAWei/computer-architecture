#include <stdint.h>

char* magic = (char*) 0xFFFFFFFFFFFFFFFFULL;

void write_char(const char c) {
    *magic = c;
}

void write_string(const char* s) {
    do {
        char c = *s;
        if (c == 0) return;
        write_char(c);
        s++;
    } while(1);
}

// multiply-with-carry pseudo-random number generator by George Marsaglia
// source: http://en.wikipedia.org/wiki/Random_number_generation

uint32_t w;
uint32_t z;

void random_init(uint32_t seed) {
    w = seed;
    z = ~seed;
    if ((w == 0) || (w == 0x464fffff)) {
        w += 1;
    }
    if ((z == 0) || (z == 0x9068ffff)) {
        z += 1;
    }
}

uint32_t random_next() {
    z = 36969 * (z & 65535) + (z >> 16);
    w = 18000 * (w & 65535) + (w >> 16);
    uint32_t res = (z << 16) + w;
    return res;
}

char hex_chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

void write_uint32(uint32_t x) {
    write_char(hex_chars[(x >> 28) & 0xf]);
    write_char(hex_chars[(x >> 24) & 0xf]);
    write_char(hex_chars[(x >> 20) & 0xf]);
    write_char(hex_chars[(x >> 16) & 0xf]);
    write_char(hex_chars[(x >> 12) & 0xf]);
    write_char(hex_chars[(x >> 8) & 0xf]);
    write_char(hex_chars[(x >> 4) & 0xf]);
    write_char(hex_chars[(x >> 0) & 0xf]);
}

void start() {
    write_string("hello\n");
    random_init(666);
    for (int i=0; i<10; i++) {
        write_uint32(random_next());
        write_char('\n');
    }
}
