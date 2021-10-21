
.PHONY: build test clean run

.DEFAULT_GOAL := run

CC = g++
CPPFLAGS = -O2 -m64
LDFLAGS = -lpthread
COUNTERS = 2#310,315,316,317
NASM_NAMES = covert  
LOGLEVEL ?= INFO
COMMON = ../common/
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
FREQ = $(shell printf "%.0f*1000\n" $$(cat /proc/cpuinfo | grep MHz | cut -d: -f 2 | sort -r | head -n 1 ) | bc)


$(COMMON)/a64.o: $(COMMON)/PMCTestA.cpp
	@$(CC)  $^ -c $(CPPFLAGS) -o$@

b64-%.o:  $(COMMON)/TemplateB64.nasm covert.nasm $(COMMON)/lfsr.inc
	@nasm  -f elf64 -o $@ -i$(COMMON) \
    -Dcounters=$(COUNTERS) \
	-D$* \
    -Pcovert.nasm \
    $(COMMON)/TemplateB64.nasm

exec-%: b64-%.o $(COMMON)/a64.o
	@$(CC)  $^ $(CPPFLAGS) -o$@ $(LDFLAGS)


%.csv: exec-%
	@echo "Measure, #nops, T1/T2, T2/T1, T1, T2," > $*.csv
	@./exec-$* 1 >> $*.csv


channel_results.csv: $(addsuffix .csv,$(NASM_NAMES)) $(COMMON)/bw.py
	@export LOGLEVEL=$(LOGLEVEL) && $(COMMON)/bw.py $(addprefix $(ROOT_DIR)/,$^) $(FREQ) $(HIGH_ENCODING) > channel_results.csv

run: channel_results.csv 

clean: 
	@$(RM) *.csv
	@$(RM) plot.pdf
