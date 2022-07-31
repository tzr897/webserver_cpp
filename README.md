# **Webserver**
## A Linux webserver developed with C++ (Still being improved...)


## Description


## Environment


## Test

g++ ./src/*.cpp -o server -pthread

./server 10000


In another terminal:

./test_presure/webbench-1.5/webbench -c 5000 -t 5 http://192.168.152.133:10000/index.html
