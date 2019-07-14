CFLAGS=-g -std=gnu99 -O3 -Werror -Wall

CFILES=$(sort $(wildcard *.c))
OCFILES=$(subst .c,.o,$(CFILES))
OFILES=$(OCFILES) $(OSFILES)
TESTS=$(sort $(wildcard *.test))
EXECS=$(subst .test,,$(TESTS))
OKFILES=$(subst .test,.ok,$(TESTS))
RUNS=$(patsubst %.test,%.result,$(TESTS))


all : $(EXECS)

$(OCFILES) : %.o : %.c Makefile
	-gcc $(CFLAGS) -MD -c $*.c

test : $(RUNS)

$(EXECS) : % : %.test Makefile $(OFILES)
	-gcc -x c $(CFLAGS) -MD -c $*.test
	-gcc -o $* $*.o $(OFILES)
	

$(OKFILES) : %.ok :
	touch $@



$(RUNS) : %.result : % %.ok
	@/bin/echo -n "$* ... "
	@-timeout 5 ./$* > $*.out
	@((diff -b $*.out $*.ok > /dev/null) && echo "pass") || (echo "fail")

clean :
	rm -f *.out $(EXECS)
	rm -f *.d
	rm -f *.o

-include *.d
