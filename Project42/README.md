# Project42VM

This VM is a very rough implementation, with the aim to use it in my other project (LSPY).
Nevertheless, it cames with an embedded assembler, and, of course, interpreter, all in one file.

It has a lot of limitations, but at the moment I quite happy with it.

## VM description

At the moment the VM uses 32KB of memory, and the first 2KB are reserved for the program. The remaining part is the DATA memory. At the beginning of the DATA memory, there's the static part for the allocation of the variables/arrays. Soon after that the program begins.

####### 0  
|     |  
  ...   *Program area*  
|     |  
|-----| 2048  
|     |  
  ...   *Variables / Constants / Arrays area*  
|     |  
|-----| 2048 + X   
|     |  
  ...   *Program memory area*  
|     |  
####### 32767  

Each instruction is encoded in a long long int (aka 64bit integer), with this format:

 6                                                              
 3                                                              0 
##################################################################
|VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVMMMMMMMMMMMMMMMMIII|
##################################################################

where:

- I = instruction opcode 
- M = memory location where to find data / variable location
- V = values to store (for the "immediate" mode) or variable location

It uses also one 32 bit registry, but at the moment only internally.

It has a 8bit flag registry (for the comparison operation).