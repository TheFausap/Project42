# Project42VM

This VM is a very rough implementation, with the aim to use it in my other project (LSPY).
Nevertheless, it cames with an embedded assembler, and, of course, interpreter, all in one file.

It has a lot of limitations, mainly is support only integers at the moment, but at the moment I quite happy with it.

## VM description

At the moment the VM uses 32KB of memory, and the first 2KB are reserved for the program. The remaining part is the DATA memory. At the beginning of the DATA memory, there's the static part for the allocation of the variables/arrays. Soon after that the program begins.

```
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
```

Memory is initialised to zero, and the END/STOP instruction is encoded as 0, this means no "invalid memory" address, the VM naturally stops.

Each instruction is encoded in a long long int (aka 64bit integer), with this format:

```
 6                                                              
 3                                                              0 
##################################################################
|VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVMMMMMMMMMMMMMMMMIII|
##################################################################
```

where:

- I = instruction opcode 
- M = memory location where to find data / variable location
- V = values to store (for the "immediate" mode) or variable location

It uses also one 32 bit registry, but at the moment only internally.

It has a 8bit flag registry (for the comparison operation).

It has a 32bit PC registry.

## Instructions

It uses the following 7 instructions (it's very RISC-y :) )

- END
- STO
- LDM
- JMP
- DEC
- INC
- JNZ
- CMP

using mostly a standard mnemonic naming convention. More on this later.

Each instruction as different addressing method, described also later.

## Assembler syntax

The source file is a text file, formatted in this way:

```
VAR|AB|1
VAR|B|1
STO|AB|42
INC|20|15
LDM|21|20
DEC|AB|10
L2:CMP|20|21|LSE
JNZ|6
JMP|L2
INC|B|776
INC|AB|@B
END
```

Some rules:

1) VAR space declaration must happen _ONLY_ at the beginning
2) Labels _MUST_ ends with a column
3) Value inside a variable location can be accessed using '@' operator
4) The character "|" is the instruction separator
5) Labels can be used _ONLY_ with jump instructions
6) No arithmetic in the assignment operation

As you can see INC/DEC are basically ADD/SUB operations.

Below the symbols mean:

- `<N>` any number
- `<M>` memory location ... any, fun stuff may happens :)
- `<V>` variable name
- `<L>` label

### Instruction : STO

The format is

```
[<L>:]STO|<M> or <V>|<N> or [@]<V>
```
