MAKE_DIR = $(shell pwd)
WORK_DIR = $(MAKE_DIR)/..
SRC_DIR = $(WORK_DIR)/source
INCLUDE_DIR = $(WORK_DIR)/include
P_THREAD = -pthread

all: rftu

rftu: rftu.o main_sender.o rftu_sender.o main_receiver.o rftu_receiver.o
	$(CC) $(P_THREAD) -I $(INCLUDE_DIR) -o $@ $^

%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/rftu.h
	$(COMPILE.c) $(OUTPUT_OPTION) -I $(INCLUDE_DIR) $<

.PHONY: clean
clean:
	rm *.o rftu
