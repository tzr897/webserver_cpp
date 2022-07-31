# #define the variants
# CXX = g++
# #src=./bin/locker.o ./bin/cond.o ./bin/sem.o ./bin/threadpool.o ./bin/http_conn.o ./bin/main.o
# src=locker.o cond.o sem.o threadpool.o http_conn.o main.o
# target=./server

# $(target):$(src)
# 	$(CXX) $(src) -o $(target)

# %.o:%.cpp
# 	$(CXX) -c $< -o $@

#define the variants
CXX=g++
OBJS=locker.o cond.o sem.o threadpool.o http_conn.o main.o
SRC=locker.cpp cond.cpp sem.cpp threadpool.cpp http_conn.cpp main.cpp
target=server

$(target):$(src)
	$(CXX) $(src) -o $(target)

locker.o:locker.cpp
	g++ -c $(SRC) -o locker.o -pthread

cond.o:cond.cpp
	g++ -c $(SRC) -o cond.o -pthread

sem.o:sem.cpp
	g++ -c $(SRC) -o sem.o -pthread

threadpool.o:threadpool.cpp
	g++ -c $(SRC) -o threadpool.o -pthread

http_conn.o:http_conn.cpp
	g++ -c $(SRC) -o http_conn.o -pthread

main.o:main.cpp
	g++ -c $(SRC) -o main.o -pthread