@echo off
tcc -Wall ^
 "-I./include" ^
 "-I./include/mbedtls/library" ^
 "-I./include/duktape" ^
 "-L." ^
 "src/loop/core.c" ^
 "include/thread/tinycthread.c" ^
 "include/http/http_parser.c" ^
 "include/ansi/ANSI.c" ^
 "-run -lduktape -lmbedtls -lws2_32 -lwsock32 -lUser32" src/main.c %*
