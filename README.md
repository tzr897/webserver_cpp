# **Webserver**
## A Linux webserver developed with C++ (Still being improved...)


## Description


## Environment


## Test

g++ ./src/*.cpp -o server -pthread

./server portnumber


In another terminal:

./test_presure/webbench-1.5/webbench -c 5000 -t 5 http://yourip:portnumber/index.html
