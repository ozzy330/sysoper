farroyo@fedora:~/nachos64/code/vm$ ./nachos -d a -x ../test/halt.lab 
Initializing stack register to 1392
Reading VA 0x0, size 4
  Translate 0x0, read: *** no valid TLB entry found for this virtual page!
Reading VA 0x0, size 4
  Translate 0x0, read: phys addr = 0x0
  value read = 0c000044
Reading VA 0x4, size 4
  Translate 0x4, read: phys addr = 0x4
  value read = 00000000
Reading VA 0x110, size 4
  Translate 0x110, read: *** no valid TLB entry found for this virtual page!
Reading VA 0x110, size 4
  Translate 0x110, read: phys addr = 0x90
  value read = 27bdffe8
Reading VA 0x114, size 4
  Translate 0x114, read: phys addr = 0x94
  value read = afbf0014
Writing VA 0x56c, size 4, value 0x8
  Translate 0x56c, write: *** no valid TLB entry found for this virtual page!
Reading VA 0x114, size 4
  Translate 0x114, read: phys addr = 0x94
  value read = afbf0014
Writing VA 0x56c, size 4, value 0x8
  Translate 0x56c, write: phys addr = 0x16c
Reading VA 0x118, size 4
  Translate 0x118, read: phys addr = 0x98
  value read = afbe0010
Writing VA 0x568, size 4, value 0x0
  Translate 0x568, write: phys addr = 0x168
Reading VA 0x11c, size 4
  Translate 0x11c, read: phys addr = 0x9c
  value read = 0c000040
Reading VA 0x120, size 4
  Translate 0x120, read: phys addr = 0xa0
  value read = 03a0f021
Reading VA 0x100, size 4
  Translate 0x100, read: phys addr = 0x80
  value read = 03e00008
Reading VA 0x104, size 4
  Translate 0x104, read: phys addr = 0x84
  value read = 00000000
Reading VA 0x124, size 4
  Translate 0x124, read: phys addr = 0xa4
  value read = 0c000004
Reading VA 0x128, size 4
  Translate 0x128, read: phys addr = 0xa8
  value read = 00000000
Reading VA 0x10, size 4
  Translate 0x10, read: phys addr = 0x10
  value read = 24020000
Reading VA 0x14, size 4
  Translate 0x14, read: phys addr = 0x14
  value read = 0000000c
Shutdown, initiated by user program.
Machine halting!

Ticks: total 85, idle 0, system 70, user 15
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 3
Network I/O: packets received 0, sent 0

Cleaning up...
