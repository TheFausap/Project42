VAR|A|1
VAR|B|1
VAR|T|1
VAR|C|1
VAR|MAXITER|1
STO|MAXITER|35
STO|C|1
STO|A|1
STO|B|1
L1:CMP|C|MAXITER|GTE
JNZ|L2
STO|T|@B
INC|B|@A
STO|A|@T
INC|C|1
JMP|L1
L2:END