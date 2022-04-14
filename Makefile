
CFLAGS := -g -Wall

TARGET := main

OBJS := common.o pipe1.o pipe.o fifo1.o fifo.o

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

pipe1.o: pipe1.cpp
pipe.o: pipe.cpp

fifo1.o: fifo1.cpp
fifo.o: fifo.cpp

common.o: common.cpp

main: main.cpp $(OBJS)

