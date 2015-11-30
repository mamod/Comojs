# Comojs

This is a development branch for nodejs api using duktape
embeddable javascript engine and a tiny event loop

This is a work in progress, the first release will be once I get
the following modules pass all tests

- net
- dns
- http
- https
- fs
- child_process

### Please note that performance will never match nodejs for 2 main reasons

1 - nodejs use V8 which is a JIT-based javascript engine, duktape isn't, duktape was designed for portability and memory constrained devices

2 - nodejs use libuv for it's event loop and c++ wrappers to bridge with javascript, comojs use a tiny event loop and all wrappers written in pure javascript for simplicity, I only used C for things that can't be done in Javascript

## design

To be fully compatible with nodejs api the easiest thing to do is using nodejs modules as is and then write wrappers in javascript to emulate libuv and node C++ wrappers see ``js/uv`` and ``js/node/wrapper`` folders, this contributed to reduce performance but also speed the development process, I may rethink this approach once we pass the tests :)

**More to come soon :)**
