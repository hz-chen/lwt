OBJS=lwt.o lwt_channel.o lwt_core.o lwt_types.o
C_FILES=$(wildcard lwt*.c)
ALL_HEADER_FILES=$(wildcard *.h)
COMM_FLAGS=-Wall -Wextra -Wno-unused-parameter
PROF_FLAGS=-pg -lc -g
DBG_FLAGS=-g -DDBG 
O_FLAGS=-O3
LIB_FLAG=-fPIC
LIBS=-llwt
CC=gcc
SRC_DIR=/usr/include/lwt
LIB_DIR=/usr/lib
LIB_FILE=liblwt.so


define make_objs
	$(foreach obj, $(OBJS), $(CC) $1 -c $(subst .o,.c, $(obj)) -o $(obj)
	)
endef


main:  main.o
	$(CC) $(COMM_FLAGS) $(PROF_FLAGS) -o $@ $^ $(LIBS)

exam:  exam.c
	$(call make_objs,$(COMM_FLAGS) $(DBG_FLAGS))
	$(CC) $(COMM_FLAGS) $(DBG_FLAGS) -o exam.o exam.c $(OBJS) $(ALL_HEADER_FILES)
	$(CC) $(COMM_FLAGS) $(DBG_FLAGS) -o $@ exam.o $(OBJS) $(ALL_HEADER_FILES) $(LIBS)

final: $(OBJS) exam.o
	$(CC) $(COMM_FLAGS) $(O_FLAGS) -o $@ $^ $(LIBS)

install: $(ALL_HEADER_FILES) $(C_FILES) 
	mkdir -p $(SRC_DIR)
	cp $^ $(SRC_DIR)
	$(call make_objs,$(COMM_FLAGS) $(DBG_FLAGS) $(LIB_FLAG))
	$(CC) -shared -Wl,-soname,$(LIB_FILE).1 -o $(LIB_FILE).1.0 $(OBJS) $(ALL_HEADER_FILES) -lpthread
	cp $(LIB_FILE).1.0 $(LIB_DIR)
	ln -sf $(LIB_DIR)/$(LIB_FILE).1.0 $(LIB_DIR)/$(LIB_FILE).1
	ln -sf $(LIB_DIR)/$(LIB_FILE).1.0 $(LIB_DIR)/$(LIB_FILE)
	ldconfig -n $(LIB_DIR)

uninstall:
	rm -rf $(LIB_DIR)/$(LIB_FILE).1.0
	rm -rf $(LIB_DIR)/$(LIB_FILE).1
	rm -rf $(LIB_DIR)/$(LIB_FILE)
	rm -rf $(SRC_DIR)

clean: 
	rm -rf exam main final *.o* $(LIB_FILE)*
