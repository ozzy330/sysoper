// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "translate.h"

#define UserStackSize 1024 // increase this as necessary!

class AddrSpace {
public:
  // Create an address space,
  // initializing it with the program
  // stored in the file "executable"
  AddrSpace(OpenFile *executable);

  // INFO: copy data for some AddrSpace but creates a new stack
  AddrSpace(const AddrSpace &source);

  // De-allocate an address space
  ~AddrSpace();
  // Initialize user-level CPU registers,
  // before jumping to user code
  void InitRegisters();

  // Save/restore address space-specific
  void SaveState();

  // info on a context switch
  void RestoreState();

  // Devuelve la pagina con sus meta datos
  TranslationEntry *EntryFromVirtPage(unsigned virtpage);
  TranslationEntry *EntryFromPhysPage(unsigned physpage);
  void printPT();

private:
  // Assume linear page table translation for now!
  TranslationEntry *pageTable;

  // Number of pages in the virtual
  // address space
  unsigned int numPages;
};

#endif // ADDRSPACE_H
