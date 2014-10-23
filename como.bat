@echo off
tcc^
 "-I./include" ^
 "-I./include/duktape" ^
 "-L./include/duktape" ^
 "-L." ^
 "src/loop/core.c" ^
 "include/thread/tinycthread.c" ^
 "include/http/http_parser.c" ^
 "include/ansi/ANSI.c" ^
 "-run -lduktape -lws2_32 -lwsock32 -llibssl32 -lUser32" src/main.c %*
