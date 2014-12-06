Comojs
======

Server side javascript using [duktape](https://github.com/svaarala/duktape) javascript engine

Currnt State
------------

Work in progress, lots of things to do, lots of things to fix, and even more lots of lots of things :)

Philosophy
----------
* Everthing Should by as tiny as possible
* Implement most things in javascript
* Must maintaine a low footprint memory usage
* Should run without compilation using tinycc compiler
* Fast compilation time, current compilation time ~ 3 seconds

Maybe
-----
"maybe" eventually Comojs will run unmodified nodejs modules (not native modules) but this is not a current purpose of Comojs.

Build
-----

You need to have perl installed, this is just for now until I write a Makefile

``perl make.pl``

or
--

You can run directly without compilation using tinycc compiler

* make sure you have tinycc installed and in your system path
* on windows

   ``como ./examples/readline.js``
* on linux
    
    ``./como.sh ./examples/readline.js``
