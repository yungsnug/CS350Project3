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

// in test::: setenv PATH ../gnu/:$PATH
// cd ../test/; setenv PATH ../gnu/:$PATH; cd ../userprog/
// in userprog::: nachos -x ../test/testfiles


#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int GLOBALCOUNT = 0;

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) { // FALL 09 CHANGES
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
      }

      buf[n++] = *paddr;

      if ( !result ) {
        //translation failed
        return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
        //translation failed
        return -1;
      }

      vaddr++;
    }

    return n;
}

void updateProcessThreadCounts(AddrSpace* addrSpace, UpadateState updateState) {
    if(addrSpace == NULL) {
        printf("%s","--------------- AddressSpace is NULL!\n");
        return;
    }

    int processId = addrSpace->processId;

    switch(updateState) {
        case SLEEP:
            processTable->processEntries[processId]->awakeThreadCount -= 1;
            processTable->processEntries[processId]->sleepThreadCount += 1;
            break;
        case AWAKE:
            processTable->processEntries[processId]->awakeThreadCount += 1;
            processTable->processEntries[processId]->sleepThreadCount -= 1;
            break;
        case FINISH:
            processTable->processEntries[processId]->awakeThreadCount -= 1;
            break;
    }
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
    	printf("%s","Bad pointer passed to Create\n");
    	delete buf;
    	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
    	printf("%s","Can't allocate kernel buffer in Open\n");
    	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
    	printf("%s","Bad pointer passed to Open\n");
    	delete[] buf;
    	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	   return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.

    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;

    if ( !(buf = new char[len]) ) {
    	printf("%s","Error allocating kernel buffer for write!\n");
    	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
    	    printf("%s","Bad pointer passed to to write: data not written\n");
    	    delete[] buf;
    	    return;
        }
    }

    if ( id == ConsoleOutput) {
        for (int ii=0; ii<len; ii++) {
            printf("%c",buf[ii]);
        }
    } else {
    	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
    	    f->Write(buf, len);
    	} else {
    	    printf("%s","Bad OpenFileId passed to Write\n");
    	    len = -1;
    	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;

    if ( !(buf = new char[len]) ) {
    	printf("%s","Error allocating kernel buffer in Read\n");
    	return -1;
    }

    if ( id == ConsoleInput) {
        //Reading from the keyboard
        scanf("%s", buf);

        if ( copyout(vaddr, len, buf) == -1 ) {
    	   printf("%s","Bad pointer passed to Read: data not copied\n");
        }
    } else {
    	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
    	    len = f->Read(buf, len);
    	    if ( len > 0 ) {
    	        //Read something from the file. Put into user's address space
      	        if ( copyout(vaddr, len, buf) == -1 ) {
                    printf("%s","Bad pointer passed to Read: data not copied\n");
                }
    	    }
    	} else {
                printf("%s","Bad OpenFileId passed to Read\n");
                len = -1;
    	   }
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

int Rand_sys(int mod, int plus) {
  return rand() % mod + plus;
}

int GetThreadArgs_sys() {
  //cout << "currentThread::::" << currentThread->getName() << "_" << currentThread->id << endl;
    return threadArgs[currentThread->id];
}

void PrintString_sys(unsigned int vaddr, int len) {
    char *buf = new char[len+1];    // Kernel buffer to put the name in
    int id;             // The openfile id

    if (!buf) {
        printf("%s","Can't allocate kernel buffer in Open\n");
        return;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
        printf("%s","Bad pointer passed to Open\n");
        delete[] buf;
        return;
    }

    buf[len]='\0';

    for (int ii=0; ii<len; ii++) {
        printf("%c",buf[ii]);
    }

    delete[] buf;
}

void PrintNum_sys(int num) {
    string numString = static_cast<ostringstream*>( &(ostringstream() << num) )->str();
    cout << numString;
}

void PrintNl_sys() {
    printf("\n");
}

void kernel_thread(int virtualAddress) {
    kernelLock->Acquire();
    //cout << "-------- launching KernelThread --------" << endl;
    // int virtualAddress = decode / 100;
    // int kernelThreadId = decode - virtualAddress * 100;
    machine->WriteRegister(PCReg, virtualAddress);
    machine->WriteRegister(NextPCReg, virtualAddress+4);
    currentThread->space->RestoreState();
    // cout << "currentThread->id: " << currentThread->id << endl;
    // cout << "currentThread->space->processId: " << currentThread->space->processId << endl;
    // int numPages = processTable->processEntries[currentThread->space->processId]->stackLocations[kernelThreadId];
    int stackRegForNewStack = processTable->processEntries[currentThread->space->processId]->stackLocations[currentThread->id] * PageSize + UserStackSize - 16;
    // machine->WriteRegister(StackReg, numPages * PageSize - 16 ); // TODO: need to calculate: currentThread->stackTop
    machine->WriteRegister(StackReg, stackRegForNewStack ); // TODO: need to calculate: currentThread->stackTop


    //cout << "stackRegForNewStack: " << stackRegForNewStack << " // should be 1024 bytes apart, stackRegForNewStak: " << stackRegForNewStack<<  endl;
    kernelLock->Release();
    machine->Run();
}

void exec_thread(int someIntThatIsEqualToZero) {
    kernelLock->Acquire();
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    kernelLock->Release();
    machine->Run();

}

bool isLastExecutingThread(Thread* tempCurrentThread) {
    //ProcessEntry* pe = processTable->processEntries[tempCurrentThread->space->processId];
    // cout << "awake: " << pe->awakeThreadCount << endl;
    // cout << "sleep: " << pe->sleepThreadCount << endl;
    if(tempCurrentThread->space->threadCount == 1) {
        return true;
    }
    return false;
}

// check before I do acquire if the lock is busy then the thread needs to go to sleep
// beofre I call acquire I need to know if the thread needs to go to sleep
// when it comes out of acquire reverse the counts
// when it gets the lock, it gets awake

bool isLastProcess() {
    if(processTable->runningProcessCount == 0) {
        return true;
    } else {
        return false;
    }
}

int handleMemoryFull(){
    printf("Entering handleMemoryFull()\n");
    cout << "Got here 3" << endl;
    ExtendedTranslationEntry* pageTable = currentThread->space->pageTable;
    // if(/*something that indicates FIFO replacement*/){ //TODO: FIFO or random replacement
    //}else if(/*something that indicates random replacement*/){
    int pageToBoot = rand () % NumPhysPages;
    if(ipt[pageToBoot].dirty){ // TODO: should be pageTable?
        cout << "Got here 3.1" << endl;
        int swapLocationPPN = swapfileBitmap->Find();
        cout << "Got here 3.1.1" << endl;
        swapfile->WriteAt(&(machine->mainMemory[pageToBoot * PageSize]), PageSize, PageSize * swapLocationPPN);
        cout << "Got here 3.1.2" << endl;
        pageTable[ipt[pageToBoot].virtualPage].diskLocation = SWAP;
        cout << "Got here 3.1.3" << endl;
        pageTable[ipt[pageToBoot].virtualPage].byteOffset = PageSize * swapLocationPPN;
    }

    cout << "Got here 3.2" << endl;
    return pageToBoot;
}

int handleIPTMiss(int virtualPage){
    int ppn = bitmap->Find();  //Find a physical page of memory
    printf("Entering handleIPTMiss(virtualPage: %d)\n", virtualPage);
    ExtendedTranslationEntry* pageTable = currentThread->space->pageTable;
    cout << "Got here 2" << endl;

    if ( ppn == -1 ) {
        cout << "Got here 2.1" << endl;
        ppn = handleMemoryFull();
    }
    if(pageTable[virtualPage].diskLocation == EXECUTABLE){
        cout << "Got here 2.3" << endl;
        currentThread->space->executable->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, pageTable[virtualPage].byteOffset);
        //executable->ReadAt(&(machine->mainMemory[pageTable[i].physicalPage * PageSize]), PageSize, 40 + pageTable[i].virtualPage * PageSize);

    }else if(pageTable[virtualPage].diskLocation == SWAP){
        cout << "Got here 2.4" << endl;
        //TODO: swap file handling
        swapfile->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, pageTable[virtualPage].byteOffset);

        cout << "Got here 2.5" << endl;
        // pagetTable[virtualPage]->diskLocation = ??;
        //NOTE: could add Clear from swapfile, cause now the page is in main memory
    }

    cout << "Got here 2.6" << endl;

    ipt[ppn].virtualPage = virtualPage;
    ipt[ppn].physicalPage = ppn;
    ipt[ppn].valid = TRUE;
    ipt[ppn].use = FALSE;
    ipt[ppn].dirty = FALSE;
    ipt[ppn].readOnly = FALSE;
    ipt[ppn].spaceOwner = currentThread->space; // extra piece of shit

    pageTable[virtualPage].physicalPage = ppn;
    pageTable[virtualPage].virtualPage = virtualPage;
    pageTable[virtualPage].valid = TRUE;
    pageTable[virtualPage].use = FALSE;
    pageTable[virtualPage].dirty = FALSE;
    pageTable[virtualPage].readOnly = FALSE;

    return ppn;
}

void HandlePageFault(int virtualAddress) {
    int virtualPage = virtualAddress / PageSize;
    printf("Entering HandlePageFault(virtualAddress: %d)\n", virtualAddress);
    printf("virtualPage = virtualAddress / PageSize: %d\n", virtualPage);
    ++tlbCounter;
    TranslationEntry* tlb = machine->tlb;

    int ppn = -1;
    
    IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts

    for(int i = 0; i < NumPhysPages; ++i) {
        //Found the physical page we need
        //cout << "ipt[i].virtualPage == virtualPage: " << ipt[i].virtualPage << " == " << virtualPage << endl;
        //cout << "ipt[i].spaceOwner == currentThread->space: " << ipt[i].spaceOwner << " == " << currentThread->space << endl;
        //cout << "ipt[i].valid: " << ipt[i].valid << endl;
        if(ipt[i].virtualPage == virtualPage && 
            ipt[i].spaceOwner == currentThread->space && 
            ipt[i].valid){
            ppn = i;
            cout << "Got here 1 || ppn: " << ppn << endl;
            break;
        }
    }
    if (ppn == -1) {
        //cout << "Got here 1.1" << endl
        ppn = handleIPTMiss( virtualPage );
    }
    
    //cout << "Got here 1.2" << endl;

    if(tlb[tlbCounter % 4].valid) {//TODO: check index
        ipt[tlb[tlbCounter % 4].physicalPage].dirty = tlb[tlbCounter % 4].dirty;
    }
   
    tlb[tlbCounter % 4].virtualPage   = ipt[ppn].virtualPage;
    tlb[tlbCounter % 4].physicalPage  = ipt[ppn].physicalPage;
    tlb[tlbCounter % 4].valid         = ipt[ppn].valid;
    tlb[tlbCounter % 4].use           = ipt[ppn].use;
    tlb[tlbCounter % 4].dirty         = ipt[ppn].dirty;
    tlb[tlbCounter % 4].readOnly      = ipt[ppn].readOnly;

    (void) interrupt->SetLevel(oldLevel); //restore interrupts
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall
    int virtualAddress = 0;

    if ( which == SyscallException ) {
    	switch (type) {
        default:
          DEBUG('a', "Unknown syscall - shutting down.\n");
        case SC_Halt:
            DEBUG('a', "Shutdown, initiated by user program.\n      ");
            //cout << "Halt is called by: " << currentThread->getName() << ", number of threads remaining in space: " << currentThread->space->threadCount <<  endl;
            currentThread->space->PrintPageTable();

            interrupt->Halt();
            break;
        case SC_Create:
            DEBUG('a', "Create syscall.\n");
            Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_Open:
            DEBUG('a', "Open syscall.\n");
            rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
            break;

        case SC_Write:
            DEBUG('a', "Write syscall.\n");
            Write_Syscall(machine->ReadRegister(4),
                  machine->ReadRegister(5),
                  machine->ReadRegister(6));
            break;
        case SC_Read:
            DEBUG('a', "Read syscall.\n");
            rv = Read_Syscall(machine->ReadRegister(4),
                  machine->ReadRegister(5),
                  machine->ReadRegister(6));
            break;
        case SC_Close:
            DEBUG('a', "Close syscall.\n");
            Close_Syscall(machine->ReadRegister(4));
            break;
        case SC_Yield:
            DEBUG('a', "Yield syscall.\n");
            kernelLock->Acquire();
            ProcessEntry* processEntry = processTable->processEntries[currentThread->space->processId];
            processEntry->sleepThreadCount += 1;
            processEntry->awakeThreadCount -= 1;
            kernelLock->Release();

            currentThread->Yield();

            kernelLock->Acquire();
            processEntry->sleepThreadCount -= 1;
            processEntry->awakeThreadCount += 1;
            kernelLock->Release();
            break;
        case SC_Fork:
            kernelLock->Acquire();
            DEBUG('a', "Fork syscall.\n");
            virtualAddress = machine->ReadRegister(4);
            Thread* kernelThread = new Thread("KernelThread");
            threadArgs[kernelThread->id] = machine->ReadRegister(5);
            kernelThread->space = currentThread->space;
            ++(currentThread->space->threadCount);
            int startStackLocation = kernelThread->space->NewPageTable();
            currentThread->space->RestoreState();
            processTable->processEntries[currentThread->space->processId]->stackLocations[kernelThread->id] = startStackLocation;

            kernelThread->Fork((VoidFunctionPtr)kernel_thread, virtualAddress);
            kernelLock->Release();
            break;
        case SC_Exec:
            //Exec instantiates a new user program
            DEBUG('a', "Exec syscall.\n");
            kernelLock->Acquire();
            //Increasing the process table count
            processTable->runningProcessCount += 1;
            virtualAddress = machine->ReadRegister(4);
            char* nameOfProcess = new char[32 + 1];
              if(copyin(virtualAddress, 32, nameOfProcess) == -1) {// Convert it to the physical address // read the contents from physical address, which will give you the name of the process to be executed
                DEBUG('a', "Copyin failed.\n");
                kernelLock->Release();
                return;
            }
            nameOfProcess[32] = '\0';
            OpenFile *filePointer = fileSystem->Open(nameOfProcess);

            if (filePointer){ // check if pointer is not null
              AddrSpace* as = new AddrSpace(nameOfProcess); // Create new addrespace for this executable file
              delete [] nameOfProcess; //TODO: MOVE THIS DELETE AROUND< PLS
              Thread* newThread = new Thread("ExecThread");
              newThread->space = as; //Allocate the space created to this thread's space
              processEntry = new ProcessEntry();
              processEntry->space = as;
              processEntry->spaceId = processCount;
              processTable->processEntries[processCount] = processEntry;
              processTable->processEntries[newThread->space->processId]->stackLocations[newThread->id] = as->StackTopForMain;
              //cout << "Start stack location for Exec_thread: " << processTable->processEntries[newThread->space->processId]->stackLocations[newThread->id]<< endl;

              rv = newThread->space->spaceId;
              newThread->Fork((VoidFunctionPtr)exec_thread, 0);
              kernelLock->Release();
            }else{
                printf("%s", "Couldn't open file\n");
                kernelLock->Release();
                break;
            }

            break;
        case SC_Exit:
            kernelLock->Acquire();
            printf("-----------Exit Output: %d\n", machine->ReadRegister(4));
            //Checks for last process and last thread
            bool isLastProcessVar = isLastProcess();
            bool isLastExecutingThreadVar = isLastExecutingThread(currentThread);
            if(isLastProcessVar && isLastExecutingThreadVar) {
            //This is the last process and last thread, can stop program
                DEBUG('a', "Last process and last thread, stopping program.\n");
                interrupt->Halt();
            } else if(!isLastProcessVar && isLastExecutingThreadVar) {
            //This is the last thread in a process, but not the last process, so we delete the entire addressspace
                  DEBUG('a', "Not last process and last thread, deleting process.\n");
                  delete currentThread->space;
                  processTable->runningProcessCount -= 1;
                  kernelLock->Release();
                  currentThread->Finish();

            }else if(!isLastExecutingThreadVar) {
              //Not last thread in process, so just delete thread
              DEBUG('a', "Not last thread in a process, deleting thread.\n");
              currentThread->space->DeleteCurrentThread();
              kernelLock->Release();
              currentThread->Finish();
            }
            break;
        case SC_CreateLock:
            DEBUG('a', "CreateLock syscall.\n");
            rv = CreateLock_sys(machine->ReadRegister(4),
                                machine->ReadRegister(5),
                                machine->ReadRegister(6));
            break;
        case SC_Acquire:
            DEBUG('a', "Acquire syscall.\n");
            Acquire_sys(machine->ReadRegister(4));
            break;
        case SC_Release:
            DEBUG('a', "Release syscall.\n");
            Release_sys(machine->ReadRegister(4));
            break;
        case SC_DestroyLock:
            DEBUG('a', "DestroyLock syscall.\n");
            DestroyLock_sys(machine->ReadRegister(4));
            break;
        case SC_CreateCondition:
            DEBUG('a', "CreateCondition syscall.\n");
            rv = CreateCondition_sys(machine->ReadRegister(4),
                                machine->ReadRegister(5),
                                machine->ReadRegister(6));
            break;
        case SC_Wait:
            DEBUG('a', "Wait syscall.\n");
            Wait_sys(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_Signal:
            DEBUG('a', "Signal syscall.\n");
            Signal_sys(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_Broadcast:
            DEBUG('a', "Broadcast syscall.\n");
            Broadcast_sys(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_DestroyCondition:
            DEBUG('a', "DestroyCondition syscall.\n");
            DestroyCondition_sys(machine->ReadRegister(4));
            break;
        case SC_Rand:
            DEBUG('a', "Rand syscall.\n");
            rv = Rand_sys(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_GetThreadArgs:
            DEBUG('a', "Get Thread Args syscall.\n");
            rv = GetThreadArgs_sys();
            break;
        case SC_PrintString:
            DEBUG('a', "Print String syscall.\n");
            PrintString_sys(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
        case SC_PrintNum:
            DEBUG('a', "Print Num syscall.\n");
            PrintNum_sys(machine->ReadRegister(4));
            break;
        case SC_PrintNl:
            DEBUG('a', "Print Nl syscall.\n");
            PrintNl_sys();
            break;
        }
	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    } else if(which == PageFaultException) {
        HandlePageFault(machine->ReadRegister(BadVAddrReg));
    } else {
cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<< " in " << currentThread->getName() << endl;
      interrupt->Halt();
    }
}
