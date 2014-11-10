HEADER_FILES=$(wildcard *.h)
COMM_FLAGS=-Wall -m32
PROF_FLAGS=-pg -lc -g
DBG_FLAGS=-g -DDBG 
O_FLAGS=-O3
LIBS=-llwt -lpthread
OUT_PATH=out
main: main/main.c 
	gcc $(OUT_PATH)/$^ $(COMM_FLAGS) $(PROF_FLAGS) -o $@ $(LIBS)

exam: $(HEADER_FILES)  exam.c
	gcc $^ $(COMM_FLAGS) $(DBG_FLAGS) -o $@ $(LIBS)

final: $(HEADER_FILES)  exam.c
	gcc $^ $(COMM_FLAGS) -O3 -o $@ $(LIBS)

install:  $(HEADER_FILES) 
	mkdir -p /usr/include/lwt
	cp $^ /usr/include/lwt/
	gcc  $(COMM_FLAGS) -o liblwt.so.1.0 -fPIC $^ -lpthread -shared
	cp liblwt.so.1.0 /usr/lib
	ln -sf /usr/lib/liblwt.so.1.0 /usr/lib/liblwt.so.1
	ln -sf /usr/lib/liblwt.so.1.0 /usr/lib/liblwt.so

uninstall:
	rm -rf /usr/lib/liblwt.so*

clean: 
	rm -rf exam main final *.o 
