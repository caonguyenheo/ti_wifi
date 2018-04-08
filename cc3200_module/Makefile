.PHONY: tests examples programs

all: tests examples programs
clean: cleaneverything

tests: 
	make -f Makefile.tests

examples:
	make -f Makefile.examples

programs:
	make -f Makefile.programs

cleaneverything:
	make -f Makefile.examples clean
	make -f Makefile.tests clean
	make -f Makefile.programs clean
