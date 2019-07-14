CFLAGS=-g -std=c99 -O0 -Werror -Wall

TEST_FILES=$(wildcard *.test)
TESTS=$(sort $(basename $(TEST_FILES)))
OK_FILES=$(addsuffix .ok,$(TESTS))
OUT_FILES=$(addsuffix .out,$(TESTS))
RESULT_FILES=$(addsuffix .result,$(TESTS))

p1 : p1.o Makefile
	gcc $(CFLAGS) -o p1 p1.o -lm

%.o : %.c Makefile
	gcc $(CFLAGS) -MD -c $*.c

%.o : %.S Makefile
	gcc $(CFLAGS) -MD -c $*.S

$(OUT_FILES) : %.out : Makefile p1 %.test
	-eval "timeout 5 ./p1 `cat $*.test` || echo timeout" > $*.out

$(RESULT_FILES) : %.result : Makefile %.out %.ok
	-@((diff -b $*.out $*.ok > /dev/null 2>&1) && echo "$* ... pass") || (echo "$* ... fail" ; echo "--- expected ---" ; cat $*.ok ; echo "--- found ---" ; cat $*.out)

test : Makefile $(RESULT_FILES)

clean :
	rm -f *.out *.d *.o p1

-include *.d
