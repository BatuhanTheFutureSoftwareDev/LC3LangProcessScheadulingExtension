#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm_dbg.h"

#define NOPS (16)

#define OPC(i) ((i) >> 12)
#define DR(i) (((i) >> 9) & 0x7)
#define SR1(i) (((i) >> 6) & 0x7)
#define SR2(i) ((i) & 0x7)
#define FIMM(i) ((i >> 5) & 01)
#define IMM(i) ((i) & 0x1F)
#define SEXTIMM(i) sext(IMM(i), 5)
#define FCND(i) (((i) >> 9) & 0x7)
#define POFF(i) sext((i) & 0x3F, 6)
#define POFF9(i) sext((i) & 0x1FF, 9)
#define POFF11(i) sext((i) & 0x7FF, 11)
#define FL(i) (((i) >> 11) & 1)
#define BR(i) (((i) >> 6) & 0x7)
#define TRP(i) ((i) & 0xFF)

/* New OS declarations */

// OS bookkeeping constants
#define PAGE_SIZE       (2048)  // Page size in words (of 2 bytes)
#define OS_MEM_SIZE     (2)     // OS Region size. Also the start of the page tables' page
#define Cur_Proc_ID     (0)     // id of the current process
#define Proc_Count      (1)     // total number of processes, including ones that finished executing.
#define OS_STATUS       (2)     // Bit 0 shows whether the PCB list is full or not
#define OS_FREE_BITMAP  (3)     // Bitmap for free pages

// Process list and PCB related constants
#define PCB_SIZE  (3)  // Number of fields in a PCB
#define PID_PCB   (0)  // Holds the pid for a process
#define PC_PCB    (1)  // Value of the program counter for the process
#define PTBR_PCB  (2)  // Page table base register for the process

#define CODE_SIZE       (2)  // Number of pages for the code segment
#define HEAP_INIT_SIZE  (2)  // Number of pages for the heap segment initially

bool running = true;

typedef void (*op_ex_f)(uint16_t i);
typedef void (*trp_ex_f)();

enum { trp_offset = 0x20 };
enum regist { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, PTBR, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };

uint16_t mem[UINT16_MAX + 1] = {0};
uint16_t reg[RCNT] = {0};
uint16_t PC_START = 0x3000;

void initOS();
int createProc(char *fname, char *hname);
void loadProc(uint16_t pid);
uint16_t allocMem(uint16_t ptbr, uint16_t vpn, uint16_t read, uint16_t write);  // Can use 'bool' instead
int freeMem(uint16_t ptr, uint16_t ptbr);
static inline uint16_t mr(uint16_t address);
static inline void mw(uint16_t address, uint16_t val);
static inline void tbrk();
static inline void thalt();
static inline void tyld();
static inline void trap(uint16_t i);

static inline uint16_t sext(uint16_t n, int b) { return ((n >> (b - 1)) & 1) ? (n | (0xFFFF << b)) : n; }
static inline void uf(enum regist r) {
    if (reg[r] == 0)
        reg[RCND] = FZ;
    else if (reg[r] >> 15)
        reg[RCND] = FN;
    else
        reg[RCND] = FP;
}
static inline void add(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void and(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void ldi(uint16_t i)  { reg[DR(i)] = mr(mr(reg[RPC]+POFF9(i))); uf(DR(i)); }
static inline void not(uint16_t i)  { reg[DR(i)]=~reg[SR1(i)]; uf(DR(i)); }
static inline void br(uint16_t i)   { if (reg[RCND] & FCND(i)) { reg[RPC] += POFF9(i); } }
static inline void jsr(uint16_t i)  { reg[R7] = reg[RPC]; reg[RPC] = (FL(i)) ? reg[RPC] + POFF11(i) : reg[BR(i)]; }
static inline void jmp(uint16_t i)  { reg[RPC] = reg[BR(i)]; }
static inline void ld(uint16_t i)   { reg[DR(i)] = mr(reg[RPC] + POFF9(i)); uf(DR(i)); }
static inline void ldr(uint16_t i)  { reg[DR(i)] = mr(reg[SR1(i)] + POFF(i)); uf(DR(i)); }
static inline void lea(uint16_t i)  { reg[DR(i)] =reg[RPC] + POFF9(i); uf(DR(i)); }
static inline void st(uint16_t i)   { mw(reg[RPC] + POFF9(i), reg[DR(i)]); }
static inline void sti(uint16_t i)  { mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]); }
static inline void str(uint16_t i)  { mw(reg[SR1(i)] + POFF(i), reg[DR(i)]); }
static inline void rti(uint16_t i)  {} // unused
static inline void res(uint16_t i)  {} // unused
static inline void tgetc()        { reg[R0] = getchar(); }
static inline void tout()         { fprintf(stdout, "%c", (char)reg[R0]); }
static inline void tputs() {
  uint16_t *p = mem + reg[R0];
  while(*p) {
    fprintf(stdout, "%c", (char) *p);
    p++;
  }
}
static inline void tin()      { reg[R0] = getchar(); fprintf(stdout, "%c", reg[R0]); }
static inline void tputsp()   { /* Not Implemented */ }
static inline void tinu16()   { fscanf(stdin, "%hu", &reg[R0]); }
static inline void toutu16()  { fprintf(stdout, "%hu\n", reg[R0]); }

trp_ex_f trp_ex[10] = {tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16, tyld, tbrk};
static inline void trap(uint16_t i) { trp_ex[TRP(i) - trp_offset](); }
op_ex_f op_ex[NOPS] = {/*0*/ br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap};

/**
  * Load an image file into memory.
  * @param fname the name of the file to load
  * @param offsets the offsets into memory to load the file
  * @param size the size of the file to load
*/
void ld_img(char *fname, uint16_t *offsets, uint16_t size) {
    FILE *in = fopen(fname, "rb");
    if (NULL == in) {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);
    }

    for (uint16_t s = 0; s < size; s += PAGE_SIZE) {
        uint16_t *p = mem + offsets[s / PAGE_SIZE];
        uint16_t writeSize = (size - s) > PAGE_SIZE ? PAGE_SIZE : (size - s);
        fread(p, sizeof(uint16_t), (writeSize), in);
    }
    
    fclose(in);
}

void run(char *code, char *heap) {
  while (running) {
    uint16_t i = mr(reg[RPC]++);
    op_ex[OPC(i)](i);
  }
}

#define OS_CUR_PID (0)
#define OS_PROC_COUNT (1)
#define OS_STATUS (2)
#define OS_BITMAP_0 (3)
#define OS_BITMAP_1 (4)
#define PCB_START (12)

int num_free_pages() {
  uint32_t bitmap = (mem[OS_BITMAP_0] << 16) | (mem[OS_BITMAP_1]);
  int free_pages = 0;
  for(int i = 0; i < 32; i++, bitmap = bitmap >> 1) {
    if (bitmap & 0x0001) {
      free_pages++;
      if (free_pages >= 4) {
        return free_pages;
      }
    }
  }
  return free_pages;
}

void initOS() {
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F
    // initializations
    mem[OS_CUR_PID] = 0xffff;
    mem[OS_PROC_COUNT] = 0x0000; // or 0
    mem[OS_STATUS] = 0x0000;
    // 111...111000 first 3 frames allocated, 1 free 0 allocated
    mem[OS_BITMAP_0] = 0x1FFF;
    mem[OS_BITMAP_1] = 0xFFFF;
    // PCB starts at 12
}

// after incrementing procCount, check if it is less than or equal to page size / # of pages
// Process functions to implement
int createProc(char *fname, char *hname) { // we will append to PCB as well, ...
  if (mem[OS_STATUS] & 0x0001) {
    printf("The OS memory region is full. Cannot create a new PCB.\n");
    return 0;
  }
  int free_pages = num_free_pages();
  if (free_pages < 2) {
    printf("Cannot create code segment.\n");
    return 0;
  } else if (free_pages < 4) {
    printf("Cannot create heap segment.\n");
    return 0;
  }
  // create PCB
  uint16_t pid = mem[OS_PROC_COUNT]++;
  if (pid >= PAGE_SIZE / 32)
    mem[OS_STATUS] = 0x0001;
  
  mem[pid * 3 + 12] = pid;
  mem[pid * 3 + 13] = 0x3000; // PC
  uint16_t ptbr = 2 * PAGE_SIZE + pid * 32;
  mem[pid * 3 + 14] = ptbr; // PTBR

  for (int k = 0; k < 32; k++) mem[ptbr + k] = 0;

  if ( !(allocMem(ptbr, 6, 0xFFFF, 0x0000)) ) {
      // mem[OS_PROC_COUNT]--;
      mem[pid * 3 + 12] = 0xFFFF;
      return 0;
  }
  if ( !(allocMem(ptbr, 7, 0xFFFF, 0x0000)) ) { // do we roll back proc count?
      // mem[OS_PROC_COUNT]--;
      mem[pid * 3 + 12] = 0xFFFF;
      freeMem(6, ptbr); // Rollback
      return 0;
  } 

  if ( !(allocMem(ptbr, 8, 0xFFFF, 0xFFFF)) ) {
      // mem[OS_PROC_COUNT]--;
      mem[pid * 3 + 12] = 0xFFFF;
      freeMem(6, ptbr); freeMem(7, ptbr); // Rollback
      return 0;
  }
  if ( !(allocMem(ptbr, 9, 0xFFFF, 0xFFFF)) ) {
      // mem[OS_PROC_COUNT]--;
      mem[pid * 3 + 12] = 0xFFFF;
      freeMem(6, ptbr); freeMem(7, ptbr); freeMem(8, ptbr); // Rollback
      return 0;
  }  

  uint16_t offsets[2] = { (mem[ptbr + 6] >> 11) * PAGE_SIZE, (mem[ptbr + 7] >> 11) * PAGE_SIZE }; // not finished
  ld_img(fname, offsets, 2 * PAGE_SIZE);
  offsets[0] = (mem[ptbr + 8] >> 11) * PAGE_SIZE;
  offsets[1] = (mem[ptbr + 9] >> 11) * PAGE_SIZE;
  ld_img(hname, offsets, 2 * PAGE_SIZE);
  return 1;
}

void loadProc(uint16_t pid) { // assuming the pid is valid: can validate with pid < mem[procCount] though
  reg[PTBR] = mem[pid * 3 + 14];
  reg[RPC] = mem[pid * 3 + 13];
  mem[OS_CUR_PID] = pid;
}

uint16_t allocMem(uint16_t ptbr, uint16_t vpn, uint16_t read, uint16_t write) { // returns the PFN?
  if (mem[ptbr + vpn] & 0x0001) {
      return 0;
  }
  uint32_t bitmap = (mem[OS_BITMAP_0] << 16) | (mem[OS_BITMAP_1]);
  uint32_t mask = 0x80000000;
  for (int i = 0; i < 32; i++, mask = mask >> 1) {
    if (bitmap & mask) {
      // found a free page at PFN i
      mem[ptbr + vpn] = (i << 11) | 0x0001; 
      if (read == 0xFFFF) mem[ptbr + vpn] |= 0x0002;
      if (write == 0xFFFF) mem[ptbr + vpn] |= 0x0004;
      
      // update global bitmap
      bitmap &= ~mask;
      
      mem[OS_BITMAP_0] = (bitmap >> 16) & 0xFFFF;
      mem[OS_BITMAP_1] = bitmap & 0xFFFF;
      return 1;
    }
  }
  return 0;
}

int freeMem(uint16_t vpn, uint16_t ptbr) { 
  uint16_t pfn = mem[ptbr + vpn] >> 11;
  uint32_t bitmap = (mem[OS_BITMAP_0] << 16) | (mem[OS_BITMAP_1]);
  uint32_t mask = 0x80000000 >> pfn;
  if ( !(bitmap & mask) ) { // if allocated 
    mem[ptbr + vpn] &= 0xFFFE; // now invalid
    bitmap |= mask;
    mem[OS_BITMAP_0] = (bitmap >> 16) & 0xFFFF;
    mem[OS_BITMAP_1] = bitmap & 0xFFFF;
    return 1;
  }
  return 0;
}

static inline void tbrk() {
  uint16_t vpn = reg[R0] >> 11;
  if ( vpn < 6) {
    printf("Cannot allocate/free memory for the reserved segment.\n");
    thalt();
    return;
  }
  uint16_t flags = reg[R0] & 0x0007;
  if (flags & 0x0001) { // allocate block
    printf("Heap increase requested by process %d.\n", mem[OS_CUR_PID]);
    if (mem[reg[PTBR] + vpn] & 0x0001) {
      printf("Cannot allocate memory for page %d of pid %d since it is already allocated.\n", vpn, mem[OS_CUR_PID]);
      return;
    } 
    if (!num_free_pages()) {
      printf("Cannot allocate more space for pid %d since there is no free page frames.\n", mem[OS_CUR_PID]);
      return;
    }
    uint16_t r_perm = (flags & 0x0002) ? 0xFFFF : 0;
    uint16_t w_perm = (flags & 0x0004) ? 0xFFFF : 0;
    if ( !(allocMem(reg[PTBR], vpn, r_perm, w_perm)) ) {
      // alloc mem failed
      return;
    }
  } else { // free block
    printf("Heap decrease requested by process %d.\n", mem[OS_CUR_PID]);
    if (!(mem[reg[PTBR] + vpn] & 0x0001)) {
      printf("Cannot free memory of page %d of pid %d since it is not allocated.\n", vpn, mem[OS_CUR_PID]);
      return;
    }
    if (!freeMem(vpn, reg[PTBR])) {
      // free mem failed
      return;
    }
  }
}

static inline void tyld() { 
  uint16_t procCount = mem[OS_PROC_COUNT];
  uint16_t cur_pid = (mem[OS_CUR_PID] + 1) % procCount;  
  for (int i = 0; i < procCount; i++, cur_pid = (cur_pid + 1) % procCount) { // procCount - 1
    if (mem[cur_pid * 3 + 12] != 0xFFFF) {
      break;
    }
  }
  if (mem[cur_pid * 3 + 12] == 0xFFFF) {
    // Probably won't ever run, since we do a full loop and will
    // come back to ourselves (which is guaranteed) to not be 0xFFFF
  } else if (cur_pid == mem[OS_CUR_PID]) {
    // dont do anything, you are the only process alive
  } else {
    mem[PCB_START + mem[OS_CUR_PID] * 3 + 1] = reg[RPC];
    printf("We are switching from process %d to %d.\n", mem[OS_CUR_PID], cur_pid);
    loadProc(cur_pid);
  }
}

static inline void thalt() {
  for(int v = 6; v < 32; v++) {
      if(mem[reg[PTBR] + v] & 0x0001) {
          freeMem(v, reg[PTBR]);
      }
  }

  mem[mem[OS_CUR_PID] * 3 + 12] = 0xFFFF; // mark current proc as terminated
  uint16_t procCount = mem[OS_PROC_COUNT];
  uint16_t cur_pid = (mem[OS_CUR_PID] + 1) % procCount;

  for (int i = 0; i < procCount; i++, cur_pid = (cur_pid + 1) % procCount) { // maybe procCount - 1
    if (mem[cur_pid * 3 + 12] != 0xFFFF) {
      break;
    }
  }
  if (mem[cur_pid * 3 + 12] != 0xFFFF) {
    // run cur_pid
    // printf("Halt: We are switching from process %d to %d.\n", mem[OS_CUR_PID], cur_pid);
    // WHY WOULD WE NOT PRINT THIS?? 
    loadProc(cur_pid);
  } else {
    running = false; 
  }
}

static inline uint16_t mr(uint16_t address) { 
    if (address < 0x3000) {
      printf("Segmentation fault.\n");
      exit(1);
    } 
    uint16_t PTE = mem[reg[PTBR] + (address >> 11)];
    if ((PTE & 0x0001) == 0) {
      printf("Segmentation fault inside free space.\n");
      exit(1);
    } 
    if ((PTE & 0x0002) == 0) {
      printf("Cannot read the page.\n");
      exit(1);
    }
    return mem[(PTE & 0xF800) | (address & 0x07FF)];
}

static inline void mw(uint16_t address, uint16_t val) {
    if (address < 0x3000) {
      printf("Segmentation fault.\n");
      exit(1);
    }
    uint16_t PTE = mem[reg[PTBR] + (address >> 11)];
    if ((PTE & 0x0001) == 0) {
      printf("Segmentation fault inside free space.\n");
      exit(1);    
    }
    if ((PTE & 0x0004) == 0) {
      printf("Cannot write to a read-only page.\n");
      exit(1);
    }
    mem[(PTE & 0xF800) | (address & 0x07FF)] = val;
}
