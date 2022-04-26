
CFLAGS := -g -Wall -fPIC
CXXFLAGS := -g -Wall -fPIC

TARGET := main

OBJS := common.o pipe1.o pipe.o pipesplice.o fifo1.o fifo.o shm.o null.o nullcopy.o

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

pipe1.o: pipe1.cpp common.h
pipe.o: pipe.cpp common.h
pipesplice.o: pipesplice.cpp common.h

fifo1.o: fifo1.cpp common.h
fifo.o: fifo.cpp common.h

shm.o: shm.cpp common.h

null.o: null.cpp common.h
nullcopy.o: nullcopy.cpp common.h

common.o: common.cpp common.h

main: main.cpp common.h $(OBJS)

