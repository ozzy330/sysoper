// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "interrupt.h"
#include "scheduler.h"
#include "stats.h"
#include "thread.h"
#include "timer.h"
#include "utility.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); // Initialization,
                                               // called before anything else
extern void Cleanup();                         // Cleanup, called when
                                               // Nachos is done.

extern Thread *currentThread;       // the thread holding the CPU
extern Thread *threadToBeDestroyed; // the thread that just finished
extern Scheduler *scheduler;        // the ready list
extern Interrupt *interrupt;        // interrupt status
extern Statistics *stats;           // performance metrics
extern Timer *timer;                // the hardware alarm clock

#ifdef USER_PROGRAM
#include "bitmap.h"
#include "disk.h"
#include "machine.h"
#include "nachostablita.h"

// user program memory and registers
extern Machine *machine;

// Mapa de bits para la memoria del procesador
extern BitMap *MapitaBits;

// Tabla de archivos abiertos
extern NachosOpenFilesTable *nachosTablita;

// Mapa con los IDs de los hilos corriendo
extern BitMap *runningThreads;

// INFO: VM declaración de región de swap
extern Disk *swap;
extern BitMap *swapSectors;
extern Semaphore *swapDone;
extern int SwapSize;
extern char *swapSpace;
extern BitMap *MemRef;
extern BitMap *TLBRef;

// Implementación de algoritmo second chance para remplazo de paginas
//
// ref: bitmap con el bit de referencia de cada página
// size: cantidad de páginas
// next: ultima página sin referenciar
// debug: string para debuggear
extern int secondChance(BitMap *ref, int next, int cant, const char* debug);

#endif

#ifdef FILESYS_NEEDED // FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice *postOffice;
#endif

#endif // SYSTEM_H
