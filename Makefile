INP_OPTS = `pkg-config --cflags glib-2.0 libcurl`
OUT_OPTS = `pkg-config --cflags glib-2.0 libxml-2.0`
EXE_OPTS = `pkg-config --libs glib-2.0 libcurl libxml-2.0`

all: 		vlast

clean:
	@rm -f vlast *.o

vlast:				vlast-input.o vlast-output.o
	@cc -o vlast vlast-output.o vlast-input.o -Wall $(EXE_OPTS)

vlast-input.o:		vlast-input.c vlast.h
	@cc -c -o vlast-input.o vlast-input.c -Wall -O2 $(INP_OPTS)

vlast-output.o:		vlast-output.c vlast.h
	@cc -c -o vlast-output.o vlast-output.c -Wall -O2 $(OUT_OPTS)
