WORK_DIR = $(shell pwd)/..

all: rftu
rftu: rftu.o rftuReceiver.o rftuSender.o
	$(CC) -o $@ $^
