CXX = g++
CFLAGS = -std=c++11 -O2 -Wall -g

TARGET = server
OBJS = ./src/*.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./$(TARGET) -pthread

clean:
	rm ./*.o
	rm ./$(TARGET)