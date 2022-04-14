
CFLAGS := -g -Wall

TARGET := main

OBJS := common.o pipe1.o pipe.o fifo1.o fifo.o

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

pipe1.o: pipe1.cpp common.h
pipe.o: pipe.cpp common.h

fifo1.o: fifo1.cpp common.h
fifo.o: fifo.cpp common.h

common.o: common.cpp common.h

main: main.cpp common.h $(OBJS)

