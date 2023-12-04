// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "synch.h"
#include "copyright.h"
#include "list.h"
#include "system.h"
#include "thread.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(const char *debugName, int initialValue) {
  name = (char *)debugName;
  value = initialValue;
  queue = new List<Thread *>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore() { delete queue; }

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Semaphore::P() {
  IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts

  while (value == 0) {            // semaphore not available
    queue->Append(currentThread); // so go to sleep
    currentThread->Sleep();
  }
  value--; // semaphore available,
           // consume its value

  interrupt->SetLevel(oldLevel); // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V() {
  Thread *thread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);

  thread = queue->Remove();
  if (thread != NULL) // make thread ready, consuming the V immediately
    scheduler->ReadyToRun(thread);
  value++;
  interrupt->SetLevel(oldLevel);
}

#ifdef USER_PROGRAM
//----------------------------------------------------------------------
// Semaphore::Destroy
// 	Destroy the semaphore, freeing the waiting threads
//	This is used to destroy a user semaphore
//----------------------------------------------------------------------

void Semaphore::Destroy() {
  Thread *thread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);

  while ((thread = queue->Remove()) != NULL) // make thread ready
    scheduler->ReadyToRun(thread);

  interrupt->SetLevel(oldLevel);
}

#endif

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!
Lock::Lock(const char *debugName) {
  this->name = (char *)debugName;
  // Inicializa el semaforo en 1 (libre)
  this->semaforo = new Semaphore("LOCK", 1);
  this->hilo_en_poder = NULL;
}

Lock::~Lock() { delete this->semaforo; }

void Lock::Acquire() {
  this->semaforo->P();
  this->hilo_en_poder = currentThread;
}

void Lock::Release() {
  if (isHeldByCurrentThread()) {
    this->hilo_en_poder = NULL;
    this->semaforo->V();
  }
}

bool Lock::isHeldByCurrentThread() {
  return currentThread == this->hilo_en_poder;
}

Condition::Condition(const char *debugName) {
  this->name = (char *)debugName;
  this->hilos_esperando = new List<Thread *>;
}

Condition::~Condition() { delete this->hilos_esperando; }

void Condition::Wait(Lock *conditionLock) {
  if (conditionLock->isHeldByCurrentThread()) {
    // Libera el Lock
    conditionLock->Release();
    // Se asegura de agregar y dormir solo el hilo actual
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    this->hilos_esperando->Append(currentThread);
    currentThread->Sleep();
    interrupt->SetLevel(oldLevel);
    // Obtiene devuelta el lock
    conditionLock->Acquire();
  }
}

void Condition::Signal(Lock *conditionLock) {
  Thread *thread;

  if (conditionLock->isHeldByCurrentThread()) {
    if (!hilos_esperando->IsEmpty()) {
      thread = this->hilos_esperando->Remove();
      if (thread != NULL) {
        scheduler->ReadyToRun(thread);
      }
    }
  }
}

void Condition::Broadcast(Lock *conditionLock) {
  while (!this->hilos_esperando->IsEmpty()) {
    this->Signal(conditionLock);
  }
}

// // Mutex class
// Mutex::Mutex(const char *debugName) {}
//
// Mutex::~Mutex() {}
//
// void Mutex::Lock() {}
//
// void Mutex::Unlock() {}
//
// // Barrier class
// Barrier::Barrier(const char *debugName, int count) {}
//
// Barrier::~Barrier() {}
//
// void Barrier::Wait() {}
