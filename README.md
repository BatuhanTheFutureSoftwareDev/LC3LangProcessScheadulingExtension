# LC-3 Virtual Machine with Paging

An extension of an LC-3 virtual machine that adds a full paging-based virtual memory system with multi-process support. Implemented as part of CS 307 – Operating Systems at Sabancı University.

## Overview

The base VM executes programs written in the LC-3 instruction set. This project extends it by separating virtual address spaces from physical memory through paging, enabling multiple processes to run concurrently with proper isolation.

All implementation work is in `vm.c`.

## What I Implemented

**Address Translation (`mr` / `mw`)**  
Memory reads and writes now go through a page table lookup using the PTBR register. The 16-bit virtual address is split into a 5-bit VPN and 11-bit offset. Accesses to reserved regions or unmapped pages trigger segmentation faults; protection bits enforce read/write permissions.

**OS Initialization (`initOS`)**  
Sets up the OS region of physical memory: initializes `curProcID`, `procCount`, `OSStatus`, and a 32-bit page bitmap tracking free/used page frames.

**Page Allocation / Deallocation (`allocMem` / `freeMem`)**  
`allocMem` finds the first free frame via linear bitmap scan and fills the corresponding PTE with PFN and protection bits. `freeMem` clears the valid bit and marks the frame free — without touching page contents, preserving footprints for inspection.

**Process Creation & Loading (`createProc` / `loadProc`)**  
`createProc` assigns a PID, writes a PCB, allocates 2 code pages and 2 heap pages, and loads object files into physical memory. Handles failure cases (full OS region, insufficient frames) with appropriate cleanup. `loadProc` restores a process's PC and PTBR from its PCB.

**Context Switching (`tyld` trap — `0x28`)**  
Saves the current process's PC and PTBR to its PCB, then scans forward for the next runnable process and restores its state. Wraps around and stays on the current process if it's the only one running.

**Halt with Multi-Process Awareness (`thalt` trap)**  
Frees all pages of the terminating process, marks its PCB as terminated (`PID_PCB = 0xffff`), and transfers control to the next runnable process. The simulation ends only when all processes have halted.

**Dynamic Heap Management (`tbrk` trap — `0x29`)**  
Reads VPN and access flags from R0. Allocates or frees a single heap page on request, updating both the page table and the bitmap. Guards against double-allocation, double-free, and attempts to touch OS-reserved memory.

## Memory Layout

```
Physical Address Space (128KB, word-addressed uint16_t)
┌─────────────────┐  0x0000
│   OS Region     │  curProcID, procCount, OSStatus,
│   (8KB)         │  page bitmap, PCBs
├─────────────────┐  0x1000
│   Page Tables   │  One table per process, 32 PTEs each
│   (4KB)         │
├─────────────────┐  0x1800
│  User Frames    │  Code & heap pages, allocated on demand
│  (rest)         │
└─────────────────┘  0xFFFF

PTE layout (16-bit):
[ PFN(5) | padding(8) | write(1) | read(1) | valid(1) ]
```

## Building & Running

```bash
# Build everything (VM binary + test programs + sample object files)
make

# Run a single process
./vm code.obj heap.obj

# Run two processes concurrently
./vm code1.obj heap1.obj code2.obj heap2.obj
```

## Tests & Samples

Unit tests live in `tests/` and sample scripts in `bin_samples/` and `lang_samples/`.

```bash
# Unit test example
tests/initos-test

# Sample run with diff check
lang_samples/lang_sample1.sh > my_out.txt
diff lang_samples/lang_sample1-result.txt my_out.txt
```

## Tech

C · LC-3 ISA · Paging · Virtual Memory · OS Concepts