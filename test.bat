@echo off
tcc "-I./include" "./src/loop/core.c" "-run -lws2_32 -lwsock32" ./tests/loop.c
