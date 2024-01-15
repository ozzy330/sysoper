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
extern const int PRUEBA = 68;
extern int SALIR = 0;

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

bool Machine::ReadMem(int addr, int size, int *value) {
  int data;
  ExceptionType exception;
  int physicalAddress;

  DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);

  exception = Translate(addr, &physicalAddress, size, false);
  if (exception != NoException) {
    machine->RaiseException(exception, addr);
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

bool Machine::WriteMem(int addr, int size, int value) {
  ExceptionType exception;
  int physicalAddress;

  DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

  exception = Translate(addr, &physicalAddress, size, true);

  if (exception != NoException) {
    machine->RaiseException(exception, addr);
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
        DEBUG('y', "\t-- VPN [%d] ON TLB [%d]\n", vpn, i);
        TLBRef->SecureMark(i);
        break;
      }
    }
    if (entry == NULL) { // not found
      DEBUG('y', "\t-- [%d] NOT on TLB\n", vpn);
      // NOTE: VM busca primero si está en memoria
      entry = currentThread->space->EntryFromVirtPage(vpn);
      if (entry == NULL) {
        DEBUG('y', "\t-- [%d] NOT on Machine\n", vpn);
        return BusErrorException;
      }
      if (entry->valid) {
        DEBUG('y', "\t-- [%d] ON Memory in [%d] \n", vpn, entry->physicalPage);
        DEBUG('y', "\t-- Buscando remplazo en TLB\n");
        // Busca una pagina libre o una a remplazar
        int tlb_page = secondChance(TLBRef, nextTLB, TLBSize);
        DEBUG('y', "\t-- TLB [VPN %d] -> [VPN %d]\n", tlb[tlb_page].virtualPage,
              entry->virtualPage);
        nextTLB = tlb_page;
        tlb[tlb_page] = *entry;
        // Marca como referenciada
        MemRef->SecureMark(entry->physicalPage);
      } else {
        // NOTE: VM si no está en memoria la toma del SWAP
        DEBUG('y', "\t-- [%d] NOT on Memory\n", vpn);
        // Verifica si hay espacio en memoria si no, remplaza
        int mem_page = MapitaBits->SecureFind();
        if (mem_page == -1) {
          DEBUG('y', "\t-- Buscando remplazo en Memoria\n");
          mem_page = secondChance(MemRef, nextMem, NumPhysPages);
          nextMem = mem_page;
          MapitaBits->SecureMark(mem_page);
        }
        // Invalida pagina remplazada en Page Table y TLB
        TranslationEntry *e = currentThread->space->EntryFromPhysPage(mem_page);
        if (e != NULL) {
          e->valid = false;
          e->physicalPage = -1;

          // La invalida en TLB si está
          for (int pag_tlb = 0; pag_tlb < TLBSize; pag_tlb++) {
            if (tlb[pag_tlb].virtualPage == entry->virtualPage) {
              tlb[pag_tlb].valid = false;
              tlb[pag_tlb].physicalPage = -1;
              break;
            }
          }
          // Guarda los cambios si hay
          if (e->dirty == true) {
            e->dirty = false;
            DEBUG('c',
                  "\t-----------------VPN %d NOT DIRTY -------------------\n",
                  e->virtualPage);
            DEBUG('1', "\t-- Guardando VPN [%d] | PPN [%d] -> SWAP [%d]\n",
                  e->virtualPage, mem_page, e->swapSector);

            DEBUG('c', "DISCARTED PAGE (%d) [", mem_page);
            for (i = 0; i < PageSize; i++) {
              DEBUG('c', "%x", this->mainMemory[mem_page * PageSize + i]);
            }
            DEBUG('c', "]\n");
            DEBUG('c', "DISCARTED SWAP (%d)[", e->swapSector);
            for (i = 0; i < PageSize; i++) {
              DEBUG('c', "%x", swapSpace[e->swapSector * PageSize + i]);
              swapSpace[e->swapSector * PageSize + i] =
                  this->mainMemory[mem_page * PageSize + i];
            }
            DEBUG('c', "]\n");
            DEBUG('c', "**** NOW SWAP (%d)[", e->swapSector);
            for (i = 0; i < PageSize; i++) {
              DEBUG('c', "%x", swapSpace[e->swapSector * PageSize + i]);
            }
            DEBUG('c', "]\n");
          }
        }
        entry->physicalPage = mem_page;
        entry->valid = true;
        int page = entry->physicalPage * PageSize;
        int sector = entry->swapSector * SectorSize;
        // moviendo a memoria
        for (int byte = 0; byte < SectorSize; byte++) {
          machine->mainMemory[page + byte] = swapSpace[sector + byte];
        }

        DEBUG('y', "\t*** MEM[%d] <- VPN[%d]\n", entry->physicalPage,
              entry->virtualPage);
        DEBUG('y', "\t-- [%d] from Swap\n", vpn);
      }
    }
    if (entry == NULL) {
      stats->numPageFaults += 1;
      // really, this is a TLB fault, the page may be
      // in memory, but not in the TLB
      return PageFaultException;
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
  if (writing) {
    entry->dirty = true;
    DEBUG('c', "\t-----------------VPN %d DIRTY-------------------\n", vpn);
  }
  *physAddr = pageFrame * PageSize + offset;
  ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
  DEBUG('a', "phys addr = 0x%x\n", *physAddr);
  // DEBUG('a', "[%d:R/W] VPAG: %d {%d}, PAG: %d SEC: %d\n", SALIR,
  //       entry->virtualPage, offset, entry->physicalPage, entry->swapSector);

  // WARN:
  int data = *(unsigned int *)&machine->mainMemory[*physAddr];
  int value = WordToHost(data);
  DEBUG('c', "  [%d:%c] PAG: %d [VPN %d] VADDR: 0x%x OFFSET %d {%8.8x} (%s)\n",
        SALIR, ((writing == true) ? 'W' : 'R'), pageFrame, vpn, virtAddr,
        offset, value, currentThread->getName());

  TranslationEntry *e = currentThread->space->EntryFromVirtPage(13);

  DEBUG('c', "Memory %d[", entry->physicalPage);
  for (i = 0; i < PageSize; i++) {
    DEBUG('c', "%x", this->mainMemory[pageFrame * PageSize + i]);
  }
  DEBUG('c', "]\n");
#ifdef VM
  DEBUG('c', "SWAP %d[", entry->swapSector);
  for (i = 0; i < PageSize; i++) {
    DEBUG('c', "%x", swapSpace[entry->swapSector * PageSize + i]);
  }
  DEBUG('c', "]\n");

#endif

  ////--------------------------
  if (e->valid == true) {
    DEBUG('c', "Memory VPN 13 [");
    for (i = 0; i < PageSize; i++) {
      DEBUG('c', "%x", this->mainMemory[e->physicalPage * PageSize + i]);
    }
    DEBUG('c', "]\n");
  }
#ifdef VM
  DEBUG('c', "SWAP VPN 13 [");
  for (i = 0; i < PageSize; i++) {
    DEBUG('c', "%x", swapSpace[e->swapSector * PageSize + i]);
  }
  DEBUG('c', "]\n");

#endif

  SALIR++;
  if (SALIR == PRUEBA) {
    DEBUG('y', "------- TERMINADO POR DEBUG ------- \n");
    return BusErrorException;
  }
  return NoException;
}
