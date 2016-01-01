Comojs
======

Server side javascript using [duktape](https://github.com/svaarala/duktape) javascript engine

Why
---

I've started this project as a learning process and to deeper explore both ``C`` and ``javascript`` languages
so ``duktape`` seemed the best tool with it's beautiful API and easy integration. In my way working on this project, I stumbled upon ``Golang`` and boy how elegeant their packages and coding style! comparing to ``nodejs`` golang standard packages is way easier to read and follow, falling in love with GO left me no choice but to explore it within comojs as well by implementing some of it's packages in javascript way, this may sounds confusing, nodejs and Go API in one place!! it's crazy but I have a good feeling for this combination :)

Status
-------

Nothing much to see now, the master branch is useless at the moment, in the coming days I'll work on the comojs-go branch and experience how I can implement GO API in javascript way, this is the hardest part, then I'll think of a way to merge nodejs API, I already have lots of nodejs tests passing so this should be easier, I guess :P

I'm writing this on the first day of 2016, so if you're reading this way after 1st of january 2016 and you think this is interesting and I'm too late then just ping me, sometimes I need a push to accelerate thing ;)

Philosophy
----------
* Everthing Should by as tiny as possible
* Implement most things in javascript
* Must maintaine a low footprint memory usage
* Should run without compilation using tinycc compiler
* Fast compilation time, current compilation time ~ 3 seconds (without embdTLS)
* Excution speed is not a priority.

Final Product
-------------

* Will have some subset of nodejs API
* Will have some subset of Go API
* Can build stand alone excutables for easy distribution

