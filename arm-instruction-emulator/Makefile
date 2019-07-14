CPP_FLAGS = --std=c++17 -O0 -g -Wall -Werror
C_FLAGS = --std=c99 -O0 -g -Wall -Werror

CPP_FILES=$(wildcard *.cpp)
CPP_O_FILES=$(subst .cpp,.o,$(CPP_FILES))

CC_FILES=$(wildcard *.cc)
CC_O_FILES=$(subst .cc,.o,$(CC_FILES))

C_FILES=$(wildcard *.c)
C_O_FILES=$(subst .cc,.o,$(C_FILES))

OK_FILES=$(sort $(wildcard *.ok))
OUT_FILES=$(subst .ok,.out,$(OK_FILES))
RESULTS=$(subst .ok,.result,$(OK_FILES))
#TESTS=$(subst .ok,,$(OK_FILES))

O_FILES = $(sort $(CPP_O_FILES) $(CC_O_FILES) $(C_O_FILES))

arm : Makefile $(O_FILES)
	g++ --std=c++17 -o arm $(O_FILES)

$(CPP_O_FILES) : %.o : Makefile %.cpp
	g++ $(CPP_FLAGS) -MD -o $*.o -c $*.cpp

$(CC_O_FILES) : %.o : Makefile %.cc
	g++ $(CPP_FLAGS) -MD -o $*.o -c $*.cc

$(C_O_FILES) : %.o : Makefile %.c
	gcc $(C_FLAGS) -MD -o $*.o -c $*.c

test : $(RESULTS);

$(OUT_FILES) : %.out : Makefile arm sample/%
	timeout 5 ./arm sample/$* > $*.out 2>&1

$(RESULTS) : %.result : Makefile %.out %.ok
	-@((diff -b $*.out $*.ok > /dev/null 2>&1) && echo "$* ... pass") || (echo "$* ... fail" ; echo "--- expected ---" ; cat $*.ok ; echo "--- found ---" ; cat $*.out)

clean :
	rm -rf *.out *.d *.o arm

-include *.d
