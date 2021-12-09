# cosmic

<span>
<b>C</b><br>
<b>O</b><i>ptimizing</i><br>
<b>S</b><i>tack-Machine</i><br>
<b>I</b><i>nspired</i><br>
<b>C</b><i>ompiler</i><br>
</span>

# 

## Suggested compile command for *cosmic*  
`clang Cosmic.c -Wno-parentheses -o cosmic.exe`  
###### Obviously, if using Linux, '.exe' is likely not desired.  

## In General

*cosmic* is typically quite fast. On Windows, it can easily rival *clang* and *gcc* in compile speed, even when *cosmic* is told to perform optimizations.  

*cosmic* uses very little memory. On Windows, it reliably uses significantly less memory than *clang* and *gcc*.  

*cosmic* will never leak memory, will never perform a memory access out of bounds, and will never depend on the contents of uninitialized memory. I test this often using *valgrind*.  

*cosmic*'s optimizations should never effect the output of code (except in cases where the volatile qualifier should have been used or cases that depend on the locations of position independant symbols).  

*cosmic*'s output is a linkable file format that contains named symbols and position independant bytecode. There is no assembler because *cosmic* converts C directly into bytecode.  

*cosmic* should compile and run flawlessly with:  
- Any C compiler that supports some of the most basic features of C99.  
- Any optimization level.  

## Warning and Error messages

*cosmic* (nearly always) gives very informative warning and error messages. (I'm still working on making that 'nearly' disappear.)  

These messages use ANSI escape codes to provide color. You can use the `-nc` command argument to suppress these escape codes for terminals that don't support them.  

These messages give the name of the source file, line number, character number in that line, a relevant segment of that line, and (if possible) an underline showing the area. It shows this for both the preprocessed source text and the source text prior to preprocessing.  

## Other contents

There are 4 other relatively large programs/tools in this repository:  
- *sim.c*  
  - This is a simulator for [cookie](https://github.com/pentolope/cookie), which is *cosmic*'s partner project.  
  - This uses the SDL (v2) library. You will need the appropriate headers and DLL to compile this. I didn't want to host those files in this repository since I did not create them.  
  - This simulator does not simulate the implementation details of *cookie*.  
  - This simulator does simulate the instruction set architecture that *cookie* uses.  
  - In addition, this simulator also simulates some of *cookie*'s IO (such as displaying text mode in a window and transforming the host's keyboard events into PS2 scan codes).  
  - This also has a few additional tools built in that are used for things such as:  
    - Debugging during simulation.  
    - Performance profiling during simulation.  
    - Special tool for transforming *cosmic*'s position independant bytecode into a different format for *cookie*'s *AutoGen.py*.  
	  - *AutoGen.py* uses this format to generate raw binary files (that are position dependant) for *cookie*'s bootloader and *KCS*.  
	  - Frankly, I think this should be handled entirely by *AutoGen.py*. However, due to the way I implemented some of the details of *cosmic*'s output bytecode, it was easier to make *sim* do some of that processing.  
- *_Loader_Sim.c*  
  - This is the loader routine for *sim*. It is used to load and run bytecode files inside of *sim*.  
    - The loader for *sim* couldn't be exactly the same as the loader for *cookie* since their implementations of some standard library functions are different.  
    - Those implementations are different because I wanted *sim* to be able to access the filesystem on the host machine.  
  - This will probably not successfully compile on any compiler except *cosmic*.  
- *KCS.c*  
  - This is the *Kernel Console Shell*.  
  - This contains a primitive bash-like shell for controlling a primitive kernel.  
  - This contains the loader routine for *cookie*.  
    - The loader for *sim* couldn't be exactly the same as the loader for *cookie* since their implementations of some standard library functions are different.  
    - Those implementations are different because *cookie* interfaces with the FAT file system on a SD card.  
  - This will probably not successfully compile on any compiler except *cosmic*.  
- *TargetInstructions/CoreGeneration.py*  
  - This is a tool I use for generating correct C code regarding the instructions represented in *cosmic*'s bytecode.  
  - This generates the C code for a function that *cosmic* can use to print arrays of instructions for debugging purposes.  
  - This generates the C initializers containing arrays of instructions that *cosmic* uses as it's building blocks for the construction of output (prior to optimization).  

There are 3 other small tools in this repository:  
- *IsIden.py*  
  - This is a tiny tool I use for testing.  
  - Since *cosmic* can compile itself, it's output should be identical regardless of if *cosmic* was compiled by itself or compiled by *clang*.  
  - This tool reads both output files and checks if they contain identical data.  
- *GenStdObjInitializer.py*  
  - This is a tiny tool I use for generating correct C code for the loader routine.  
  - When the loader is loading a bytecode file into memory it must link the implementation of C's standard library while it is loading it.  
  - This tool generates some C code that holds the names and address of those functions in sorted order.  
  - Sorted is important because the loader uses binary search for symbol name lookup and it doesn't want to waste time sorting at runtime.  
- *IntrinsicBuiltFiles/create_initializer_for_loader.py*  
  - This is a tiny tool I use for generating correct C code for the loader routine.  
  - This tool converts a binary file into an initializer so it can be viewed in C as an array.  

## Other Notes

*cosmic* requires byte accessed little-endian memory to run correctly. It will check this when it starts.  

I am currently taking no care for the possibility of a C++ compiler attempting to compile *cosmic*.  

The goal of this project is to make (at least) a C99 compiler/linker that can compile/link it's own source code.  

I am likely not going to include some of the features in C99 that are optional in C18.  

## I have not thought about how I want to license this code. For now, obey this:  
- You may download this repository.  
- You may compile the code in this repository.  
- You may use the contents in this repository for personal use (NOT commercial use).  
  - Although, why would anyone else use this? It doesn't target x86 or ARM, it targets an arcitecture of my own creation.  
- You may give feedback on the contents in this repository.  
- You may NOT claim the contents in this repository as your own, regardless of if it is modified or unmodified.  
- Any use of the contents in this repository, regardless of if it is modified or unmodified, must refer to me (pentolope) as the creater.  
  - Further, if anyone else uses this for something, or it ever inspires any ideas in anyone, I would like to known about it so please notify me via github.  
- Don't be a rude person by stealing my work. I've worked hard for years on this project.  
- Have a nice day


