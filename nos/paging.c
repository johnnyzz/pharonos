#define ExternalObjectsArray 38
#define FirstLinkIndex 0
#define NextLinkIndex 0
#define SchedulerAssociation 3
#define ValueIndex 1
#define ProcessListsIndex 0
#define ActiveProcessIndex 1
#include "ints.h"
#include "sq.h"
/**
 *  This file handles the basic stuff related to memory paging
**/

extern unsigned long tabs;

// we add 1024 extra so we can get aligned to 1024 blocks

// returns a pointer to an array of 1024 page directory entries
long* page_directory()
{
	static long pd[1024+1024];
	return (long*)((long)(pd+1023) & 0xfffff000);
}

// returns a pointer to an array of 1024 page tables, of 1024 entries each.
// 1024 pages per table * 1024 tables = 1 M entries
// 4 KB per page * 1 M pages = 4 GB
long* page_tables_start()
{
	static long pt[1024*1024+1024];
	return (long*)((long)(pt+1023) & 0xfffff000);
}

void generate_empty_page_directory()
{
	//set each entry to not present
	int i = 0;
	long *next_page_table = page_tables_start();
	for(i = 0; i < 1024; i++, next_page_table += 1024)
	{
		//attribute: supervisor level, read/write, present.
		page_directory()[i] = (long)next_page_table | 3;
	}
	
	next_page_table = page_tables_start();
	//~ next_page_table += 1024 * 10;
	//~ 
	//~ for(i = 0x10; i < 1020; i++, next_page_table += 1024)
	//~ {
		//~ //attribute: supervisor level, read/write, present.
		//~ page_directory()[i] = (long)next_page_table;
	//~ }
}


void generate_empty_page_tables()
{
	// holds the physical address where we want to start mapping these pages to.
	// in this case, we want to map these pages to the very beginning of memory.
	unsigned int address = 0; 
	unsigned int i;
	 
	//we will fill all 1024*1024 entries, mapping 4 gigabytes
	for(i = 0; i < 1024*1024; i++)
	{
		page_tables_start()[i] = address | 3; // attributes: supervisor level, read/write, present.
		address = address + 4096; //advance the address to the next page boundary
	}
}

void setTableReadOnly(unsigned int *pageTable, int firstIndex, int lastIndex)
{
	int i;
	//printf_pocho("");
	for (i = firstIndex; i <= lastIndex; i++)
		pageTable[i] = pageTable[i] & 0xFFFFFFFD;
	
}

void setTableReadWrite(unsigned int *pageTable, int firstIndex, int lastIndex)
{
	int i;
	//printf_pocho("");
	for (i = firstIndex; i <= lastIndex; i++)
		pageTable[i] = pageTable[i] | 0x00000002;
	
}

void makeReadOnly(unsigned long from, unsigned long to){
	printf_pochoTab(tabs,"Read only from: %d to: %d\n",from,to);
	unsigned int *pageDirectory;

	asm volatile("movl %%cr3, %0": "=a" (pageDirectory));

	int i;

	unsigned int firstDirIndex   = from / (1024 * 1024 * 4);
	unsigned int lastDirIndex    = to   / (1024 * 1024 * 4);
	unsigned int firstTableIndex = (from & 0x003FF000) >> 12;
	unsigned int lastTableIndex  = (to   & 0x003FF000) >> 12;
	
	printf_pochoTab(tabs,"Page Directory: %d\n,First table: %d, last table: %d,\n first entry in first table: %d, last entry in last table: %d\n",
				pageDirectory, firstDirIndex, lastDirIndex, firstTableIndex, lastTableIndex);
	
	for (i = firstDirIndex; i <= lastDirIndex; i++)
	{
		unsigned int firstIndex = 0;
		unsigned int lastIndex = 1023;
		if (i == firstDirIndex)
			firstIndex = firstTableIndex;
		
		if (i == lastDirIndex)
			lastIndex = lastTableIndex;
			
		setTableReadOnly((unsigned int*)(pageDirectory[i] & 0xFFFFF000), firstIndex, lastIndex);
	}
	
}

void makeReadWrite(){
	unsigned int *pageDirectory;

	asm volatile("movl %%cr3, %0": "=a" (pageDirectory));

	int i;
	int from = 0;
	int to = 300000000;
	unsigned int firstDirIndex   = from / (1024 * 1024 * 4);
	unsigned int lastDirIndex    = to   / (1024 * 1024 * 4);
	unsigned int firstTableIndex = (from & 0x003FF000) >> 12;
	unsigned int lastTableIndex  = (to   & 0x003FF000) >> 12;
	
	printf_pocho("Page Directory: %d\n,First table: %d, last table: %d,\n first entry in first table: %d, last entry in last table: %d\n",
				pageDirectory, firstDirIndex, lastDirIndex, firstTableIndex, lastTableIndex);
	
	for (i = firstDirIndex; i <= lastDirIndex; i++)
	{
		unsigned int firstIndex = 0;
		unsigned int lastIndex = 1023;
		if (i == firstDirIndex)
			firstIndex = firstTableIndex;
		
		if (i == lastDirIndex)
			lastIndex = lastTableIndex;
			
		setTableReadWrite((unsigned int*)(pageDirectory[i] & 0xFFFFF000), firstIndex, lastIndex);
	}
	
}


usqInt getScheduler(){
	extern sqInt specialObjectsOop;
	usqInt association;
	association  = longAt((specialObjectsOop + (BASE_HEADER_SIZE)) + (SchedulerAssociation << (SHIFT_FOR_WORD)));
	return longAt((association + (BASE_HEADER_SIZE)) + (ValueIndex << (SHIFT_FOR_WORD)));
} 

void saveSpecialPages(){
	extern t_IRQSemaphores IRQSemaphores;
	extern usqInt activeContext, youngStart;
	usqInt activeProcess, scheduler;
	scheduler = getScheduler();
	activeProcess = longAt((scheduler + (BASE_HEADER_SIZE)) + (ActiveProcessIndex << (SHIFT_FOR_WORD)));
	saveExternalSemaphorePages(IRQSemaphores[1]); 	//keyboard
	saveExternalSemaphorePages(IRQSemaphores[3]);   //serial port
	saveExternalSemaphorePages(IRQSemaphores[4]);	//serial port
	saveExternalSemaphorePages(IRQSemaphores[12]);	//mouse
	saveExternalSemaphorePages(IRQSemaphores[15]);	//page Fault
	saveProcessListPagesWithPriority(40);
	saveProcessListPagesWithPriority(70);
	saveProcessListPagesWithPriority(71);
	saveSnapshotPage(activeProcess);
	saveSnapshotPage(activeContext);
	saveSnapshotPage(scheduler);
	if ((youngStart && 0xFFFFF000) != youngStart) saveSnapshotPage(youngStart);	
	//saveExternalSemaphorePages(IRQSemaphores[8]);	//cmos
}	

void saveProcessListPagesWithPriority(sqInt priority){
	extern sqInt specialObjectsOop,nilObj;
	usqInt association,scheduler,processLists,processList,firstProcess;
	processLists = longAt((getScheduler() + (BASE_HEADER_SIZE)) + (ProcessListsIndex << (SHIFT_FOR_WORD)));
	processList = longAt((processLists + (BASE_HEADER_SIZE)) + ((priority - 1) << (SHIFT_FOR_WORD)));
	saveSnapshotPage(processList);
	firstProcess = longAt((processList + (BASE_HEADER_SIZE)) + (FirstLinkIndex << (SHIFT_FOR_WORD)));
	if (firstProcess != nilObj) saveProcessList(firstProcess);
}	

void saveExternalSemaphorePages(sqInt index){
	extern sqInt specialObjectsOop,nilObj;
	sqInt array, semaphore,firstProcess;
	printf_pochoTab(tabs,"Entre saveExternalSemaphorePages %d\n", index);	
	array = longAt((specialObjectsOop + (BASE_HEADER_SIZE)) + (ExternalObjectsArray << (SHIFT_FOR_WORD)));
	semaphore = longAt((array + (BASE_HEADER_SIZE)) + ((index - 1) << (SHIFT_FOR_WORD)));	
	saveSnapshotPage(semaphore);
	firstProcess = longAt((semaphore + (BASE_HEADER_SIZE)) + (FirstLinkIndex << (SHIFT_FOR_WORD)));
	if ((firstProcess == nilObj) || (index == 0)) return;
	saveProcessList(firstProcess);
	printf_pochoTab(tabs,"Sali saveExternalSemaphorePages\n");
}

void saveProcessList(sqInt aProcess){
	extern sqInt nilObj;
	sqInt actual;
	actual = aProcess;
	while(actual != nilObj){
		saveSnapshotPage(actual);
		actual = longAt((actual + (BASE_HEADER_SIZE)) + (NextLinkIndex << (SHIFT_FOR_WORD)));
	}
}

void saveSnapshotPage(unsigned long virtualAddressFailure){
	extern Computer computer;
	printf_pochoTab(tabs,"Entre a saveSnapshotPage en la direccion:%d\n",virtualAddressFailure);
	unsigned long pageStart = virtualAddressFailure & 0xFFFFF000;
	if (alreadySaved(pageStart)) {printf_pochoTab(tabs,"Sali de saveSnapshotPage por el alreadyStart\n");return;}
	unsigned long saved = computer.snapshot.pagesSaved;
	computer.snapshot.pages[saved].virtualAddress = pageStart;
	computer.snapshot.pages[saved].physicalAddress = pageStart;
	//printf_pocho("estructura: %x, posicion actual: %x \n", computer->snapshot.pages, computer->snapshot.pages[saved].contents);
	memcpy(computer.snapshot.pages[saved].contents, pageStart, 4096); 	
	changeDirectoryToReadWrite(virtualAddressFailure);
	computer.snapshot.pagesSaved = saved + 1;
	printf_pochoTab(tabs,"Sali de saveSnapshotPage\n");
}

void changeDirectoryToReadWrite(unsigned long virtualAddressFailure){
	unsigned long directoryIndex, pageTableIndex, pageDirectoryEntry, *directory, *pageTable, *pageTableEntry;
	asm("movl %%cr3, %0" : "=a" (directory) );
	directoryIndex = virtualAddressFailure >> 22;
	directoryIndex &= 0x000003FF;
	pageDirectoryEntry = directory[directoryIndex];
	pageTable = pageDirectoryEntry & 0xfffff000;
	pageTableIndex = virtualAddressFailure >> 12;
	pageTableIndex &= 0x000003FF;
	pageTableEntry = &pageTable[pageTableIndex];
	*pageTableEntry |= 0x00000002;
}

int alreadySaved(sqInt pageStart){
	extern Computer computer;	
	unsigned long i;
	for (i=0; i<computer.snapshot.pagesSaved; i++)
		if (computer.snapshot.pages[i].virtualAddress == pageStart) return 1;
	return 0;
}

void enable_paging_in_hardware()
{
	unsigned int cr0;
	long *pd = page_directory();
	
	asm volatile("xchg %%bx, %%bx" ::: "ebx");
	
	//moves pd (which is a pointer) into the cr3 register.
	asm volatile("mov %0, %%cr3":: "b"(pd));
	
	//reads cr0, switches the "paging enable" bit, and writes it back.
	
	asm volatile("mov %%cr0, %0": "=b"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "b"(cr0));
}


void enable_paging()
{
	generate_empty_page_tables();
	generate_empty_page_directory();
	enable_paging_in_hardware();
	
	asm volatile("xchg %%bx, %%bx" ::: "ebx"); // This is only qemu debugging stuff...
}
