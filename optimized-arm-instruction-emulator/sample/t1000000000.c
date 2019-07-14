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
    
volatile uint64_t n = 1000000000;

void start() {
    write_string("hello\n");
    while (n > 0) n--;
    write_string("goodbye\n");
}
