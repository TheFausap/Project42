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

There is, also, one pseudo-instruction:

- VAR

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

- `<N>` any integer number
- `<M>` memory location (integer 16-bit number) ... any, fun stuff may happens :) i.e. Any instruction is encoded as an integer, so a program could rewrites itself.
- `<V>` variable name
- `<L>` label
- `<MD>` mode of comparison

### Pseudo-instruction : VAR

The format is

```
VAR|<V>|<N>
```

It allocates `<N>` contigous memory addresses and created a reference to the name specified by `<V>`.

Must be at the beginning of the program.

### Instruction : STO

The format is

```
[<L>:]STO|<M> or <V>[#<N> or <V>]|<N> or [@]<V>[#<N> or <V>]
```

It stores a value or a content of a variable in a memory location pointed by a integer number or a variable name.

The operator `#` represents an index in the memory area pointed by V.

### Instruction : LDM

The format is

```
[<L>:]LDM|<M> or <V>|<M> or [@]<V>
```

Copy memory locations (like memcpy in C), but it can also use variables. It's mostly a duplicate of `STO`, so maybe this can be removed in the future or it will change the syntax/behaviour.

### Instructions : INC / DEC

The format is

```
[<L>:]DEC|<M> or <V>[#<N> or <V>]|<N> or [@]<V>
```


```
[<L>:]INC|<M> or <V>[#<N> or <V>]|<N> or [@]<V>
```

It adds or subtract the value specified by an integer number or the contect of the variable, to the value stored at memory location (direct address) or variable.

### Instruction : CMP

The format is

```
[<L>:]CMP|<M> or <V>|<M> or <V>|<MD>
```

It compares the content of two memory locations (pointed by an address or variable) using the criteria specified by `<MD>`.

The criteria `<MD>` is one of the following mnemonics:

- LSS
- GTT
- LSE
- GTE
- EQL

It uses 8-bit lower portion of the R15 registry (at the moment the registry itself is not addressable directly by any instructions, but in the future this may changes, so this is a word of advice). The content of R15 is zeroed each time a CMP instruction is called.

The result change the flags registry. Only 1-bit (0 or 1) is stored, and used in the flags registry, as result of a boolean operation (false/true).

The fact that CMP uses a registry can be lead to further expansions and more complex operations, also not directly related to the comparison (and this will requires a name change, possibly).

### Instructions : JMP / JNZ

The format is:

```
[<L>:]JMP|<M> or <L>
```


```
[<L>:]JNZ|<M> or <L>
```

Standard mnemonics here:

- JMP is unconditional jump to a memory location pointed by a number or label
- JNZ is conditional jump (if the first bit of flags registry is not zero, due to a CMP operation) to a memory location pointed by a number or label

It doesn't clear the flags.

### Instruction : END

The format is:

```
[<L>:]END
```

It ends the program. It accepts also a label. Basically the instruction is like a NOP.
Because the memory is zeroed, any jump to any very high unused memory address will end the program without error.

Ending a program with this instruction is totally optional, but it can be useful if used with a label.

## Example code

### Fibonacci number

Below you can find a program that can evaluate the N-th fibonacci number

```
VAR|A|1
VAR|B|1
VAR|T|1
VAR|C|1
VAR|MAXITER|1
STO|MAXITER|35
STO|C|2
STO|A|0
STO|B|1
L1:CMP|C|MAXITER|GTE
JNZ|L2
STO|T|@B
INC|B|@A
STO|A|@T
INC|C|1
JMP|L1
L2:END
```

`MAXITER` is the n-th fibonacci number we want to evaluate. The final result will be stored in the `B` variable.

Of course there are no `bigInteger` here, so the limit is the `long long int` range.

The same program is stored in the `test.asm` file.

### Variable indexing

```
VAR|AB|5
VAR|C|1
VAR|M|1
STO|M|5
STO|C|0
L1:STO|AB#C|10
INC|C|1
CMP|C|M|GTE
JNZ|L2
JMP|L1
L2:END
```

In this code, the `#` operator is used alongside a variable. This loop fills the first 5 memory location pointed by `AB`, with value 10.

