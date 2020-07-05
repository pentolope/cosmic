Suggested compile command:
`clang Main.c -O1 -fno-inline -Wno-parentheses -Werror=implicit-function-declaration -g -o cosmic.exe`
Obviously, if on Linux, the '.exe' is likely not desired.
clang is not required. Other compilers such as gcc should work flawlessly.

I am currently taking no care for the possibility of a C++ compiler attempting to compile this.
The goal of this project is to make (at least) a C99 compiler/linker that can compile/link it's own source code.
