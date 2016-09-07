MAKE_DIR = $(shell pwd)
WORK_DIR = $(MAKE_DIR)/../
SRC_DIR = $(WORK_DIR)/source
INCLUDE_DIR = $(WORK_DIR)/include

all: rftu

rftu: rftu.o rftu_sender.o rftu_receiver.o
	$(CC) -I $(INCLUDE_DIR) -o $@ $^

%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/rftu.h
	$(COMPILE.c) $(OUTPUT_OPTION) -I $(INCLUDE_DIR) $<

.PHONY: clean
clean:
	rm *.o rftu
