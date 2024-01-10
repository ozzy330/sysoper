// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "syscall.h"
#include "system.h"
#include <cstring>

void returnFromSystemCall() {

  machine->WriteRegister(PrevPCReg,
                         machine->ReadRegister(PCReg)); // PrevPC <- PC
  machine->WriteRegister(PCReg,
                         machine->ReadRegister(NextPCReg)); // PC <- NextPC
  machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) +
                                        4); // NextPC <- NextPC + 4
} // returnFromSystemCall

/*
 *  System call interface: Halt()
 */
void NachOS_Halt() { // System call 0

  printf("Shutdown, initiated by user program.\n");
  // DEBUG('a', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() { // System call 1
  int status = machine->ReadRegister(4);
  runningThreads->Clear(currentThread->id);
  delete currentThread->space;
  currentThread->Finish();
  if (status == 0) {
    printf("\nExiting successfully the user program.\n");
  } else {
    printf("\nExiting with error on the user program.\n");
  }
  returnFromSystemCall();
}

static void NachosExecThread(void *filename) {
  printf("AHHHHHHHHHHHHHHHH\n");
  const char *fn = (char *)filename;
  printf("Executing %s\n", fn);
  // Abrir el archivo name
  OpenFile *executable = fileSystem->Open(fn);
  if (executable == NULL) {
    DEBUG('a', "Can not open the file %s\n", filename);
    return;
  }
  // Crear un addrspace nuevo
  AddrSpace *addrspace = new AddrSpace(executable);

  // Remplaza el programa actual con el nuevo
  currentThread->space = addrspace;

  delete executable; // close file

  addrspace->InitRegisters();
  addrspace->RestoreState();

  // Genera un nuevo id para el proximo programa (recicla ids)
  currentThread->id = runningThreads->SecureFind();
  machine->WriteRegister(2, currentThread->id);
  // Corre el nuevo programa
  machine->Run();
  ASSERT(false)
}
/*
 *  System call interface: SpaceId Exec( char * )
 *  No hace un exec, hace un fork que pone a ejecutar un programa
 */
void NachOS_Exec() { // System call 2
  Thread *newT = new Thread("New thread for Exec");
  void *filename = (void *)machine->ReadRegister(4);

  newT->Fork(NachosExecThread, filename);
}

/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() { // System call 3
  SpaceId id = machine->ReadRegister(4);
  Thread *this_thread = currentThread;
  while (runningThreads->SecureTest(id)) {
    this_thread->Sleep();
  }
  // Cuando termina de ejecutar el hilo deseado
  scheduler->ReadyToRun(this_thread);
  returnFromSystemCall();
}

/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() { // System call 4
  fileSystem->Create((char *)machine->ReadRegister(4), 0);
  returnFromSystemCall();
}

/*
 *
 *  System call 5
 *  System call interface: OpenFileId Open( char * )
 */
void NachOS_Open() {
  // Read the name from the user memory, see 5 below
  char *name = (char *)machine->ReadRegister(4);
  // Use NachosOpenFilesTable class to create a relationship
  // between user file and unix file
  int unixhandle = OpenForReadWrite(name, true);
  OpenFileId fileId = nachosTablita->Open(unixhandle);
  // Devuelve el ID del archivo abierto
  machine->WriteRegister(2, fileId);
  returnFromSystemCall(); // Update the PC registers
}

// Lee 'size' bytes desde la memoria en la direccion 'address'
char *NachosReadMem(int size, int address) {
  char *buffer = new char[size];
  for (int i = 0; i < size; i++) {
    machine->ReadMem(address + i, 1, (int *)&buffer[i]);
  }
  return buffer;
}

/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() { // System call 6
  char *buffer = NULL;
  int size = machine->ReadRegister(5); // Read size to write

  buffer = NachosReadMem(size, machine->ReadRegister(4));
  OpenFileId descriptor = machine->ReadRegister(6); // Read file descriptor

  // INFO: los archivos 0, 1, 2 están reservados

  // Need a semaphore to synchronize access to console
  // Console->P();
  switch (descriptor) {
  case ConsoleInput: // User could not write to standard input
    machine->WriteRegister(2, -1);
    break;
  case ConsoleOutput:
    printf("%s", buffer);
    fflush(stdout);
    break;
  case ConsoleError: // This trick permits to write integers to console
    printf("%d\n", machine->ReadRegister(4));
    break;
  default: // All other opened files
    // Verify if the file is opened, if not return -1 in r2
    if (!nachosTablita->isOpened(descriptor)) {
      machine->WriteRegister(2, -1);
      break;
    }
    // Get the unix handle from our table for open files
    int unixhandle = nachosTablita->getUnixHandle(descriptor);
    // Do the read from the already opened Unix file
    WriteFile(unixhandle, buffer, size);
    // Return the number of chars read from user, via r2
    machine->WriteRegister(2, size);
    break;
  }
  // Update simulation stats, see details in Statistics class in
  // machine/stats.cc
  stats->numDiskWrites += size;
  // Console->V();

  returnFromSystemCall(); // Update the PC registers
}

/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() { // System call 7
  // Do the read from the already opened Unix file
  int dir_buffer = machine->ReadRegister(4);
  int size = machine->ReadRegister(5);
  int unixhandle = nachosTablita->getUnixHandle(machine->ReadRegister(6));

  char buffer[size] = {0};
  int readed = ReadPartial(unixhandle, buffer, size);
  for (int offset = 0; offset < size; offset++) {
    machine->WriteMem(dir_buffer + offset, 1, buffer[offset]);
  }
  machine->WriteRegister(2, readed);
  returnFromSystemCall();
}

/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() { // System call 8
  OpenFileId id = machine->ReadRegister(4);
  Close(nachosTablita->getUnixHandle(id));
  nachosTablita->Close(id);
  returnFromSystemCall(); // Update the PC registers
}

// Pass the user routine address as a parameter for this function
// This function is similar to "StartProcess" in "progtest.cc" file under
// "userprog" Requires a correct AddrSpace setup to work well
void NachosForkThread(void *p) { // for 64 bits version
  AddrSpace *space;

  space = currentThread->space;
  space->InitRegisters(); // set the initial register values
  space->RestoreState();  // load page table register

  // Set the return address for this thread to the same as the main thread
  // This will lead this thread to call the exit system call and finish
  machine->WriteRegister(RetAddrReg, 4);

  machine->WriteRegister(PCReg, (long)p);
  machine->WriteRegister(NextPCReg, (long)p + 4);

  machine->Run(); // jump to the user progam
  ASSERT(false);
}

/*
 *  System call interface: void Fork( void (*func)() )
 */
void NachOS_Fork() { // System call 9
  DEBUG('u', "Entering Fork System call\n");
  // We need to create a new kernel thread to execute the user thread
  Thread *newT = new Thread("child to execute Fork code");

  // We need to share the Open File Table structure with this new child

  // Child and father will also share the same address space, except for the
  // stack Text, init data and uninit data are shared, a new stack area must be
  // created for the new child We suggest the use of a new constructor in
  // AddrSpace class, This new constructor will copy the shared segments (space
  // variable) from currentThread, passed as a parameter, and create a new stack
  // for the new child
  newT->space = new AddrSpace(*currentThread->space);

  // We (kernel)-Fork to a new method to execute the child code
  // Pass the user routine address, now in register 4, as a parameter
  // Note: in 64 bits register 4 need to be casted to (void *)
  newT->Fork(NachosForkThread, (void *)machine->ReadRegister(4));
  returnFromSystemCall(); // This adjust the PrevPC, PC, and NextPC registers
  DEBUG('u', "Exiting Fork System call\n");
}

/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() { // System call 10
  currentThread->Yield();
  returnFromSystemCall();
}

/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() { // System call 11
}

/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() { // System call 12
}

/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() { // System call 13
}

/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() { // System call 14
}

/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() { // System call 15
}

/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() { // System call 16
}

/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() { // System call 17
}

/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() { // System call 18
}

/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() { // System call 19
}

/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() { // System call 20
}

/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() { // System call 21
}

/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() { // System call 22
}

/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() { // System call 23
}

/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() { // System call 30
}

/*
 *  System call interface: Socket_t Connect( char *, int )
 */
void NachOS_Connect() { // System call 31
}

/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() { // System call 32
}

/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() { // System call 33
}

/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() { // System call 34
}

/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() { // System call 25
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

// INFO: Manenja las excepciones en NachOS
void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2);

  switch (which) {

  case SyscallException:
    switch (type) {
    case SC_Halt: // System call # 0
      NachOS_Halt();
      break;
    case SC_Exit: // System call # 1
      NachOS_Exit();
      break;
    case SC_Exec: // System call # 2
      NachOS_Exec();
      break;
    case SC_Join: // System call # 3
      NachOS_Join();
      break;

    case SC_Create: // System call # 4
      NachOS_Create();
      break;
    case SC_Open: // System call # 5
      NachOS_Open();
      break;
    case SC_Read: // System call # 6
      NachOS_Read();
      break;
    case SC_Write: // System call # 7
      NachOS_Write();
      break;
    case SC_Close: // System call # 8
      NachOS_Close();
      break;

    case SC_Fork: // System call # 9
      NachOS_Fork();
      break;
    case SC_Yield: // System call # 10
      NachOS_Yield();
      break;

    case SC_SemCreate: // System call # 11
      NachOS_SemCreate();
      break;
    case SC_SemDestroy: // System call # 12
      NachOS_SemDestroy();
      break;
    case SC_SemSignal: // System call # 13
      NachOS_SemSignal();
      break;
    case SC_SemWait: // System call # 14
      NachOS_SemWait();
      break;

    case SC_LckCreate: // System call # 15
      NachOS_LockCreate();
      break;
    case SC_LckDestroy: // System call # 16
      NachOS_LockDestroy();
      break;
    case SC_LckAcquire: // System call # 17
      NachOS_LockAcquire();
      break;
    case SC_LckRelease: // System call # 18
      NachOS_LockRelease();
      break;

    case SC_CondCreate: // System call # 19
      NachOS_CondCreate();
      break;
    case SC_CondDestroy: // System call # 20
      NachOS_CondDestroy();
      break;
    case SC_CondSignal: // System call # 21
      NachOS_CondSignal();
      break;
    case SC_CondWait: // System call # 22
      NachOS_CondWait();
      break;
    case SC_CondBroadcast: // System call # 23
      NachOS_CondBroadcast();
      break;

    case SC_Socket: // System call # 30
      NachOS_Socket();
      break;
    case SC_Connect: // System call # 31
      NachOS_Connect();
      break;
    case SC_Bind: // System call # 32
      NachOS_Bind();
      break;
    case SC_Listen: // System call # 33
      NachOS_Listen();
      break;
    case SC_Accept: // System call # 32
      NachOS_Accept();
      break;
    case SC_Shutdown: // System call # 33
      NachOS_Shutdown();
      break;

    default:
      printf("Unexpected syscall exception %d\n", type);
      ASSERT(false);
      break;
    }
    break;

  case PageFaultException: {
    break;
  }

  case ReadOnlyException:
    printf("Read Only exception (%d)\n", which);
    ASSERT(false);
    break;

  case BusErrorException:
    printf("Bus error exception (%d)\n", which);
    ASSERT(false);
    break;

  case AddressErrorException:
    printf("Address error exception (%d)\n", which);
    ASSERT(false);
    break;

  case OverflowException:
    printf("Overflow exception (%d)\n", which);
    ASSERT(false);
    break;

  case IllegalInstrException:
    printf("Ilegal instruction exception (%d)\n", which);
    ASSERT(false);
    break;

  default:
    printf("Unexpected exception %d\n", which);
    ASSERT(false);
    break;
  }
}
