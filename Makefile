#define the variants
src=cond.o http_conn.o locker.o sem.o threadpool.o main.o
target=server

$(target):$(src)
	$(CC) $(src) -o $(target)

%.o:%.cpp
	$(CC) -c $< -o $@