// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "bitmap.h"
#include "copyright.h"
#include "machine.h"
#include "noff.h"
#include "system.h"
#include "utility.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
  noffH->noffMagic = WordToHost(noffH->noffMagic);
  noffH->code.size = WordToHost(noffH->code.size);
  noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
  noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
  noffH->initData.size = WordToHost(noffH->initData.size);
  noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
  noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
  noffH->uninitData.size = WordToHost(noffH->uninitData.size);
  noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
  noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

void ReadPagesAtMachine(OpenFile *executable, int firstPage, int size,
                        int startFileAddr, TranslationEntry *pageTable,
                        int numPages) {
  int page, sector, file_offset;
  // char buff[SectorSize];

  for (int i = firstPage; i < firstPage + numPages; i++) {
    page = pageTable[i].physicalPage * PageSize;
    sector = pageTable[i].swapSector * PageSize;
    file_offset = startFileAddr + (i * PageSize);
    // Lee de disco
    // executable->ReadAt(buff, PageSize, file_offset);
#ifndef VM
    // Escribe en marco
    executable->ReadAt(&(machine->mainMemory[page]), PageSize, file_offset);
    DEBUG('x', "En memoria %d [", pageTable[i].physicalPage);
    for (int offset = 0; offset < SectorSize; offset++) {
      DEBUG('x', "%x", machine->mainMemory[page + offset]);
    }
    DEBUG('x', "]\n");
#else
    executable->ReadAt(&(swapSpace[sector]), SectorSize, file_offset);
    DEBUG('x', "En swap %d [", pageTable[i].swapSector);
    for (int offset = 0; offset < SectorSize; offset++) {
      DEBUG('x', "%x", swapSpace[sector + offset]);
    }
    DEBUG('x', "]\n");
#endif
  }
  stats->numDiskWrites += numPages;
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
AddrSpace::AddrSpace(OpenFile *executable) {
  NoffHeader noffH;
  unsigned int i, size;

  // NOTE: Lee el archivo desde el inicio del disco (0)
  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);

  // NOTE: pasa los headers a big endian de ser necesario
  if ((noffH.noffMagic != NOFFMAGIC) &&
      (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
    SwapHeader(&noffH);
  }
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  // how big is address space?
  // we need to increase the size to leave room for the stack
  // size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
  //        UserStackSize;
  // numPages = divRoundUp(size, PageSize);
  // size = numPages * PageSize;

  // NOTE: tamaño de página de 128 bytes (Sectorsize)
  // INFO: nueva forma de generar las páginas logicas
  int numCodePages = divRoundUp(noffH.code.size, PageSize);
  int numDataPages = divRoundUp(noffH.initData.size, PageSize);
  int numUninitDataPages = divRoundUp(noffH.uninitData.size, PageSize);
  int numStackPages = divRoundUp(UserStackSize, PageSize);
  numPages = numCodePages + numDataPages + numUninitDataPages + numStackPages;
#ifndef VM
  // check we're not trying to run anything too big -- at least until we have
  // virtual memory
  ASSERT(numPages <= NumPhysPages);
#endif
  size = numPages * PageSize;

  DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
        size);

  // first, set up the translation
  this->pageTable = new TranslationEntry[numPages];

  // WARN: ensuciando páginas
  // for (int pagina = 0; pagina <= 10; pagina+=2) {
  //   MapitaBits->Mark(pagina);
  // }

  for (i = 0; i < numPages; i++) {
    this->pageTable[i].virtualPage = i;
#ifndef VM
    this->pageTable[i].swapSector = -1;
    this->pageTable[i].physicalPage = MapitaBits->SecureFind();
    this->pageTable[i].valid = true;
#else
    this->pageTable[i].swapSector = swapSectors->SecureFind();
    this->pageTable[i].physicalPage = -1;
    // NOTE: VM siempre inicia como falsa
    this->pageTable[i].valid = false;
#endif
    // if the code segment was entirely on
    // a separate page, we could set its
    // pages to be read-only
    this->pageTable[i].readOnly = false;
    this->pageTable[i].use = false;
    this->pageTable[i].dirty = false;
  }

  // zero out the entire address space, to zero the unitialized data segment and
  // the stack segment
  // bzero(machine->mainMemory, size);

  // then, copy in the code and data segments into memory
  if (noffH.code.size > 0) {
    ReadPagesAtMachine(executable, 0, noffH.code.size, noffH.code.inFileAddr,
                       this->pageTable, numCodePages);
  }
  if (noffH.initData.size > 0) {
    ReadPagesAtMachine(executable, numCodePages, noffH.initData.size,
                       noffH.initData.inFileAddr, this->pageTable,
                       numDataPages);
  }
  for (i = 0; i < numPages; i++) {
    DEBUG('g', "[NEW] VPAG: %d, PAG: %d SEC: %d\n", pageTable[i].virtualPage,
          pageTable[i].physicalPage, pageTable[i].swapSector);
    // #ifdef VM
    //     DEBUG('g', "Escrito a swap [%d]", i);
    //     for (int offset = 0; offset < SectorSize; offset++) {
    //       DEBUG('g', "%x", swapSpace[pageTable[i].swapSector + offset]);
    //     }
    //     DEBUG('g', "]\n");
    // #else
    //     DEBUG('g', "Escrito a memoria [%d]", i);
    //     for (int offset = 0; offset < SectorSize; offset++) {
    //       DEBUG('g', "%x", machine->mainMemory[pageTable[i].physicalPage +
    //       offset]);
    //     }
    //     DEBUG('g', "]\n");
    // #endif
  }
}
// Crea una copia de los segmentos compartidos y crea un nuevo stack
AddrSpace::AddrSpace(const AddrSpace &source) : numPages(source.numPages) {
  // Calcula el tamño del stack
  int numStackPages = divRoundUp(UserStackSize, PageSize);
  this->numPages += numStackPages;

// check we're not trying to run anything too big -- at least until we have
// virtual memory
#ifndef VM
  ASSERT(numPages <= NumPhysPages);
#endif
  this->pageTable = new TranslationEntry[numPages];

  memcpy(pageTable, source.pageTable,
         sizeof(TranslationEntry) * source.numPages);

  int i;
  for (i = numPages - numStackPages; i < numPages; i++) {
    this->pageTable[i].virtualPage = i;
#ifndef VM
    this->pageTable[i].swapSector = -1;
    this->pageTable[i].physicalPage = MapitaBits->SecureFind();
    this->pageTable[i].valid = true;
#else
    this->pageTable[i].swapSector = swapSectors->SecureFind();
    this->pageTable[i].physicalPage = -1;
    // NOTE: VM siempre inicia como falsa
    this->pageTable[i].valid = false;
#endif
    this->pageTable[i].readOnly = false;
    this->pageTable[i].use = false;
    this->pageTable[i].dirty = false;
  }

  DEBUG('1', "PT FOR %s:\n", currentThread->getName());
  for (i = 0; i < currentThread->space->numPages; i++) {
    TranslationEntry pt = *currentThread->space->EntryFromVirtPage(i);
    DEBUG('1',
          "[%d] VP %d | PP %d | SWP %d | VAL %d | DTY %d : ",i, pt.virtualPage,
          pt.physicalPage, pt.swapSector, pt.valid, pt.dirty);
    if (pt.valid == true) {
      for (int j = 0; j < 15; j++) {
        DEBUG('1', "%x", machine->mainMemory[pt.physicalPage * PageSize + j]);
      }
    }
    DEBUG('1', "\n");
  }

  for (i = 0; i < numPages; i++) {
    DEBUG('g', "[CPY] VPAG: %d, PAG: %d SEC: %d\n", pageTable[i].virtualPage,
          pageTable[i].physicalPage, pageTable[i].swapSector);
    // #ifdef VM
    //     DEBUG('g', "Escrito a swap [%d]", i);
    //     for (int offset = 0; offset < SectorSize; offset++) {
    //       DEBUG('g', "%x", swapSpace[pageTable[i].swapSector + offset]);
    //     }
    //     DEBUG('g', "]\n");
    // #else
    //     DEBUG('g', "Escrito a memoria [%d]", i);
    //     for (int offset = 0; offset < SectorSize; offset++) {
    //       DEBUG('g', "%x", machine->mainMemory[pageTable[i].physicalPage +
    //       offset]);
    //     }
    //     DEBUG('g', "]\n");
    // #endif
  }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
  DEBUG('x', "Marcando memoria como libre\n");
  // Marca como libre el espacio de memoria que se ocupaba
  for (int page = 0; page < numPages; page++) {
    if (this->pageTable[page].valid) {
      MapitaBits->SecureClear(this->pageTable[page].physicalPage);
    }
#ifdef VM
    swapSectors->SecureClear(this->pageTable[page].swapSector);
#endif
  }
  DEBUG('y', "\t||| DELETING ADDRESS SPACE ... {%s}\n",
        currentThread->getName());
  delete this->pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
  int i;

  for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

  // Set the stack register to the end of the address space, where we
  // allocated the stack; but subtract off a bit, to make sure we don't
  // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {
  DEBUG('1', "\t||| SAVING ... {%s}\n", currentThread->getName());
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

// WARN: VM problema con TLB
void AddrSpace::RestoreState() {

  DEBUG('o', " ___ESTADO MEMORIA___\n");
  DEBUG('o', "[");
  for (int i = 0; i < PageSize; i++) {
    DEBUG('o', "%x", machine->mainMemory[PageSize + i]);
  }
  DEBUG('o', "]\n");

#ifndef USE_TLB
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
#else

  DEBUG('1', "\t||| RESTORE TLB -> NULL {%s}\n", currentThread->getName());
  machine->tlb = new TranslationEntry[TLBSize];
  machine->nextTLB = 0;

  for(int i = 0; i < this->numPages; i++){
    this->pageTable[i].valid = false;
    this->pageTable[i].physicalPage = -1;
    DEBUG('1', "\t||| RESTORING VPN %d\n", i);
  }

  // DEBUG('o', " ____ESTADO SWAP____\n");
  // DEBUG('o', "[");
  // for (int i = 0; i < PageSize; i++) {
  //   DEBUG('o', "%x", swapSpace[PageSize + i]);
  // }
  // DEBUG('o', "]\n");

#endif
}

TranslationEntry *AddrSpace::EntryFromVirtPage(unsigned virtpage) {
  if (virtpage < numPages) {
    return &this->pageTable[virtpage];
  }
  return NULL;
}
TranslationEntry *AddrSpace::EntryFromPhysPage(unsigned physpage) {
  for (int vp = 0; vp < numPages; vp++) {
    TranslationEntry entry = this->pageTable[vp];
    if (entry.physicalPage == physpage && entry.valid) {
      return &this->pageTable[vp];
    }
  }
  return NULL;
}

void AddrSpace::printPT() {
  for (int vp = 0; vp < numPages; vp++) {
    TranslationEntry e = this->pageTable[vp];
    printf("[%d] VALID: %d DIRTY: %d |VP %d|PP %d|SE %d|\n", vp, e.valid,
           e.dirty, e.virtualPage, e.physicalPage, e.swapSector);
  }
}
