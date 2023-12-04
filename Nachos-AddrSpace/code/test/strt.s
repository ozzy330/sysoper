# 0 "start.s"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "start.s"
# 10 "start.s"
# 1 "../userprog/syscall.h" 1
# 16 "../userprog/syscall.h"
# 1 "../threads/copyright.h" 1
# 17 "../userprog/syscall.h" 2
# 11 "start.s" 2

        .text
        .align 2
# 24 "start.s"
 .globl __start
 .ent __start
__start:
 jal main
 move $4,$0
 jal Exit
 .end __start
# 45 "start.s"
 .globl Halt
 .ent Halt
Halt:
 addiu $2,$0,0
 syscall
 j $31
 .end Halt

 .globl Exit
 .ent Exit
Exit:
 addiu $2,$0,1
 syscall
 j $31
 .end Exit

 .globl Exec
 .ent Exec
Exec:
 addiu $2,$0,2
 syscall
 j $31
 .end Exec

 .globl Join
 .ent Join
Join:
 addiu $2,$0,3
 syscall
 j $31
 .end Join

 .globl Create
 .ent Create
Create:
 addiu $2,$0,4
 syscall
 j $31
 .end Create

 .globl Open
 .ent Open
Open:
 addiu $2,$0,5
 syscall
 j $31
 .end Open

 .globl Read
 .ent Read
Read:
 addiu $2,$0,6
 syscall
 j $31
 .end Read

 .globl Write
 .ent Write
Write:
 addiu $2,$0,7
 syscall
 j $31
 .end Write

 .globl Close
 .ent Close
Close:
 addiu $2,$0,8
 syscall
 j $31
 .end Close

 .globl Fork
 .ent Fork
Fork:
 addiu $2,$0,9
 syscall
 j $31
 .end Fork

 .globl Yield
 .ent Yield
Yield:
 addiu $2,$0,10
 syscall
 j $31
 .end Yield

 .globl SemCreate
 .ent SemCreate
SemCreate:
 addiu $2,$0,11
 syscall
 j $31
 .end SemCreate

 .globl SemDestroy
 .ent SemDestroy
SemDestroy:
 addiu $2,$0,12
 syscall
 j $31
 .end SemDestroy

 .globl SemSignal
 .ent SemSignal
SemSignal:
 addiu $2,$0,13
 syscall
 j $31
 .end SemSignal

 .globl SemWait
 .ent SemWait
SemWait:
 addiu $2,$0,14
 syscall
 j $31
 .end SemWait

 .globl LckCreate
 .ent LckCreate
LckCreate:
 addiu $2,$0,15
 syscall
 j $31
 .end LckCreate

 .globl LckDestroy
 .ent LckDestroy
LckDestroy:
 addiu $2,$0,16
 syscall
 j $31
 .end LckDestroy

 .globl LckAcquire
 .ent LckAcquire
LckAcquire:
 addiu $2,$0,17
 syscall
 j $31
 .end LckAcquire

 .globl LckRelease
 .ent LckRelease
LckRelease:
 addiu $2,$0,18
 syscall
 j $31
 .end LckRelease

 .globl CondCreate
 .ent CondCreate
CondCreate:
 addiu $2,$0,19
 syscall
 j $31
 .end CondCreate

 .globl CondDestroy
 .ent CondDestroy
CondDestroy:
 addiu $2,$0,20
 syscall
 j $31
 .end CondDestroy

 .globl CondAcquire
 .ent CondAcquire
CondSignal:
 addiu $2,$0,21
 syscall
 j $31
 .end CondAcquire

 .globl CondSignal
 .ent CondSignal
CondWait:
 addiu $2,$0,22
 syscall
 j $31
 .end CondWait

 .globl CondBroadcast
 .ent CondBroadcast
CondBroadCast:
 addiu $2,$0,23
 syscall
 j $31
 .end CondBroadcast

 .globl Socket
 .ent Socket
Socket:
 addiu $2,$0,30
 syscall
 j $31
 .end Socket

 .globl Connect
 .ent Connect
Connect:
 addiu $2,$0,31
 syscall
 j $31
 .end Connect

 .globl Bind
 .ent Bind
Bind:
 addiu $2,$0,32
 syscall
 j $31
 .end Bind

 .globl Listen
 .ent Listen
Listen:
 addiu $2,$0,33
 syscall
 j $31
 .end Listen

 .globl Accept
 .ent Accept
Accept:
 addiu $2,$0,34
 syscall
 j $31
 .end Accept

 .globl Shutdown
 .ent Shutdown
Shutdown:
 addiu $2,$0,35
 syscall
 j $31
 .end Shutdown


        .globl __main
        .ent __main
__main:
        j $31
        .end __main
