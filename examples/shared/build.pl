system("gcc -w -c dll.c");
system("gcc -w ../../include/duktape/duktape.c -shared -o dll.como dll.o");
unlink('./dll.o');
