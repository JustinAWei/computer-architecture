GCC = aarch64-linux-gnu-gcc
OBJDUMP = aarch64-linux-gnu-objdump

C_FILES = $(wildcard *.c)
C_O_FILES = $(subst .c,.o,$(C_FILES))

S_FILES = $(wildcard *.S)
S_O_FILES = $(subst .S,.o,$(S_FILES))

s_FILES = $(wildcard *.s)
s_O_FILES = $(subst .s,.o,$(s_FILES))

O_FILES = $(C_O_FILES) $(S_O_FILES) $(s_O_FILES)

SAMPLES = $(subst .o,,$(O_FILES))

all : Makefile $(SAMPLES);

$(s_O_FILES) : %.o : Makefile %.s
	$(GCC) -c $*.s

$(S_O_FILES) : %.o : Makefile %.S
	$(GCC) -c $*.S

$(C_O_FILES) : %.o : Makefile %.c
	$(GCC) -O3 -c $*.c

$(SAMPLES) : % : Makefile %.o
	$(GCC) -e start -nostdlib -static -o $@ $*.o
	$(OBJDUMP) -D $@ > $*.disas

#hello : Makefile hello.c
#	@echo "+-------+"
#	@echo "| hello |"
#	@echo "+-------+"
#	$(GCC) -O3 -e start -nostdlib -nostdinc -static -o $@ $@.c
#	$(OBJDUMP) -D $@ > $@.disas
