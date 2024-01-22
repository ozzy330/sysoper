// translate.cc
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception.
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "translate.h"
#include "addrspace.h"
#include "copyright.h"
#include "disk.h"
#include "machine.h"
#include "system.h"
#include "utility.h"

// extern const int PRUEBA = 503;
extern const int PRUEBA = -1;
extern int SALIR = 0; 
extern int DEBUG_NUM = - 1; 
extern int DEBUG_LASTVPN = -1;
extern int DEBUG_LASTOFFSET = -1;

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
  register unsigned long result;
  result = (word >> 24) & 0x000000ff;
  result |= (word >> 8) & 0x0000ff00;
  result |= (word << 8) & 0x00ff0000;
  result |= (word << 24) & 0xff000000;
  return result;
#else
  return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
  register unsigned short result;
  result = (shortword << 8) & 0xff00;
  result |= (shortword >> 8) & 0x00ff;
  return result;
#else
  return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short ShortToMachine(unsigned short shortword) {
  return ShortToHost(shortword);
}

//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into
//	the location pointed to by "value".
//
//   	Returns false if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------

bool Machine::ReadMem(int addr, int size, int *value, const char *debug) {
  int data;
  ExceptionType exception;
  int physicalAddress;

  DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);

  int vpn = (unsigned)addr / PageSize;
  int offset = (unsigned)addr % PageSize;
  if (DEBUG_NUM == -1 || DEBUG_NUM == SALIR) {
    DEBUG('0', "?? TRIYING [%d:R] VPN: %d OFFS: %d {%s}\n", SALIR, vpn, offset,
          debug);
  }

  exception = Translate(addr, &physicalAddress, size, false);
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    return false;
  } else {
    SALIR++;
  }

  if (DEBUG_NUM == -1 || DEBUG_NUM == SALIR) {
    DEBUG('c', ">> DONE [%d:R] VPN: %d OFFS: %d {%s} === %s\n", SALIR, vpn,
          offset, debug, currentThread->getName());
  }

  if (PRUEBA != -1 && SALIR == PRUEBA) {
    DEBUG('c', "------- TERMINADO POR DEBUG ------- \n");
    machine->RaiseException(BusErrorException, addr);
    return false;
  }

  switch (size) {
  case 1:
    data = machine->mainMemory[physicalAddress];
    *value = data;
    break;

  case 2:
    data = *(unsigned short *)&machine->mainMemory[physicalAddress];
    *value = ShortToHost(data);
    break;

  case 4:
    data = *(unsigned int *)&machine->mainMemory[physicalAddress];
    *value = WordToHost(data);
    break;

  default:
    ASSERT(false);
  }

  DEBUG('a', "\tvalue read = %8.8x\n", *value);
  return true;
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns false if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool Machine::WriteMem(int addr, int size, int value, const char *debug) {
  ExceptionType exception;
  int physicalAddress;

  DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

  int vpn = (unsigned)addr / PageSize;
  int offset = (unsigned)addr % PageSize;

  if (DEBUG_NUM == -1 || DEBUG_NUM == SALIR) {
    DEBUG('0', "?? TRIYING [%d:W] VPN: %d OFFS: %d {%s} === %s\n", SALIR, vpn,
          offset, debug, currentThread->getName());
  }

  exception = Translate(addr, &physicalAddress, size, true);

  if (exception != NoException) {
    machine->RaiseException(exception, addr);
    return false;
  } else {
    SALIR++;
  }

  if (DEBUG_NUM == -1 || DEBUG_NUM == SALIR) {
    DEBUG('c', ">> DONE [%d:W] VPN: %d OFFS: %d {%s} === %s\n", SALIR, vpn,
          offset, debug, currentThread->getName());
  }

  if (PRUEBA != -1 && SALIR == PRUEBA) {
    DEBUG('c', "------- TERMINADO POR DEBUG ------- \n");
    machine->RaiseException(BusErrorException, addr);
    return false;
  }

  switch (size) {
  case 1:
    machine->mainMemory[physicalAddress] = (unsigned char)(value & 0xff);
    break;

  case 2:
    *(unsigned short *)&machine->mainMemory[physicalAddress] =
        ShortToMachine((unsigned short)(value & 0xffff));
    break;

  case 4:
    *(unsigned int *)&machine->mainMemory[physicalAddress] =
        WordToMachine((unsigned int)value);
    break;

  default:
    ASSERT(false);
  }

  return true;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using
//	either a page table or a TLB.  Check for alignment and all sorts
//	of other errors, and if everything is ok, set the use/dirty bits in
//	the translation table entry, and store the translated physical
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if true, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

ExceptionType Machine::Translate(int virtAddr, int *physAddr, int size,
                                 bool writing) {
  int i;
  unsigned int vpn, offset;
  TranslationEntry *entry;
  unsigned int pageFrame;

  DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

  // check for alignment errors
  if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))) {
    DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
    return AddressErrorException;
  }

  // we must have either a TLB or a page table, but not both!
  ASSERT(tlb == NULL || pageTable == NULL);
  ASSERT(tlb != NULL || pageTable != NULL);

  // calculate the virtual page number, and offset within the page,
  // from the virtual address
  vpn = (unsigned)virtAddr / PageSize;
  offset = (unsigned)virtAddr % PageSize;

#ifdef VM
  DEBUG('1', "BEFORE TLB:\n");
  for (i = 0; i < TLBSize; i++) {
    DEBUG('1', "[%d] VP %d | PP %d | SWP %d | VAL %d | DTY %d\n", i,
          tlb[i].virtualPage, tlb[i].physicalPage, tlb[i].swapSector,
          tlb[i].valid, tlb[i].dirty);
  }

#endif

  if (tlb == NULL) { // => page table => vpn is index into table
    if (vpn >= pageTableSize) {
      DEBUG('a', "virtual page # %d too large for page table size %d!\n",
            virtAddr, pageTableSize);
      return AddressErrorException;
    } else if (!pageTable[vpn].valid) {
      DEBUG('a', "virtual page # %d not valid!\n", virtAddr);
      // MapitaBits->Print();
      return PageFaultException;
    }
    entry = currentThread->space->EntryFromVirtPage(vpn);
    DEBUG('l', "Encontrada la pagina virtual %d\n", vpn);

  } else {
#ifdef VM
    for (entry = NULL, i = 0; i < TLBSize; i++) {
      if (tlb[i].valid && (tlb[i].virtualPage == (int)vpn)) {
        entry = &tlb[i]; // FOUND!
        TLBRef->Mark(i);
        DEBUG('2', "\t-- ON TLB[%d] : [VPN %d]\n", i, entry->virtualPage);
        break;
      }
    }
    if (entry == NULL) {
      DEBUG('2', "\t-- NOT ON TLB [VPN %d]\n", vpn);
      TranslationEntry *page = currentThread->space->EntryFromVirtPage(vpn);
      if (page == NULL) {
        DEBUG('2', "\t-- NOT ON MAHCINE [VPN %d]!!!!\n", vpn);
        return BusErrorException;
      }
      if (page->valid) {
        DEBUG('3', "\t-- ON MEM[%d] : [VPN %d]\n", page->physicalPage,
              page->virtualPage);
        int tlb_frame = secondChance(TLBRef, nextTLB, TLBSize, "FOR TLB");
        nextTLB = tlb_frame;
        this->tlb[tlb_frame] = *page;
        DEBUG('7', "\t\t$$ TLB[%d] -> VPN [%d]\n", tlb_frame,
              page->virtualPage);
      } else {
        // Busca un marco de página libre
        int mem_frame = MapitaBits->Find();
        if (mem_frame == -1) {
          DEBUG('3', "\t-- NOT SPACE FOR [VPN %d] ON MEM\n", page->virtualPage);
          int tlb_f;
          mem_frame = secondChance(MemRef, nextMem, NumPhysPages, "FOR MEM");
          nextMem = mem_frame;
          TranslationEntry *e =
              currentThread->space->EntryFromPhysPage(mem_frame);
          if (e != NULL) {
            for (tlb_f = 0; tlb_f < TLBSize; tlb_f++) {
              if (this->tlb[tlb_f].physicalPage == mem_frame &&
                  this->tlb[tlb_f].valid) {
                break;
              }
            }
            DEBUG('4', "\t>> Checking if [VPN %d] DIRTY: %s\n", e->virtualPage,
                  (e->dirty) ? "TRUE" : "FALSE");

            if (e->dirty == true) {
              int mem_offset = e->physicalPage * PageSize;
              int swap_offset = e->swapSector * SectorSize;

              DEBUG('4', "\t-- MEM[%d] : [VPN %d] -> SWAP [%d]\n",
                    e->physicalPage, e->virtualPage, e->swapSector);
              DEBUG('4', "MEM:");
              for (int byte = 0; byte < PageSize; byte++) {
                DEBUG('4', "%x", this->mainMemory[mem_offset + byte]);
              }
              DEBUG('4', " >> SWAP:");
              for (int byte = 0; byte < PageSize; byte++) {
                DEBUG('4', "%x", swapSpace[swap_offset + byte]);
              }
              DEBUG('4', "\n");

              for (int byte = 0; byte < PageSize; byte++) {
                swapSpace[swap_offset + byte] =
                    this->mainMemory[mem_offset + byte];
              }

              e->dirty = false;
              this->tlb[tlb_f].dirty = false;
            }
            DEBUG('5', "\t-- TLB[%d] : VPN[%d] -> INVALID\n", tlb_f,
                  e->virtualPage);
            this->tlb[tlb_f].valid = false;
            this->tlb[tlb_f].physicalPage = -1;
            DEBUG('5', "\t-- MEM[%d] : VPN[%d] -> INVALID\n", mem_frame,
                  e->virtualPage);
            e->valid = false;
            e->physicalPage = -1;
          }
        }
        page->physicalPage = mem_frame;
        page->valid = true;
        DEBUG('3', "\t-- NOT ON MEM [VPN %d]\n", vpn);
        int mem_offset = page->physicalPage * PageSize;
        int swap_offset = page->swapSector * SectorSize;
        DEBUG('4', "\t-- SWAP [%d] : [VPN %d] -> MEM[%d]\n", page->swapSector,
              page->virtualPage, page->physicalPage);

        DEBUG('4', "SWAP:");
        for (int byte = 0; byte < PageSize; byte++) {
          DEBUG('4', "%x", swapSpace[swap_offset + byte]);
        }
        DEBUG('4', " >> MEM:");
        for (int byte = 0; byte < PageSize; byte++) {
          DEBUG('4', "%x", this->mainMemory[mem_offset + byte]);
        }
        DEBUG('4', "\n");

        for (int byte = 0; byte < PageSize; byte++) {
          this->mainMemory[mem_offset + byte] = swapSpace[swap_offset + byte];
        }
      }
    }
    if (entry == NULL) {

      // NOTE: VM vuelve a ejecutar la misma instruccion
      DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
      stats->numPageFaults++;
      return PageFaultException;
      // really, this is a TLB fault, the page may be
      // in memory, but not in the TLB
    }
#endif
  }

  if (entry->readOnly && writing) { // trying to write to a read-only page
    DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
    return ReadOnlyException;
  }
  pageFrame = entry->physicalPage;

  // if the pageFrame is too big, there is something really wrong!
  // An invalid translation was loaded into the page table or TLB.
  if (pageFrame >= NumPhysPages) {
    DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
    return BusErrorException;
  }
  entry->use = true; // set the use, dirty bits
  currentThread->space->EntryFromVirtPage(vpn)->use = true;
  if (writing) {
    entry->dirty = true;
    currentThread->space->EntryFromVirtPage(vpn)->dirty = true;
  }

  *physAddr = pageFrame * PageSize + offset;
  ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
  DEBUG('a', "phys addr = 0x%x\n", *physAddr);
  // DEBUG('a', "[%d:R/W] VPAG: %d {%d}, PAG: %d SEC: %d\n", SALIR,
  //       entry->virtualPage, offset, entry->physicalPage,
  //       entry->swapSector);

  char stat_mem[PageSize] = {0};
  char stat_swap[PageSize] = {1};
  bool equals = true;
  if (DEBUG_NUM == -1 || DEBUG_NUM == SALIR) {
    DEBUG('1', "MEM  (%d) [", entry->physicalPage);
    for (i = 0; i < PageSize; i++) {
      DEBUG('1', "%x", this->mainMemory[pageFrame * PageSize + i]);
      stat_mem[i] = this->mainMemory[pageFrame * PageSize + i];
    }
    DEBUG('1', "]\n");
#ifdef VM
    DEBUG('1', "SWAP (%d) [", entry->swapSector);
    for (i = 0; i < PageSize; i++) {
      DEBUG('1', "%x", swapSpace[entry->swapSector * PageSize + i]);
      stat_swap[i] = swapSpace[entry->swapSector * PageSize + i];
    }
    DEBUG('1', "]\n");

    for (i = 0; i < PageSize; i++) {
      if (stat_mem[i] != stat_swap[i]) {
        equals = false;
        break;
      }
    }
    if (equals) {
      DEBUG('1', "SWAP (%d) == MEM (%d)\n", entry->swapSector,
            entry->physicalPage);
    } else {
      DEBUG('1', "*********SWAP (%d) != MEM (%d)*********\n", entry->swapSector,
            entry->physicalPage);
    }

    DEBUG('1', "TLB:\n");
    for (i = 0; i < TLBSize; i++) {
      DEBUG('1', "[%d] VP %d | PP %d | SWP %d | VAL %d | DTY %d\n", i,
            tlb[i].virtualPage, tlb[i].physicalPage, tlb[i].swapSector,
            tlb[i].valid, tlb[i].dirty);
    }

#endif

    DEBUG('1', "PT:\n");
    for (i = 0; i < currentThread->space->numPages; i++) {
      TranslationEntry pt = *currentThread->space->EntryFromVirtPage(i);
      DEBUG('1', "[%d] VP %d | PP %d | SWP %d | VAL %d | DTY %d : ", i,
            pt.virtualPage, pt.physicalPage, pt.swapSector, pt.valid, pt.dirty);
      if (pt.valid == true) {
        for (int j = 0; j < PageSize; j++) {
          DEBUG('1', "%x", this->mainMemory[pt.physicalPage * PageSize + j]);
        }
      }
      DEBUG('1', "\n");
    }
  }

  return NoException;
}
