Suggested initial compile command for cosmic:\
`clang Cosmic.c -O1 -Wno-parentheses -o cosmic.exe`\
Obviously, if on Linux, the '.exe' is likely not desired.\
clang is not required. Other compilers such as gcc should work flawlessly.\
Other optimization levels should all work flawlessly, and should produce identical output.\
If compiled at `-O3`, cosmic is very fast ;)\
\
There are 2 other tools that can be made in addition to `cosmic`: `sim` and `_Loader_Sim`.\
`sim` is intended to be used on modern machines, and will not work on the target machine.\
`_Loader_Sim` is intended to be used as a loader inside the simulator, and it will not work directly on modern machines. Additionally, it probably will not successfully compile on any compiler except `cosmic`.\
A loader for the physical machine will be similiar to `_Loader_Sim`, but is currently not made yet.\
\
I am currently taking no care for the possibility of a C++ compiler attempting to compile this.\
The goal of this project is to make (at least) a C99 compiler/linker that can compile/link it's own source code.\
I am likely not going to include some of the features in C99 that are optional in C18.\
\
I have not thought about how I want to license this code. For now, just follow this:\
-You may download this repository.\
-You may compile the code in this repository.\
-You may use the contents in this repository for personal use (NOT commercial use).\
--Though, I would add, why would anyone else use this? It doesn't target x86 or ARM, it targets an arcitecture of my own creation.\
-You may give feedback on the contents in this repository.\
-You may NOT claim the contents in this repository as your own, regardless of if it is modified or unmodified.\
-Any use of the contents in this repository, regardless of if it is modified or unmodified, must refer to me (pentolope) as the creater.\
--Further, if anyone else uses this, or it ever inspires any ideas in anyone, I would like to known about it so please send me a message on github.\
-Don't be a rude person, I've worked for months on this project.\
-Have a nice day :)


