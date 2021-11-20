//
//  main.c
//  P42
//
//  Created by Fausto Saporito on 16/11/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MEMSIZE 32767
#define DATAORIG 2048
#define STACKORIG DATAORIG-256
#define INT_WIDTH 64

#define VSHIFT 22

long long int mem[MEMSIZE] = { 0 };
long double dmem[MEMSIZE] = { 0.0 };
short dmemp = 0;

int DATAOFF = DATAORIG;       /* max prog size in byte */
int DDATAOFF = 1024;          /* 1KB reserved */
int pc = 0;
int stackpos = 0;

long long int r0 = 0;
long long int r1 = 0;

long double dr0 = 0.0;
long double dr1 = 0.0;

char flags = 0;
char ind[DATAORIG] = { 0 };
short indexes[DATAORIG] = { 0 };
short procs[DATAORIG] = { 0 };

typedef struct {
    char sym[10];
    int a;                /* position in the DATA MEM */
    int sz;
} VE;

typedef struct {
    char sym[10];
    int a;                /* position in the DATA MEM */
} LE;

VE var_area[50] = { 0,0,0 };
LE labels[100] = { 0,0 };
int var_count = 0;
int lab_count = 0;

enum {NOP=0,STO,LDM,JMP,DEC,INC,JNZ,CMP};

#define LSS 0
#define GTT 1
#define LSE 2
#define GTE 3
#define EQL 4
#define DBL 5  /* used to store a double val in dr0 or dr1 */

#define DADD 50 /* perform the related arith operation */
#define DSUB 51 /* perform the related arith operation */
#define DMUL 52 /* perform the related arith operation */
#define DDIV 53 /* perform the related arith operation */
#define DMOV 54 /* move double data from reg R0 or R1 into DMEM */

#define DR0  55
#define DR1  56

#define STRFY(x) #x

#define D(x) DATAOFF+(x)
#define D1(x) DATAORIG+(x)

#define DH(x) memmove((x), (x) + 1, strlen((x)))
#define DT(x) (x)[strlen((x)) - 1] = '\0'

char* itob(long long int n) {
    int num_bits = sizeof(long long int) * 8;
    char* string = malloc(num_bits + 1);
    if (!string) {
        return NULL;
    }
    for (int i = num_bits - 1; i >= 0; i--) {
        string[i] = (n & 1) + '0';
        n >>= 1;
    }
    string[num_bits] = '\0';
    return string;
}

void var(char* s, int sz) {
    strncpy(var_area[var_count].sym, s, 9);
    if (sz < 0) {
        /* negative size means floating-point variable */
        var_area[var_count].a = DDATAOFF;
        var_area[var_count].sz = sz;
        DDATAOFF += abs(sz);
    }
    else {
        var_area[var_count].a = DATAOFF;
        var_area[var_count].sz = sz;
        DATAOFF += sz;
    }
    var_count++;
    
}

int get_var(char* s) {
    for (int i = 0;i<var_count;i++)
        if (strcmp(s, var_area[i].sym) == 0) {
            return var_area[i].a;
        }
    printf("\nVariable not found\n");
    exit(-20);
}

int get_size(int a) {
    for (int i = 0;i<var_count;i++)
        if (var_area[i].a == a) {
            return var_area[i].sz;
        }
    printf("\nNo variable stored in that mem location\n");
    exit(-21);
}

int get_lab(char* s) {
    for (int i = 0; i < lab_count; i++)
        if (strcmp(s, labels[i].sym) == 0) {
            return labels[i].a;
        }
    return -1;
}

/* Set a label entry */
int lab(char* s, int loc) {
    int i;

    if ((i=get_lab(s)) < 0) {
        strncpy(labels[lab_count].sym, s, 9);
        labels[lab_count].a = (loc < 0) ? lab_count : loc;
        lab_count++;
        return (lab_count - 1);
    }
    else {
        labels[i].a = loc;
        return i;
    }
}

void ldm(long long int d, long long int s) {
    if ((d >= DATAORIG) && (d <= DATAOFF))
        mem[d] = (d >= DATAORIG) && (d <= DATAOFF) ? mem[s] : mem[D(s)];
    else
        mem[D(d)] = mem[D(s)];
}

void sto(short a, long long int v) {
    if ((a >= DATAORIG) && (a <= DATAOFF)) {
        short a1 = 0;

        a1 = a;
        a1 += (indexes[pc] > 0) ? (short) mem[indexes[pc]] : 0;
        mem[a1] = (ind[pc] == 1) ?
            ((indexes[pc] > 0) ?
                mem[v] + mem[indexes[pc]]
                : mem[v])
            : v;
    }
    else
        mem[D(a)] = v;
}

void dsto(short a, long double v) {
    switch (a-DATAORIG) {
    case DR0:
        dr0 = v;
        break;
    case DR1:
        dr1 = v;
        break;
    }
}

void dadd(void) {
    dr0 += dr1;
}

void dsub(void) {
    dr0 -= dr1;
}

void dmul(void) {
    dr0 *= dr1;
}

void ddiv(void) {
    dr0 /= dr1;
}

void jmp(int ad) {
    if (ind[pc]) {
        pc = labels[ad].a - 1;
    }
    else {
        pc = ad - 1;
    }
}

void dec(short a, long long int v) {
    if ((a >= DATAORIG) && (a <= DATAOFF)) {
        long long int a1 = 0;

        a1 = a;
        a1 += (indexes[pc] > 0) ? mem[indexes[pc]] : 0;
        mem[a1] -= (ind[pc] == 1) ? mem[v] : v;
    }
    else
        mem[D(a)] -= v;
}

void inc(short a, long long int v) {
    if ((a >= DATAORIG) && (a <= DATAOFF)) {
        long long int a1 = 0;

        a1 = a;
        a1 += (indexes[pc] > 0) ? mem[indexes[pc]] : 0;
        mem[a1] += (ind[pc] == 1) ? mem[v] : v;
    }
    else
        mem[D(a)] += v;
}

void end(void) {
    pc = -2;
}

void cmp(short l1, short l2, char em) {
    short l11 = 0;
    short l22 = 0;
    long double dval = 0.0;
    //l2 = (em == DBL) ? l2 : (short) l2;
    l11 = ((l1 >= DATAORIG) && (l1 <= DATAOFF)) ? l1 : D(l1);
    l22 = ((l2 >= DATAORIG) && (l2 <= DATAOFF)) ? l2 : D(l2);
    
    switch (em) {
    case LSS:
        flags = mem[l11] < mem[l22];
        break;
    case GTT:
        flags = mem[l11] > mem[l22];
        break;
    case LSE:
        flags = mem[l11] <= mem[l22];
        break;
    case GTE:
        flags = mem[l11] >= mem[l22];
        break;
    case EQL:
        flags = mem[l11] == mem[l22];
        break;
    case DBL:
    /* Moving into DR0 or DR1 */
    /* Can be either a floating-point var or an integer var */
        if ((l1 == DR0 + DATAORIG) || (l1 == DR1 + DATAORIG)) {
            if (l2 >= 1024) {
                dval = (get_size(l2) < 0) ? dmem[l2] : mem[l2];
                dval += (indexes[pc] > 0) ? mem[indexes[pc]] : 0;
            } else
                dval = dmem[l2];
            dsto(l1, dval);
        }
        else {
            switch (l1 - DATAORIG) {
            case DADD:
                dadd();
                break;
            case DSUB:
                dsub();
                break;
            case DMUL:
                dmul();
                break;
            case DDIV:
                ddiv();
                break;
            case DMOV:
                /* no address conversion needed */
                l2 += (indexes[pc] > 0) ? (short) mem[indexes[pc]] : 0;
                dmem[l2] = dr0;
                break;
            }
        }
    }
    flags = flags << 2;
}

void jnz(int ad) {
    if ((flags >> 2) & 1) {
        if (ind[pc]) {
            pc = labels[ad].a - 1;
        }
        else {
            pc = ad - 1;
        }
    }
}

void pmem(int sz) {
    printf("\n");
    for (int i = 0; i < sz; i++) {
        printf("MEM\t[%d]:\t%lld\n", i, mem[i]);
    }

    printf("R0: %lld\n", r0);
    printf("R1: %lld\n", r1);

    printf("DR0: %Lf\n", dr0);
    printf("DR1: %Lf\n", dr1);

    for (int i = 0; i < sz; i++) {
        printf("DATA\t[%d]:\t%lld\n", DATAORIG + i, mem[DATAORIG + i]);
    }
    for (int i = 0; i < sz; ++i) {
        printf("FDATA\t[%d]:\t%Lf\n", 1024 + i, dmem[1024 + i]);
    }
    for (int i = 0; i < sz; i++) {
        printf("VAR\t[%d]:\t%s\tMEM[%d]\t%d\n", i, var_area[i].sym, var_area[i].a, var_area[i].sz);
    }
}

void prcols(int c, int sz) {
    printf("\n");
    int l1 = sz / c;
    int l2 = sz % c;

    printf("R0: %lld\n", r0);
    printf("R1: %lld\n", r1);
    printf("--------------\n");
    printf("DR0: %Lf\n", dr0);
    printf("DR1: %Lf\n", dr1);
    printf("--------------\n");

    printf("VARIABLES DUMP\n");
    for (int j = 0; j < l1; j++) {
        int pp = 0;
        for (int i = 0; i < c; i++) {
            pp = 0;
            if (var_area[i + c * j].sz) {
                printf("%s\t @%04d\t{%02d}\t", var_area[i + c * j].sym, var_area[i + c * j].a, var_area[i + c * j].sz);
                pp = 1;
            }
        }
        if (pp) printf("\n");
    }
    if (l2) {
        for (int j = 0; j < sz - c * l1; j++) {
            if (var_area[c * l1 + j].sz)
                printf("%s\t @%04d\t{%02d}\t", var_area[c * l1 + j].sym, var_area[c * l1 + j].a, var_area[c * l1 + j].sz);
        }
    }
    printf("DATA DUMP\n");
    for (int j = 0; j < l1; j++) {
        for (int i = 0; i < c; i++) {
            printf("[%02d]: % lld\t", DATAORIG + i + c * j, mem[DATAORIG + i + c * j]);
        }
        printf("\n");
    }
    if (l2) {
        for (int j = 0; j < sz - c * l1; j++) {
            printf("[%02d]: % lld\t", DATAORIG + j, mem[DATAORIG + c * l1 + j]);
        }
    }
    printf("--------------\n");
    printf("FP DATA DUMP\n");
    for (int j = 0; j < l1; j++) {
        for (int i = 0; i < c; i++) {
            printf("[%02d]: % lf\t", 1024 + i + c * j, dmem[1024 + i + c * j]);
        }
        printf("\n");
    }
    if (l2) {
        for (int j = 0; j < sz - c * l1; j++) {
            printf("[%02d]: % lf\t", 1024 + j, dmem[1024 + c * l1 + j]);
        }
    }
    printf("--------------\n");
    printf("MEMORY DUMP\n");
    for (int j = 0; j < l1; j++) {
        int pp = 0;
        for (int i = 0; i < c; i++) {
            pp = 0;
            if (mem[i + c * j]) {
                printf("[%02d]: %010lld\t", i + c * j, mem[i + c * j]);
                pp = 1;
            }
        }
        if (pp) printf("\n");
    }
    if (l2) {
        for (int j = 0; j < sz - c * l1; j++) {
            printf("[%02d]: %010lld\t", j, mem[c * l1 + j]);
        }
    }
    printf("--------------\n");
    printf("BIN MEMORY DUMP\n");
    for (int i = 0; i < sz; i++) {
        if (mem[i])
            printf("[%02d]: %s\n", i, itob(mem[i]));
    }
    printf("--------------\n");
//if l2 > 0:
//    for j in range(l - c * l1) :
//        print(d[c * l1 + j], end = '\t')
//        print("")
}
    

int readp(int a) {
    int inst = mem[a] & 7;
    short m = mem[a] >> 3 & 0xffff;
    long long int o = mem[a] >> VSHIFT;
    char ex = mem[a] >> 19 & 7;

    switch (inst) {
    case NOP:
        if ((m == 0) && (o == 0)) {
            end();
        }
        break;
    case STO:
        sto(m, o);
        break;
    case LDM:
        ldm(m, o);
        break;
    case JMP:
        jmp(m);
        break;
    case JNZ:
        jnz(m);
        break;
    case DEC:
        dec(m, o);
        break;
    case INC:
        inc(m, o);
        break;
    case CMP:
        cmp(m, (short) o, ex);
        break;
    default:
        return -1;
    }
    return 0;
}

long long int enc(int i, int m, long long int v) {
    long long int* ad = malloc(sizeof(long long int));
    if (ad == NULL) exit(-200);
    long long int r = 0;
    r += (long long int)v << VSHIFT;
    r += (short)(m) << 3;
    r += i;
    ad[0] = r;
    return *ad;
}

long long int enc4(int i, int m, long long int v, char ex) {
    long long int* ad = malloc(sizeof(long long int));
    if (ad == NULL) exit(-200);
    long long int r = 0;
    r += (long long int)v << VSHIFT;
    r += (char)(ex) << 19;
    r += (short)(m) << 3;
    r += i;
    ad[0] = r;
    return *ad;
}

long long int enc1(int i, int m) {
    long long int* ad = malloc(sizeof(long long int));
    if (ad == NULL) exit(-200);
    long long int r = 0;
    r += (short)(m) << 3;
    r += i;
    ad[0] = r;
    return *ad;
}

/* rudimental assembler */
void loadp(char* fn) {
    FILE* f;
    int e;
    int len = 80;
    char er15 = 0;
    char* l = malloc(len);
    char* tok = NULL;
    char* ltok = NULL;
    char* labline = malloc(20);
    char* varline = malloc(20);
    char* idx = malloc(10);
    char* instr = malloc(20);
    char* ipos = NULL;
    char* m = malloc(80);
    int m1 = 0;
    char* v = malloc(80);
    long long int v1 = 0;
    char* md = malloc(80);
    int loc = 0;
    int idx1 = 0;


    printf("\nReading program\n");
#ifdef _WIN32
    e = fopen_s(&f,fn, "r");
    if (e != 0) exit(-10);
#else
    f = fopen(fn,"r");
    if (f == NULL) exit(-10);
#endif
    while (fgets(l, len, f) != NULL) {
        printf("LINE\t(%d):\t%s", loc, l);
    
        /* get the first token */
        /* OPCODE */
        tok = strtok(l , "|");
        strcpy(instr, tok);

        /* process label */
        if ((ipos=strstr(instr, ":")) != NULL) {
            strcpy(labline, instr);
            char* res = strchr(labline, ':');
            strcpy(instr,res);
            labline[(res - labline)]='\0'; /* extract label name */
            DH(instr);
            lab(labline, loc);
        }

        if (strcmp(instr, "END") == 0) {
            mem[loc] = enc(NOP,0,0);
        }
        else if (strcmp(instr, "NOP") == 0) {
            mem[loc] = enc(NOP,1,1);
        }
        else if (strcmp(instr, "STO") == 0) {
            /* walk through other tokens */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                if ((ipos = strstr(m, "#")) != NULL) {
                    /* variable indexing */
                    strcpy(varline, m);
                    char* res = strchr(varline, '#');
                    strcpy(idx, res);
                    DH(idx);
                    if (isalpha(idx[0])) {
                        idx1 = get_var(idx);
                        indexes[loc] = idx1;
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m);
                    }
                    else {
                        idx1 = atoi(idx);
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m) + idx1;
                    }
                }
                /* Can be a registry */
                else if (m[0] == 'R') {
                    switch(m[1]) {
                        case '0':
                            m1 = -1;
                            break;
                        case '1':
                            m1 = -2;
                    }
                }
                else {
                    m1 = get_var(m);
                }
            }
            else
                m1 = atoi(m);

            tok = strtok(NULL, "|");
            strcpy(v, tok);
            idx1 = 0;
            if (isalpha(v[0])) {
                if ((ipos = strstr(v, "#")) != NULL) {
                                /* variable indexing */
                    strcpy(varline, v);
                    char* res = strchr(varline, '#');
                    strcpy(idx, res);
                    DH(idx);
                    if (isalpha(idx[0])) {
                        idx1 = get_var(idx);
                        indexes[loc] = idx1;
                    }
                    else
                        idx1 = atoi(idx);
                    varline[(res - varline)] = '\0';
                    strcpy(v, varline);
                    v1 = get_var(v) + idx1;
                    ind[loc] = 1;
                }
                else {
                    //DH(v); 
                    DT(v);
                    v1 = get_var(v);
                    ind[loc] = 1;
                }
            }
            else
                v1 = atoi(v);

            if (m1 < 0) v1 += 1;
            mem[loc] = enc(STO, m1, v1);
        }
        else if (strcmp(instr, "LDM") == 0) {
            /* walk through other tokens */
            /* LDM dest,src (can be memory loc or var) */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0]))
                m1 = get_var(m);
            else
                m1 = atoi(m);
            tok = strtok(NULL, "|");
            strcpy(v, tok);
            if (isalpha(v[0])) {
                //memmove(v, v + 1, strlen(v));
                DH(v); DT(v);
                //v[strlen(v) - 1] = '\0';
                v1 = get_var(v);
                ind[loc] = 1;
            }
            else
                v1 = atoi(v);
            mem[loc] = enc(LDM, m1, v1);
        }
        else if (strcmp(instr, "DEC") == 0) {
            /* walk through other tokens */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                if ((ipos = strstr(m, "#")) != NULL) {
                    /* variable indexing */
                    strcpy(varline, m);
                    char* res = strchr(varline, '#');
                    strcpy(idx, res);
                    DH(idx);
                    if (isalpha(idx[0])) {
                        idx1 = get_var(idx);
                        indexes[loc] = idx1;
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m);
                    }
                    else {
                        idx1 = atoi(idx);
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m) + idx1;
                    }
                }
                /* Can be a registry */
                else if (m[0] == 'R') {
                    switch(m[1]) {
                    case '0':
                        m1 = -1;
                        break;
                    case '1':
                        m1 = -2;
                    }
                }
                else {
                    m1 = get_var(m);
                }
            }
            else
                m1 = atoi(m);
            tok = strtok(NULL, "|");
            strcpy(v, tok);
            if (isalpha(v[0])) {
                //DH(v); 
                DT(v);
                
                v1 = get_var(v);
                ind[loc] = 1;
            }
            else
                v1 = atoi(v);

            if (m1 < 0) v1 += 1;
            mem[loc] = enc(DEC, m1, v1);
        }
        else if (strcmp(instr, "INC") == 0) {
            /* walk through other tokens */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                if ((ipos = strstr(m, "#")) != NULL) {
                    /* variable indexing */
                    strcpy(varline, m);
                    char* res = strchr(varline, '#');
                    strcpy(idx, res);
                    DH(idx);
                    if (isalpha(idx[0])) {
                        idx1 = get_var(idx);
                        indexes[loc] = idx1;
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m);
                    }
                    else {
                        idx1 = atoi(idx);
                        varline[(res - varline)] = '\0';
                        strcpy(m, varline);
                        m1 = get_var(m) + idx1;
                    }
                }
                /* Can be a registry */
                else if (m[0] == 'R') {
                    switch (m[1]) {
                    case '0':
                        m1 = -1;
                        break;
                    case '1':
                        m1 = -2;
                    }
                }
                else {
                    m1 = get_var(m);
                }
            }
            else
                m1 = atoi(m);
            tok = strtok(NULL, "|");
            strcpy(v, tok);
            if (isalpha(v[0])) {
                //DH(v); 
                DT(v);
                v1 = get_var(v);
                ind[loc] = 1;
            }
            else
                v1 = atoi(v);

            if (m1 < 0) v1 += 1;
            mem[loc] = enc(INC, m1, v1);
        }
        else if (strcmp(instr, "JMP") == 0) {
            /* walk through other tokens */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                //m[strlen(m) - 1] = '\0';
                DT(m);
                int lb = get_lab(m);
                if (lb < 0) {
                    m1 = lab(m, -1); /* create a place holder */
                                     /* m1 is the location in array */
                    ind[loc] = 1;
                }
                else {
                    m1 = lb;         /* label already present */
                                     /* m1 is the mem address */
                }
            }
            else
                m1 = atoi(m);
            mem[loc] = enc1(JMP, m1);
        }
        else if (strcmp(instr, "JNZ") == 0) {
            /* walk through other tokens */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                //m[strlen(m) - 1] = '\0';
                DT(m);
                int lb = get_lab(m);
                if (lb < 0) {
                    m1 = lab(m, -1); /* create a place holder */
                                     /* m1 is the location in array */
                    ind[loc] = 1;
                }
                else {
                    m1 = lb; /* label already present */
                                     /* m1 is the mem address */
                }
            }
            else
                m1 = atoi(m);
            mem[loc] = enc1(JNZ, m1);
        }
        else if (strcmp(instr, "CMP") == 0) {
            /* walk through other tokens */
            //r15 = 0;
            
            er15 = 0;
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            if (isalpha(m[0])) {
                /* Check if a DBL operation */
                if (strcmp(m, "DADD") == 0)
                    m1 = DADD + DATAORIG;
                else if (strcmp(m, "DSUB") == 0)
                    m1 = DSUB + DATAORIG;
                else if (strcmp(m, "DMUL") == 0)
                    m1 = DMUL + DATAORIG;
                else if (strcmp(m, "DDIV") == 0)
                    m1 = DDIV + DATAORIG;
                else if (strcmp(m, "DMOV") == 0)
                    m1 = DMOV + DATAORIG;
                else if (strcmp(m, "DR0") == 0)
                    m1 = DR0 + DATAORIG;
                else if (strcmp(m, "DR1") == 0)
                    m1 = DR1 + DATAORIG;
                else
                    m1 = get_var(m);
            }
            else
                m1 = atoi(m);

            /* Get the second argument */
            tok = strtok(NULL, "|");
            strcpy(v, tok);
            if (isalpha(v[0]))
                if ((ipos = strstr(v, "#")) != NULL) {
                    /* variable indexing */
                    strcpy(varline, v);
                    char* res = strchr(varline, '#');
                    strcpy(idx, res);
                    DH(idx);
                    if (isalpha(idx[0])) {
                        idx1 = get_var(idx);
                        indexes[loc] = idx1;
                        varline[(res - varline)] = '\0';
                        strcpy(v, varline);
                        v1 = get_var(v);
                    }
                    else {
                        /* numerical index */
                        idx1 = atoi(idx);
                        varline[(res - varline)] = '\0';
                        strcpy(v, varline);
                        v1 = get_var(v) + idx1;
                    }
                }
                else
                    v1 = get_var(v);
            else if (strchr(v, '.') != NULL) {
                if (((m1 - DATAORIG) == DR0) || ((m1 - DATAORIG) == DR1)) {
                    dmem[dmemp] = strtold(v, NULL);
                    v1 = dmemp;
                    dmemp++;
                }
                else {
                    dmem[get_var(m)] = strtold(v, NULL);
                    v1 = get_var(m);
                }
            }
            else
                v1 = atoi(v);
            tok = strtok(NULL, "|");

            /* Get the mode */
            strcpy(md, tok);
            DT(md);

            if (strcmp(md, STRFY(LSS)) == 0)
                er15 = (char)LSS;
            else if (strcmp(md, STRFY(GTT)) == 0)
                er15 = (char)GTT;
            else if (strcmp(md, STRFY(LSE)) == 0)
                er15 = (char)LSE;
            else if (strcmp(md, STRFY(GTE)) == 0)
                er15 = (char)GTE;
            else if (strcmp(md, STRFY(EQL)) == 0)
                er15 = (char)EQL;
            else if (strcmp(md, STRFY(DBL)) == 0)
                er15 = (char)DBL;
            else {
                printf("\nSyntax Error in CMP\n"); exit(-12);
            }

            mem[loc] = enc4(CMP, m1, v1, er15);
        }
        else if (strcmp(instr, "CALL") == 0) {
            /* calling func or proc */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            DT(m);
            mem[loc] = enc1(JMP, get_lab(m));
            mem[procs[get_lab(m)]] = enc1(JMP, loc+1);
        }
        else if (strcmp(instr, "RET") == 0) {
            /* returns from a proc */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            DT(m);
            procs[get_lab(m)] = STACKORIG + stackpos;
            mem[loc] = enc1(JMP, STACKORIG + stackpos);
            stackpos++;
        }
        else if (strcmp(instr, "VAR") == 0) {
            /* pseudo instruction */
            tok = strtok(NULL, "|");
            strcpy(m, tok);
            tok = strtok(NULL, "|");
            strcpy(v, tok);
            var(m, atoi(v));
            loc--;
        }
        loc++;
    }
    printf("\nEnd of listing\n");
    free(l);
    free(m);
    free(v);
    fclose(f);
}

int main(int argc, char **argv) {
    loadp(argv[1]);
    while (pc >= 0) {
        readp(pc);
        pc++;
    }
    //pmem(30);
    prcols(3, 30);
    return 0;
}
