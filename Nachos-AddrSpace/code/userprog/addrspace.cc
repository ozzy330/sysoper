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
#include "copyright.h"
#include "noff.h"
#include "system.h"

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

void ReadPages(OpenFile *executable, int firstPage, int size, int startFileAddr,
               TranslationEntry *pageTable, int numPages) {
  int bytesLeft = size;

  for (int i = firstPage; i < firstPage + numPages; i++) {
    if (bytesLeft < PageSize) {
      // leer solo los bytes faltantes
      executable->ReadAt(
          &(machine->mainMemory[pageTable[i].physicalPage * PageSize]),
          bytesLeft, startFileAddr + (i * PageSize));
    }
    // leer de disco y escribir en marco de pagina libre
    executable->ReadAt(
        &(machine->mainMemory[pageTable[i].physicalPage * PageSize]), PageSize,
        startFileAddr + (i * PageSize));
    bytesLeft -= PageSize;
  }
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
  size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
         UserStackSize;
  numPages = divRoundUp(size, PageSize);
  size = numPages * PageSize;

  // check we're not trying to run anything too big -- at least until we have
  // virtual memory
  ASSERT(numPages <= NumPhysPages);
  // NOTE: tamaño de página de 128 bytes (Sectorsize)
  // INFO: nueva forma de generar las páginas logicas
  int numCodePages = divRoundUp(noffH.code.size, PageSize);
  int numDataPages = divRoundUp(noffH.initData.size, PageSize);
  int numUninitDataPages = divRoundUp(noffH.uninitData.size, PageSize);
  int numStackPages = divRoundUp(UserStackSize, PageSize);
  numPages = numCodePages + numDataPages + numUninitDataPages + numStackPages;
  size = numPages * PageSize;

  DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
        size);

  // INFO: asigna páginas logicas a fisicas 1 a 1, por ahora para cada proceso

  // first, set up the translation
  pageTable = new TranslationEntry[numPages];

  // for (int pagina = 0; pagina <= 10; pagina+=2) {
  //   MapitaBits->Mark(pagina);
  // }

  int free_page;
  for (i = 0; i < numPages; i++) {
    // for now, virtual page # = phys page #
    free_page = MapitaBits->Find();
    pageTable[i].virtualPage = i;
    // INFO:  marca las páginas físicas utilizadas (marcos de página)
    pageTable[i].physicalPage = free_page;
    pageTable[i].valid = true;
    pageTable[i].use = false;
    pageTable[i].dirty = false;
    // if the code segment was entirely on
    // a separate page, we could set its
    // pages to be read-only
    pageTable[i].readOnly = false;
  }

  // zero out the entire address space, to zero the unitialized data segment
  // and the stack segment
  // bzero(machine->mainMemory, size);

  // then, copy in the code and data segments into memory
  if (noffH.code.size > 0) {
    // printf("Inicializando segmento de código\n");
    // executable->ReadAt(&(machine->mainMemory[pageTable[0].physicalPage]),
    //                    noffH.code.size, noffH.code.inFileAddr);
    ReadPages(executable, 0, noffH.code.size, noffH.code.inFileAddr,
              pageTable, numCodePages);
    // printf("Cargado segmento de código\n");
  }
  if (noffH.initData.size > 0) {
    // printf("Inicializando segmento de datos\n");
    // executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
    //                    noffH.initData.size, noffH.initData.inFileAddr);
    // La siguiente página despues del código es la de los datos
    ReadPages(executable, numCodePages, noffH.initData.size, noffH.initData.inFileAddr,
              pageTable, numDataPages);
    // printf("Cargado segmento de datos\n");
  }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() { delete pageTable; }

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

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
}
