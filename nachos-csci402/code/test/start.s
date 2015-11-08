/* Start.s
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we dont want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#define IN_ASM
#include "syscall.h"

        .text
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main".
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0
	jal	Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl Halt
	.ent	Halt
Halt:
	addiu $2,$0,SC_Halt
	syscall
	j	$31
	.end Halt

	.globl Exit
	.ent	Exit
Exit:
	addiu $2,$0,SC_Exit
	syscall
	j	$31
	.end Exit

	.globl Exec
	.ent	Exec
Exec:
	addiu $2,$0,SC_Exec
	syscall
	j	$31
	.end Exec

	.globl Join
	.ent	Join
Join:
	addiu $2,$0,SC_Join
	syscall
	j	$31
	.end Join

	.globl Create
	.ent	Create
Create:
	addiu $2,$0,SC_Create
	syscall
	j	$31
	.end Create

	.globl Open
	.ent	Open
Open:
	addiu $2,$0,SC_Open
	syscall
	j	$31
	.end Open

	.globl Read
	.ent	Read
Read:
	addiu $2,$0,SC_Read
	syscall
	j	$31
	.end Read

	.globl Write
	.ent	Write
Write:
	addiu $2,$0,SC_Write
	syscall
	j	$31
	.end Write

	.globl Close
	.ent	Close
Close:
	addiu $2,$0,SC_Close
	syscall
	j	$31
	.end Close

	.globl Fork
	.ent	Fork
Fork:
	addiu $2,$0,SC_Fork
	syscall
	j	$31
	.end Fork

	.globl Yield
	.ent	Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

	.globl CreateLock
	.ent	CreateLock
CreateLock:
	addiu $2,$0,SC_CreateLock
	syscall
	j	$31
	.end CreateLock

	.globl Acquire
	.ent	Acquire
Acquire:
	addiu $2,$0,SC_Acquire
	syscall
	j	$31
	.end Acquire

	.globl Release
	.ent	Release
Release:
	addiu $2,$0,SC_Release
	syscall
	j	$31
	.end Release

	.globl DestroyLock
	.ent	DestroyLock
DestroyLock:
	addiu $2,$0,SC_DestroyLock
	syscall
	j	$31
	.end DestroyLock

	.globl CreateCondition
	.ent	CreateCondition
CreateCondition:
	addiu $2,$0,SC_CreateCondition
	syscall
	j	$31
	.end CreateCondition

	.globl Wait
	.ent	Wait
Wait:
	addiu $2,$0,SC_Wait
	syscall
	j	$31
	.end Wait

	.globl Signal
	.ent	Signal
Signal:
	addiu $2,$0,SC_Signal
	syscall
	j	$31
	.end Signal

	.globl Broadcast
	.ent	Broadcast
Broadcast:
	addiu $2,$0,SC_Broadcast
	syscall
	j	$31
	.end Broadcast

	.globl DestroyCondition
	.ent	DestroyCondition
DestroyCondition:
	addiu $2,$0,SC_DestroyCondition
	syscall
	j	$31
	.end DestroyCondition

	.globl Rand
	.ent	Rand
Rand:
	addiu $2,$0,SC_Rand
	syscall
	j	$31
	.end Rand

	.globl GetThreadArgs
	.ent	GetThreadArgs
GetThreadArgs:
	addiu $2,$0,SC_GetThreadArgs
	syscall
	j	$31
	.end GetThreadArgs

	.globl PrintString
	.ent	PrintString
PrintString:
	addiu $2,$0,SC_PrintString
	syscall
	j	$31
	.end PrintString

	.globl PrintNum
	.ent	PrintNum
PrintNum:
	addiu $2,$0,SC_PrintNum
	syscall
	j	$31
	.end PrintNum

	.globl PrintNl
	.ent	PrintNl
PrintNl:
	addiu $2,$0,SC_PrintNl
	syscall
	j	$31
	.end PrintNl

	.globl CreateMonitor
	.ent	CreateMonitor
CreateMonitor:
	addiu $2,$0,SC_CreateMonitor
	syscall
	j	$31
	.end CreateMonitor

	.globl GetMonitor
	.ent	GetMonitor
GetMonitor:
	addiu $2,$0,SC_GetMonitor
	syscall
	j	$31
	.end GetMonitor

	.globl SetMonitor
	.ent	SetMonitor
SetMonitor:
	addiu $2,$0,SC_SetMonitor
	syscall
	j	$31
	.end SetMonitor

	.globl DestroyMonitor
	.ent	DestroyMonitor
DestroyMonitor:
	addiu $2,$0,SC_DestroyMonitor
	syscall
	j	$31
	.end DestroyMonitor

/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main
