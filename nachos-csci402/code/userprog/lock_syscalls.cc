#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "synchlist.h"
#include "addrspace.h"
#include "network.h"
#include "post.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>

#define BUFFER_SIZE 32

int CreateLock_sys(int vaddr, int size, int appendNum) {
	currentThread->space->locksLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode
	if (currentThread->space->lockCount >= MAX_LOCK_COUNT){ //check if over max size of lock count
		DEBUG('l',currentThread->getName());
		DEBUG('l'," has too many locks!-----------------------\n");
		currentThread->space->locksLock->Release();
		return -1;
	}
	if (size < 0 || size >= 32){ // check if size of char array is valid
		DEBUG('l',currentThread->getName());
		DEBUG('l'," Size must be between 0 and 32!-----------------------\n");
		currentThread->space->locksLock->Release();
		return -1;
	}
	char* buffer = new char[size + 1]; //allocate new char array
	buffer[size] = '\0'; //end the char array with a null character

	if (copyin(vaddr, size, buffer) == -1){
		DEBUG('l',"%s"," COPYIN FAILED\n");
		delete[] buffer;
		currentThread->space->locksLock->Release();
		return -1;
	}; //copy contents of the virtual addr (ReadRegister(4)) to the buffer

	// set attributes of new lock
    PacketHeader pktHdr;
	MailHeader mailHdr;

    mailHdr.to = 0;
    mailHdr.from = 0;
    pktHdr.to = 0;

    stringstream ss;
	ss << "L C ";
	ss << buffer;
	string str = ss.str();
	char sendBuffer[64];
	for(int i = 0; i < str.size(); ++i) {
		cout << "[" << i << "]: " << str.at(i) << endl;
		sendBuffer[i] = str.at(i);
	}
	sendBuffer[str.size()] = '\0';
	cout << "Client::This is the init buffer: " << sendBuffer << endl;
    bool success = postOffice->Send(pktHdr, mailHdr, sendBuffer);

	if ( !success ) {
		printf("Client::The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
    char inBuffer[64];
    postOffice->Receive(0, &pktHdr, &mailHdr, inBuffer);

    cout << "inBuffer: " << inBuffer << endl;
    ss.str("");
    ss.clear();
    ss << inBuffer;
    cout << ss.str() << endl;
    int currentLockIndex = -1;
    ss >> currentLockIndex;

    cout << "currentLockIndex: " << currentLockIndex << endl;

    if(currentLockIndex == -1) {
        cout << "Client::currentLockIndex == -1" << endl;
        interrupt->Halt();
    }

    cout << currentLockIndex << endl;
	++(currentThread->space->lockCount); // increase lock count
	printf("conner1");
	//DEBUG('a', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	//DEBUG('l',"    Lock::Lock number: %d || name: %s created by %s\n", currentLockIndex, currentThread->space->userLocks[currentLockIndex].userLock->getName(), currentThread->getName());
	currentThread->space->locksLock->Release(); //release kernel lock
	printf("conner2");
	return currentLockIndex;
}

void Acquire_sys(int index) {
	currentThread->space->locksLock->Acquire();
	if (index < 0 || index >= currentThread->space->lockCount){ // check if index is in valid range
		DEBUG('l',"    Lock::Lock number %d invalid, thread %s can't acquire-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}

	if (currentThread->space->userLocks[index].isDeleted == TRUE){ // check if lock is deleted
		DEBUG('l',"    Lock::Lock number %d already destroyed, thread %s can't acquire-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		interrupt->Halt();
	}

	// check if lock is busy
	if(currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->BUSY){
		DEBUG('l',"    Lock::Lock number %d and name %s already in use, adding to queue-----------------------\n", index, currentThread->space->userLocks[index].userLock->getName());
		currentThread->space->locksLock->Release();
		currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
		return;
	}

	DEBUG('a', "Lock  number %d and name %s\n", index, currentThread->space->userLocks[index].userLock->getName());
	DEBUG('l',"    Lock::Lock number: %d || name:  %s acquired by %s\n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());

	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if(userLock->lockStatus != userLock->FREE) {
		updateProcessThreadCounts(currentThread->space, SLEEP);
	}
	currentThread->space->locksLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
}

void Release_sys(int index) {
	currentThread->space->locksLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if (index < 0 || index >= currentThread->space->lockCount){ //check if index is valid
		DEBUG('l',"Lock number %d invalid, thread %s can't release-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){ // check if lock is deleted
		DEBUG('l'," Lock number %d already destroyed, %s can't release-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if(userLock->lockStatus == userLock->FREE){ // check if lock is already free
		DEBUG('l'," lock not in use, nothing is done-----------------------\n");
		currentThread->space->locksLock->Release();
		return;
	}
	DEBUG('l',"    Lock::Lock number: %d || and name: %s released by %s\n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());

	currentThread->space->locksLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Release(); // release userlock at index
	// destroys lock if lock is free and delete flag is true
	if(currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->FREE && currentThread->space->userLocks[index].deleteFlag == TRUE) {
		DEBUG('l'," Lock  number %d  and name %s is destroyed by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
		currentThread->space->userLocks[index].isDeleted = TRUE;
		delete currentThread->space->userLocks[index].userLock;
	}
}

void DestroyLock_sys(int index) {
	currentThread->space->locksLock->Acquire();; // CL: acquire locksLock so that no other thread is running on kernel mode
	if (index < 0 || index >= currentThread->space->lockCount){ // check if lock index is valid
		DEBUG('l'," Lock number %d invalid, thread %s can't destroy-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){ // check if lock is already destroyed
		DEBUG('l'," Lock number %d already destroyed, thread %s can't destroy-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}

	currentThread->space->userLocks[index].deleteFlag = TRUE;
	if (currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->BUSY){
		DEBUG('l'," DestroyLock::Lock number %d and name %s still in use, delete later-----------------------\n", index, currentThread->space->userLocks[index].userLock->getName());
	}else{
		currentThread->space->userLocks[index].isDeleted = TRUE;
		delete currentThread->space->userLocks[index].userLock;
		DEBUG('l'," Lock number %d and name %s deleted-----------------------\n", index, currentThread->space->userLocks[index].userLock->getName());

	}
	currentThread->space->locksLock->Release();//release kernel lock
}
