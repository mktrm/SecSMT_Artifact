include ../Makefile.mk

channel_results.csv: $(addsuffix .csv,$(NASM_NAMES))
	printf "%f, 0" $$(printf "$(FREQ)/(%f/100)\n" $$(sed -n '21p' covert.csv | cut -f4 -d"," ) | bc) > channel_results.csv