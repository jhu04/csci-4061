CC=gcc
CFLAGS=-g -Wall -pthread

SRCDIR=src
INCDIR=include
LIBDIR=lib

all: outdir $(LIBDIR)/utils.o image_rotation

image_rotation: $(LIBDIR)/utils.o $(INCDIR)/utils.h $(SRCDIR)/image_rotation.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LIBDIR)/utils.o $(SRCDIR)/image_rotation.c -lm


.PHONY: clean outdir

test: clean all
	@read -p "Please a Dir number: " dir_num; \
	./image_rotation img/$$dir_num output/$$dir_num 10 180;\


outdir: 
	for i in `seq 0 10`; do mkdir -p -m 0777 "output/$$i"; done;

clean:
	rm -f image_rotation
	rm -rf output

performance: Makefile
	for i in `seq 1 100`; do rm -rf output; echo $$i; time -v ./image_rotation img/5 output/5 $$i 180; done;

echo_performance: performance
	make performance &> performance/performance.txt