@echo off
tcc -Wall ^
 "-I./libs" ^
 "-I./libs/mbedtls/library" ^
 "-I./libs/duktape" ^
 "-L." ^
 "src/loop/core.c" ^
 "libs/thread/tinycthread.c" ^
 "libs/http/http_parser.c" ^
 "libs/ansi/ANSI.c" ^
 "-run -lduktape -lmbedtls -lws2_32 -lwsock32 -lUser32" src/main.c %*
