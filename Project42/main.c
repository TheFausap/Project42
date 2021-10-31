#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MEMSIZE 32767
#define DATAORIG 2048

#define VSHIFT 20

long long int mem[MEMSIZE] = { 0 };
int DATAOFF = DATAORIG;       /* max prog size in byte */
int pc = 0;
int r15 = 0;
char flags = 0;
char ind[DATAORIG] = { 0 };
int indexes[DATAORIG] = { 0 };

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
LE labels[50] = { 0,0 };
int var_count = 0;
int lab_count = 0;

enum {END=0,STO,LDM,JMP,DEC,INC,JNZ,CMP};
#define LSS 0
#define GTT 1
#define LSE 2
#define GTE 3
#define EQL 4
#define STRFY(x) #x

#define D(x) DATAOFF+(x)
#define D1(x) DATAORIG+(x)

#define DH(x) memmove((x), (x) + 1, strlen((x)))
#define DT(x) (x)[strlen((x)) - 1] = '\0'

void var(char* s, int sz) {
	strncpy(var_area[var_count].sym, s, 9);
	var_area[var_count].a = DATAOFF;
	var_area[var_count].sz = sz;
	DATAOFF += sz;
	var_count++;
}

int get_var(char* s) {
	for (int i = 0;i<var_count;i++) 
		if (strcmp(s, var_area[i].sym) == 0) {
			return var_area[i].a;
		}
	printf("Variable not found\n");
	exit(-20);
}

int get_lab(char* s) {
	for (int i = 0; i < lab_count; i++)
		if (strcmp(s, labels[i].sym) == 0) {
			return labels[i].a;
		}
	return -1;
}

int lab(char* s, int loc) {
	int i;

	if ((i=get_lab(s)) < 0) {
		strncpy(labels[lab_count].sym, s, 9);
		labels[lab_count].a = loc;
		lab_count++;
		return (lab_count - 1);
	}
	else {
		labels[i].a = loc;
		return i;
	}
}

void ldm(long long int d, long long int s) {
	mem[D(d)]=mem[D(s)];
}

void sto(int a, long long int v) {
	if ((a >= DATAORIG) && (a <= DATAOFF)) {
		long long int a1 = 0;

		a1 = a;
		a1 += (indexes[pc] > 0) ? mem[indexes[pc]] : 0;
		mem[a1] = (ind[pc] == 1) ?
			((indexes[pc] > 0) ?
				mem[v] + mem[indexes[pc]]
				: mem[v])
			: v;
	}
	else
		mem[D(a)] = v;
}

void jmp(int ad) {
	if (ind[pc]) {
		pc = labels[ad].a - 1;
	}
	else {
		pc = ad - 1;
	}
}

void dec(int a, long long int v) {
	if ((a >= DATAORIG) && (a <= DATAOFF))
		mem[a] -= (ind[pc] == 1) ? mem[v] : v;
	else
		mem[D(a)] -= v;
}

void inc(int a, long long int v) {
	if ((a >= DATAORIG) && (a <= DATAOFF))
		mem[a] += (ind[pc] == 1) ? mem[v] : v;
	else
		mem[D(a)] += v;
}

void end() {
	pc = -2;
}

void cmp(long long int l1, long long int l2, int m) {
	long long int l11 = 0;
	long long int l22 = 0;
	l11 = ((l1 >= DATAORIG) && (l1 <= DATAOFF)) ? l1 : D(l1);
	l22 = ((l2 >= DATAORIG) && (l2 <= DATAOFF)) ? l2 : D(l2);
	switch (m) {
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
	for (int i = 0; i < sz; i++) {
		printf("DATA\t[%d]:\t%lld\n", i, mem[DATAORIG+i]);
	}
	for (int i = 0; i < sz; i++) {
		printf("VAR\t[%d]:\t%s\tMEM[%d]\t%d\n", i, var_area[i].sym, var_area[i].a, var_area[i].sz);
	}
}

int readp(int a) {
	int inst = mem[a] & 7;
	if (!inst) { end(); return 0; }
	int m = mem[a] >> 3 & 0xffff;
	long long int o = mem[a] >> VSHIFT;

	switch (inst) {
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
		cmp(m, o, r15);
		break;
	default:
		return -1;
	}
	return 0;
}

long long int enc(int i, int m, long long int v) {
	long long int* ad = malloc(sizeof(long long int));
	long long int r = 0;
	r += (long long int)v << VSHIFT;
	r += (short) (m << 3);
	r += i;
	ad[0] = r;
	return *ad;
}

long long int enc1(int i, int m) {
	long long int* ad = malloc(sizeof(long long int));
	long long int r = 0;
	r += (short) (m << 3);
	r += i;
	ad[0] = r;
	return *ad;
}

/* rudimental assembler */
void loadp() {
	FILE* f;
	int e;
	int len = 80;
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
	e = fopen_s(&f,"test.asm", "r");
	if (e != 0) exit(-10);
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
			break;
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
					}
					else
						idx1 = atoi(idx);
					varline[(res - varline)] = '\0';
					strcpy(m, varline);
					m1 = get_var(m);
				}
				else {
					m1 = get_var(m);
				}
			}
			else 
				m1 = atoi(m);

			tok = strtok(NULL, "|");
			strcpy(v, tok);
			if (ispunct(v[0])) {
				if ((ipos = strstr(v, "#")) != NULL) {
                    /* variable indexing */
					strcpy(varline, v);
					char* res = strchr(varline, '#');
					strcpy(idx, res);
					DH(idx);
					if (isalpha(idx[0]))
						idx1 = get_var(idx);
					else
						idx1 = atoi(idx);
					varline[(res - varline)] = '\0';
					strcpy(v, varline);
					v1 = get_var(v) + mem[idx1];
					ind[loc] = 1;
				}
				else {
					//memmove(v, v + 1, strlen(v));
					DH(v); DT(v);
					//v[strlen(v) - 1] = '\0';
					v1 = get_var(v);
					ind[loc] = 1;
				}
			}
			else
				v1 = atoi(v);

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
				m1 = get_var(m);
			}
			else
				m1 = atoi(m);
			tok = strtok(NULL, "|");
			strcpy(v, tok);
			if (ispunct(v[0])) {
				//memmove(v, v + 1, strlen(v));
				DH(v); DT(v);
				//v[strlen(v) - 1] = '\0';
				v1 = get_var(v);
				ind[loc] = 1;
			}
			else
				v1 = atoi(v);
			mem[loc] = enc(DEC, m1, v1);
		}
		else if (strcmp(instr, "INC") == 0) {
			/* walk through other tokens */
			tok = strtok(NULL, "|");
			strcpy(m, tok);
			if (isalpha(m[0])) {
				m1 = get_var(m);
			}
			else
				m1 = atoi(m);
			tok = strtok(NULL, "|");
			strcpy(v, tok);
			if (ispunct(v[0])) {
				//memmove(v, v + 1, strlen(v));
				DH(v); DT(v);
				//v[strlen(v) - 1] = '\0';
				v1 = get_var(v);
				ind[loc] = 1;
			}
			else
				v1 = atoi(v);
			mem[loc] = enc(INC, m1, v1);
		}
		else if (strcmp(instr, "JMP") == 0) {
			/* walk through other tokens */
			tok = strtok(NULL, "|");
			strcpy(m, tok);
			if (isalpha(m[0])) {
				//m[strlen(m) - 1] = '\0';
				DT(m);
				if (get_lab(m) < 0) {
					m1 = lab(m, -1); /* create a place holder */
					                 /* m1 is the location in array */
					ind[loc] = 1;
				}
				else {
					m1 = get_lab(m); /* label already present */
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
				if (get_lab(m) < 0) {
					m1 = lab(m, -1); /* create a place holder */
									 /* m1 is the location in array */
					ind[loc] = 1;
				}
				else {
					m1 = get_lab(m); /* label already present */
									 /* m1 is the mem address */
				}
			}
			else
				m1 = atoi(m);
			mem[loc] = enc1(JNZ, m1);
		}
		else if (strcmp(instr, "CMP") == 0) {
			/* uses r15 registry. destroyed each time CMP is called */
			/* walk through other tokens */
			r15 = 0;
			tok = strtok(NULL, "|");
			strcpy(m, tok);
			if (isalpha(m[0])) {
				m1 = get_var(m);
			}
			else
				m1 = atoi(m);
			tok = strtok(NULL, "|");
			strcpy(v, tok);
			if (isalpha(v[0]))
				v1 = get_var(v);
			else
				v1 = atoi(v);
			tok = strtok(NULL, "|");
			strcpy(md, tok);
			//md[strlen(md) - 1] = '\0';
			DT(md);

			mem[loc] = enc(CMP, m1, v1);

			if (strcmp(md, STRFY(LSS)) == 0)
				r15 = (char)LSS;
			else if (strcmp(md, STRFY(GTT)) == 0)
				r15 = (char)GTT;
			else if (strcmp(md, STRFY(LSE)) == 0)
				r15 = (char)LSE;
			else if (strcmp(md, STRFY(GTE)) == 0)
				r15 = (char)GTE;
			else if (strcmp(md, STRFY(EQL)) == 0)
				r15 = (char)EQL;
			else {
				printf("Syntax Error in CMP\n"); exit(-12);
			}
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

int main() {
	loadp();
	while (pc >= 0) {
		readp(pc);
		pc++;
	}
	pmem(25);
	return 0;
}