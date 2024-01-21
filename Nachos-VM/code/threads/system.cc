// system.cc
//	Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "system.h"
#include "copyright.h"
#include "preemptive.h"

// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;       // the thread we are running now
Thread *threadToBeDestroyed; // the thread that just finished
Scheduler *scheduler;        // the ready list
Interrupt *interrupt;        // interrupt status
Statistics *stats;           // performance metrics
Timer *timer;                // the hardware timer device,
                             // for invoking context switches

// 2007, Jose Miguel Santos Espino
PreemptiveScheduler *preemptiveScheduler = NULL;
const long long DEFAULT_TIME_SLICE = 50000;

#ifdef FILESYS_NEEDED
FileSystem *fileSystem;
#endif

#ifdef FILESYS
SynchDisk *synchDisk;
#endif

// requires either FILESYS or FILESYS_STUB
#ifdef USER_PROGRAM
// user program memory and registers
Machine *machine;

// Definicion del mapa de bits para la memoria
BitMap *MapitaBits;

// Definicion para tabla de archivso abiertos
NachosOpenFilesTable *nachosTablita;

// Definicion de control para hilos abiertos
BitMap *runningThreads;

// INFO: VM mantiene registro de paginas referenciadas de memoria
BitMap *MemRef;
#endif

#ifdef VM
// INFO: VM inicializacion de región de swap
Disk *swap;
Semaphore *swapDone;
BitMap *swapSectors;
// NOTE: cada vez que se termina de utilizar el swap se llama a esta function
// que libera la maquina para que pueda continuar trabajando
static void SwapAvailable(void *arg) { swapDone->V(); };
int SwapSize;
char *swapSpace;

// INFO: VM mantiene registro de paginas referenciadas en TLB
BitMap *TLBRef;

int secondChance(BitMap *ref, int next, int cant);

#endif

#ifdef NETWORK
PostOffice *postOffice;
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	Interrupt handler for the timer deviSIGTRAP example -perlce.  The timer
// device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each timeALRM there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as
//	if the interrupted thread called Yield at the point it is
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
static void TimerInterruptHandler(void *dummy) {
  if (interrupt->getStatus() != IdleMode)
    interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
// 	Initialize Nachos global data structures.  Interpret command
//	line arguments in ALRMorder to determine flags for the initialization.
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv) {

  int argCount;
  const char *debugArgs = "";
  bool randomYield = false;

  // 2007, Jose Miguel Santos Espino
  bool preemptiveScheduling = false;
  long long timeSlice;

#ifdef USER_PROGRAM
  // single step user program
  bool debugUserProg = false;
  // INFO: Inicializacion mapa de bits para el procesador
  MapitaBits = new BitMap(NumPhysPages);
  // INFO: Inicializacion de la tabla de archivos abiertos
  nachosTablita = new NachosOpenFilesTable();
  // INFO: Inicializacion de control para hilos abiertos
  runningThreads = new BitMap(MaxNumProcesses);
  MemRef = new BitMap(NumPhysPages);
#endif

#ifdef VM
  // INFO: VM inicializacion de region de swap
  swap = new Disk("SWAP space", SwapAvailable, 0);
  // INFO: VM inicializacion bitmap para saber que sectores están en uso
  // swapSectors = new BitMap(NumSectors);
  // INFO: VM Permite que se pueda continuar la ejecucion despues de leer o
  // escribir a disco (SWAP)
  swapDone = new Semaphore("SWAP available", 1);

  SwapSize = 64;
  swapSpace = new char[SwapSize * SectorSize];
  for (int i = 0; i < SwapSize * SectorSize; i++)
    swapSpace[i] = 0;
  swapSectors = new BitMap(SwapSize);

  TLBRef = new BitMap(TLBSize);
#endif

#ifdef FILESYS_NEEDED
  bool format = false; // format disk
#endif
#ifdef NETWORK
  double rely = 1; // network reliability
  int netname = 0; // UNIX socket name
#endif

  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;
    if (!strcmp(*argv, "-d")) {
      if (argc == 1)
        debugArgs = "+"; // turn on all debug flags
      else {
        debugArgs = *(argv + 1);
        argCount = 2;
      }
    } else if (!strcmp(*argv, "-rs")) {
      ASSERT(argc > 1);
      RandomInit(atoi(*(argv + 1))); // initialize pseudo-random
                                     // number generator
      randomYield = true;
      argCount = 2;
    }
    // 2007, Jose Miguel Santos Espino
    else if (!strcmp(*argv, "-p")) {
      preemptiveScheduling = true;
      if (argc == 1) {
        timeSlice = DEFAULT_TIME_SLICE;
      } else {
        timeSlice = atoi(*(argv + 1));
        argCount = 2;
      }
    }
#ifdef USER_PROGRAM
    if (!strcmp(*argv, "-s"))
      debugUserProg = true;
#endif
#ifdef FILESYS_NEEDED
    if (!strcmp(*argv, "-f"))
      format = true;
#endif
#ifdef NETWORK
    if (!strcmp(*argv, "-l")) {
      ASSERT(argc > 1);
      rely = atof(*(argv + 1));
      argCount = 2;
    } else if (!strcmp(*argv, "-m")) {
      ASSERT(argc > 1);
      netname = atoi(*(argv + 1));
      argCount = 2;
    }
#endif
  }

  DebugInit(debugArgs);        // initialize DEBUG messages
  stats = new Statistics();    // collect statistics
  interrupt = new Interrupt;   // start up interrupt handling
  scheduler = new Scheduler(); // initialize the ready queue
  if (randomYield)             // start the timer (if needed)
    timer = new Timer(TimerInterruptHandler, 0, randomYield);

  threadToBeDestroyed = NULL;

  // We didn't explicitly allocate the current thread we are running in.
  // But if it ever tries to give up the CPU, we better have a Thread
  // object to save its state.
  currentThread = new Thread("main");
  currentThread->setStatus(RUNNING);

  interrupt->Enable();
  CallOnUserAbort(Cleanup); // if user hits ctl-C

  // Jose Miguel Santos Espino, 2007
  if (preemptiveScheduling) {
    preemptiveScheduler = new PreemptiveScheduler();
    preemptiveScheduler->SetUp(timeSlice);
  }

#ifdef USER_PROGRAM
  machine = new Machine(debugUserProg); // this must come first
#endif

#ifdef FILESYS
  synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
  fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
  postOffice = new PostOffice(netname, rely, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void Cleanup() {

  printf("\nCleaning up...\n");

  // 2007, Jose Miguel Santos Espino
  delete preemptiveScheduler;

#ifdef NETWORK
  delete postOffice;
#endif

#ifdef USER_PROGRAM
  delete machine;
  delete nachosTablita;
  delete runningThreads;
  delete MapitaBits;
#endif

#ifdef VM
  delete swapDone;
  delete swap;
  delete swapSectors;
#endif

#ifdef FILESYS_NEEDED
  delete fileSystem;
#endif

#ifdef FILESYS
  delete synchDisk;
#endif

  delete timer;
  delete scheduler;
  delete interrupt;

  Exit(0);
}

#ifdef USER_PROGRAM
int secondChance(BitMap *ref, int next, int cant, const char *debug) {
  int to_replace = next;
  bool replaced = false;
  while (!replaced) {
    if (ref->Test(to_replace)) {
      DEBUG('3', "\t\t\t PAG [%d/%d] -> REF: FALSE  (%s)\n", to_replace,
            cant - 1, debug);
      ref->Clear(to_replace);
      to_replace = (to_replace == cant - 1) ? 0 : ++to_replace;
    } else {
      ref->Mark(to_replace);
      replaced = true;
      DEBUG('3', "\t\t\t PAG [%d/%d] -> REF: TRUE (%s)\n", to_replace, cant - 1,
            debug);
    }
  }
  DEBUG('3', "\t\t\t REMPLAZADA PAG [%d]\n", to_replace, cant - 1);
  return to_replace;
}
#endif
