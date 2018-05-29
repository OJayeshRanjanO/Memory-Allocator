#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"
#include <string.h>
void iterator(sf_free_header *temp);
void sf_freeHelper(void *ptr);
sf_footer * getFooter(void *ptr);
uint64_t getSize(void *ptr);
bool blockAllocator(size_t size, uint64_t padding);
void* blockFinder(size_t size, uint64_t padding);
int previousBlockAllocationChecker(void * ptr);
int nextBlockAllocationChecker(void *ptr);
void * getNextBlockHeader(void* ptr);
void removeNode(sf_free_header * ptr);
void addNode(sf_free_header * ptr);
void * getPreviousBlockHeader(void* ptr);
void coalesceLeft(sf_free_header * currentAllocatedBlockHeader);
void coalesceRight(sf_free_header * currentAllocatedBlockHeader);
sf_header * getPayload(void* ptr);
void changeFooterStatus(sf_footer * ptr ,int alloc , int block_size);
void changeHeaderStatus(void * ptr ,int alloc , int block_size , int unused_bits , int padding_size);

// All functions you make for the assignment must be implemented in this file.
// Do not submit your assignment with a main function in this file.
// If you submit with a main function in this file, you will get a zero.

sf_free_header* freelist_head = NULL;

sf_free_header* sbrkStart = NULL;
sf_footer *sbrkEnd = NULL;

size_t availableSize = 0;

bool startOfProgram = true;

size_t pages = 0;

size_t allocationCount =0 ;
size_t freeCount = 0;
size_t coalesceCount = 0;
size_t internalFragmentation = 0;
size_t externalFragmentaiton = 0;


void *sf_malloc(size_t size){
	errno = 0;
	if(size == 0){
		errno = ENOMEM;
		return NULL;
	}
	uint64_t padding = (size%16 == 0) ? 0 : 16 - (size%16);
	if(startOfProgram){
		sbrkStart = sf_sbrk(0);//Start of the heap
		freelist_head = sbrkStart;
		bool blockFound = blockAllocator((size + 16 + padding), padding);
		startOfProgram = false;
		if(blockFound){
			allocationCount++;
			return blockFinder(size, padding);
		}
		else{
			errno = ENOMEM;
			return NULL; //SHOULD BE ERRNO
		}
	}
	else{
		void* block = blockFinder(size, padding);
		if(block!=NULL){
			return block;
		}
		else{
			bool blockFound = blockAllocator((size + 16 + padding), padding);//This coaleses
			if(blockFound){
				allocationCount++;
				return blockFinder(size, padding);
			}
			else{
				errno = ENOMEM;
				return NULL; //SHOULD BE ERRNO
			}

		}

	}

}

sf_footer * pageFooter = NULL;
sf_footer * pointerToEndOfPage = NULL;
sf_footer * endOfCurrentPage = NULL;
bool blockAllocator(size_t size, uint64_t padding){
	if(startOfProgram){
		size_t sbrkTimes = (size / 4097) + 1;
		pages += sbrkTimes;
		if(pages > 4){
			return false;
		}
		for(int i = 0; i < sbrkTimes ; i++){
			sbrkEnd = sf_sbrk(1);
			pageFooter = ((void*)sbrkEnd) - 8;
			availableSize+=4096;
		}
		changeHeaderStatus(freelist_head,0,availableSize,-1,0);
		changeFooterStatus(pageFooter,0,availableSize);
		return true;

	}
	else{
		size_t sizeLeftFromPreviousPage = 0;
		if(pageFooter->alloc != 1){
			sizeLeftFromPreviousPage = getSize(pageFooter);
		}
		size_t sbrkTimes = ((size-sizeLeftFromPreviousPage) / 4097) + 1;
		pages += sbrkTimes;
		if(pages >4){
			return false;
		}
		int i = 0;
		for(i = 0 ; i < sbrkTimes; i++){
			if(i == 0){//Setting the head to starting of page (Needs to be coalesed)
				sf_free_header * nextToHead = freelist_head;
				freelist_head = sf_sbrk(0);
				freelist_head->next = nextToHead;
				freelist_head->prev = NULL;
				if(nextToHead!=NULL){
					nextToHead->prev = NULL;
				}
			}
			sbrkEnd = sf_sbrk(1);
			pageFooter = ((void*)sbrkEnd) - 8;
			availableSize+=4096;
			externalFragmentaiton+=4096;

		}

		changeHeaderStatus(freelist_head,0,i*4096,-1,0);
		changeFooterStatus(pageFooter,0,i*4096);
		if(getFooter(getPreviousBlockHeader(freelist_head))->alloc==0){
			coalesceCount++;
			coalesceLeft(freelist_head);
		}
		return true;
	}
}

void* blockFinder(size_t size, uint64_t padding){
	if(size <= 0){
		return NULL;
	}
	size_t requiredSize = size + 16 + padding;
	sf_free_header * cursor = freelist_head;
	while(cursor!=NULL){
		if(requiredSize <= getSize(cursor)){
			if((getSize(cursor) - requiredSize) < 32){//If splinter can form them take the entire block
				removeNode(cursor);
				changeHeaderStatus(cursor,1,getSize(cursor),-1,padding);
				changeFooterStatus(getFooter(cursor),1,getSize(cursor));

				internalFragmentation += 16 + padding;
				externalFragmentaiton-=requiredSize;
				return getPayload(cursor);
			}
			else{
				if(previousBlockAllocationChecker(cursor)==0){
					size_t sizeOfBlock = getSize(cursor);
					removeNode(cursor);

					sf_footer * cursorFooter = getFooter(cursor);
					changeHeaderStatus(cursor,1,requiredSize,-1,padding);
					changeFooterStatus(((void*)cursor)+(requiredSize-8),1,requiredSize);

					sf_free_header * nextFreeBlockHeader =  ((void*)cursor)+(requiredSize);
					changeHeaderStatus(nextFreeBlockHeader,0,sizeOfBlock - requiredSize,-1,-1);
					changeFooterStatus(cursorFooter,0,sizeOfBlock - requiredSize);

					addNode(nextFreeBlockHeader);
				}
				else{
					size_t sizeOfBlock = getSize(cursor);
					removeNode(cursor);

					changeHeaderStatus(cursor,1,requiredSize,-1,padding);
					changeFooterStatus(getFooter(cursor),1,requiredSize);

					sf_free_header * nextFreeBlockHeader =  getNextBlockHeader(cursor);
					addNode(nextFreeBlockHeader);
					changeHeaderStatus(nextFreeBlockHeader,0,sizeOfBlock - requiredSize,-1,-1);
					changeFooterStatus(getFooter(nextFreeBlockHeader),0,sizeOfBlock - requiredSize);
				}
				externalFragmentaiton-=requiredSize;
				internalFragmentation += 16 + padding;
				return getPayload(cursor);
			}
		}
		cursor = cursor->next;
	}
	// errno = ENOMEM;
	return NULL;
}


bool validatePointer(void * ptr);

void sf_free(void *ptr){
	errno = 0;
	if(ptr==NULL){
		return;
	}
	if(validatePointer(ptr)){
		sf_freeHelper(ptr);
	}
}

void sf_freeHelper(void *ptr){
	sf_free_header *currentAllocatedBlockHeader = ptr - 8;
	sf_footer * currentAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);

	if(freelist_head == NULL){//CASE 1
		addNode(currentAllocatedBlockHeader);
		freelist_head->header.alloc = 0;
		freelist_head->header.block_size = getSize(currentAllocatedBlockHeader) >> 4;

		currentAllocatedBlockFooter->alloc = 0;
		currentAllocatedBlockFooter->block_size = getSize(currentAllocatedBlockHeader) >> 4;
		freeCount++;
		externalFragmentaiton+=getSize(currentAllocatedBlockHeader);
		internalFragmentation = internalFragmentation - ((currentAllocatedBlockHeader->header.padding_size)+16);
	}
	else{
		if(previousBlockAllocationChecker(currentAllocatedBlockHeader)==1  && nextBlockAllocationChecker(currentAllocatedBlockHeader)==1){
			addNode(currentAllocatedBlockHeader);

			currentAllocatedBlockHeader->header.alloc = 0;
			currentAllocatedBlockHeader->header.block_size = getSize(currentAllocatedBlockHeader) >> 4;
			currentAllocatedBlockHeader->header.padding_size = 0;

			currentAllocatedBlockFooter->alloc = 0;
			currentAllocatedBlockFooter->block_size = getSize(currentAllocatedBlockHeader) >> 4;
			freeCount++;
			coalesceCount++;
			externalFragmentaiton+=getSize(currentAllocatedBlockHeader);
			internalFragmentation = internalFragmentation - ((currentAllocatedBlockHeader->header.padding_size)+16);

		}else if(previousBlockAllocationChecker(currentAllocatedBlockHeader)==1  && nextBlockAllocationChecker(currentAllocatedBlockHeader)==0){
			coalesceRight(currentAllocatedBlockHeader);
			freeCount++;
			coalesceCount++;
		}else if(previousBlockAllocationChecker(currentAllocatedBlockHeader)==0  && nextBlockAllocationChecker(currentAllocatedBlockHeader)==1){
			coalesceLeft(currentAllocatedBlockHeader);
			freeCount++;
			coalesceCount++;
		}else if(previousBlockAllocationChecker(currentAllocatedBlockHeader)==0  && nextBlockAllocationChecker(currentAllocatedBlockHeader)==0){
			sf_free_header * previousFreeBlockHeader = getPreviousBlockHeader(currentAllocatedBlockHeader);
			sf_free_header * nextFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);

			sf_footer * nextFreeBlockFooter = getFooter(nextFreeBlockHeader);

			size_t currentBlockSize = getSize(currentAllocatedBlockHeader);
			size_t previousBlockSize = getSize(previousFreeBlockHeader);
			size_t nextBlockSize = getSize(nextFreeBlockHeader);

			nextFreeBlockFooter->alloc = 0;
			nextFreeBlockFooter->block_size = (currentBlockSize + previousBlockSize + nextBlockSize) >> 4;

			previousFreeBlockHeader->header.alloc = 0;
			previousFreeBlockHeader->header.block_size = (currentBlockSize + previousBlockSize + nextBlockSize) >> 4;
			previousFreeBlockHeader->header.padding_size = 0;

			removeNode(previousFreeBlockHeader);
			removeNode(nextFreeBlockHeader);

			addNode(previousFreeBlockHeader);
			freeCount++;
			coalesceCount++;

			externalFragmentaiton+=currentBlockSize + previousBlockSize + nextBlockSize;
			internalFragmentation = internalFragmentation - ((currentAllocatedBlockHeader->header.padding_size)+16);
		}

	}
}

void *sf_realloc(void *ptr, size_t size){
	errno = 0;
	if(size == 0){
		errno = EINVAL;
		return NULL;
	}
	if(validatePointer(ptr)){
		sf_header * currentAllocatedBlockHeader = ptr - 8;
		size_t padding = (size%16 == 0) ? 0 : 16 - (size%16);
		size_t requiredSize = size + 16 + padding;
		if(requiredSize <= getSize(currentAllocatedBlockHeader)){//SHRINK

			sf_free_header * nextFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);

			size_t currentBlockSize = getSize(currentAllocatedBlockHeader);
			size_t nextBlockSize = getSize(nextFreeBlockHeader);

			if((getSize(currentAllocatedBlockHeader) - requiredSize) < 32){//SPLINTER
				if(nextBlockAllocationChecker(currentAllocatedBlockHeader) == 1){//nextBlock is not free
					internalFragmentation = internalFragmentation + (padding - currentAllocatedBlockHeader->padding_size);
					currentAllocatedBlockHeader->padding_size = padding;
					return getPayload(currentAllocatedBlockHeader);
				}
				else{
					removeNode(nextFreeBlockHeader); // Remove the adjacent free block

					//Setting up the allocated block
					internalFragmentation = internalFragmentation + (padding - currentAllocatedBlockHeader->padding_size);
					currentAllocatedBlockHeader->alloc = 1;
					currentAllocatedBlockHeader->block_size = requiredSize >> 4;
					currentAllocatedBlockHeader->padding_size = padding;

					sf_footer * newAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);
					newAllocatedBlockFooter->alloc = 1;
					newAllocatedBlockFooter->block_size = requiredSize >> 4;

					//Setting up the free block
					sf_free_header * newFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);
					changeHeaderStatus(newFreeBlockHeader,0,((currentBlockSize + nextBlockSize) - requiredSize),-1,0);

					sf_footer * newFreeBlockFooter = getFooter(newFreeBlockHeader);
					changeFooterStatus(newFreeBlockFooter,0,((currentBlockSize + nextBlockSize) - requiredSize));

					addNode(newFreeBlockHeader);
					coalesceCount++;
					freeCount++;
					allocationCount++;
					externalFragmentaiton = (externalFragmentaiton + (currentBlockSize + nextBlockSize))-requiredSize;
					return getPayload(currentAllocatedBlockHeader);
				}
			}else if((getSize(currentAllocatedBlockHeader) - requiredSize) >= 32){//NO SPLINTER
				if(nextBlockAllocationChecker(currentAllocatedBlockHeader) == 1){
					internalFragmentation = (internalFragmentation - currentAllocatedBlockHeader->padding_size) + padding;


					//Setting up the allocated block
					changeHeaderStatus(currentAllocatedBlockHeader,1,requiredSize,-1,padding);
					sf_footer * newAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);
					changeFooterStatus(newAllocatedBlockFooter,1,requiredSize);

					//Setting up the free block
					sf_free_header * newFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);
					changeHeaderStatus(newFreeBlockHeader,0,((currentBlockSize) - requiredSize),-1,0);
					sf_footer * newFreeBlockFooter = getFooter(newFreeBlockHeader);
					changeFooterStatus(newFreeBlockFooter,0,((currentBlockSize) - requiredSize));

					addNode(newFreeBlockHeader);
					coalesceCount++;
					freeCount++;
					allocationCount++;
					externalFragmentaiton = (externalFragmentaiton + (currentBlockSize))-requiredSize;
					return getPayload(currentAllocatedBlockHeader);
				}else{
					removeNode(nextFreeBlockHeader); // Remove the adjacent free block
					internalFragmentation = (internalFragmentation - currentAllocatedBlockHeader->padding_size) + padding;

					//Setting up the allocated block
					changeHeaderStatus(currentAllocatedBlockHeader,1,requiredSize, -1, padding);
					sf_footer * newAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);
					changeFooterStatus(newAllocatedBlockFooter,1,requiredSize);

					//Setting up the free block
					sf_free_header * newFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);
					changeHeaderStatus(newFreeBlockHeader,0,((currentBlockSize + nextBlockSize) - requiredSize),-1,0);
					sf_footer * newFreeBlockFooter = getFooter(newFreeBlockHeader);
					changeFooterStatus(newFreeBlockFooter,0,((currentBlockSize + nextBlockSize) - requiredSize));

					addNode(newFreeBlockHeader);
					coalesceCount++;
					freeCount++;
					allocationCount++;
					externalFragmentaiton = (externalFragmentaiton + (currentBlockSize + nextBlockSize))-requiredSize;
					return getPayload(currentAllocatedBlockHeader);

				}
			}
		}
		else{//Expanding
			if(nextBlockAllocationChecker(currentAllocatedBlockHeader) == 1){
				void * ptr2 = sf_malloc(requiredSize-16);
				if(ptr2 != NULL){
					memcpy(ptr2,((void*)currentAllocatedBlockHeader) + 8,getSize(currentAllocatedBlockHeader)-8);
					sf_free(ptr);
					return ptr2;
				}
				else{
					errno = ENOMEM;
					return NULL;
				}
			}else{
				sf_free_header * nextFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);
				size_t nextBlockSize = getSize(nextFreeBlockHeader);
				size_t currentBlockSize = getSize(currentAllocatedBlockHeader);
				if((nextBlockSize + currentBlockSize) < requiredSize){//If the fit perfectly then no need to find new block, move to else
					void * ptr2 = sf_malloc(requiredSize-16);
					if(ptr2 != NULL){
						memcpy(ptr2,((void*)currentAllocatedBlockHeader) + 8,getSize(currentAllocatedBlockHeader)-8);
						sf_free(ptr);
						return ptr2;
					}
					else{
						errno = ENOMEM;
						return NULL;
					}
				}
				else{//Splitting the block
					removeNode(nextFreeBlockHeader); // Remove the adjacent free block
					internalFragmentation = (internalFragmentation - currentAllocatedBlockHeader->padding_size) + padding;
					//Setting up the allocated block
					changeHeaderStatus(currentAllocatedBlockHeader,1,requiredSize, -1, padding);
					sf_footer * newAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);
					changeFooterStatus(newAllocatedBlockFooter,1,requiredSize);

					//Setting up the free block
					if(32 <= (nextBlockSize + currentBlockSize)-requiredSize){//Can form a new block
						sf_free_header * newFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);
						changeHeaderStatus(newFreeBlockHeader,0,((currentBlockSize + nextBlockSize) - requiredSize),-1,-1);
						sf_footer * newFreeBlockFooter = getFooter(newFreeBlockHeader);
						changeFooterStatus(newFreeBlockFooter,0,((currentBlockSize + nextBlockSize) - requiredSize));

						addNode(newFreeBlockHeader);
						coalesceCount++;
					}
					freeCount++;
					allocationCount++;
					externalFragmentaiton = (externalFragmentaiton + (((int)(currentBlockSize + nextBlockSize)))-((int)requiredSize));
					return getPayload(currentAllocatedBlockHeader);

				}
			}

		}
	}
	else{
		errno = ENOMEM;
		return NULL;
	}
	errno = ENOMEM;
	return NULL;
}


int sf_info(info *meminfo){
	externalFragmentaiton = 0;
	if(meminfo!=NULL){
		sf_free_header * temp = freelist_head;
		while(temp!=NULL){
			externalFragmentaiton+= (temp->header.block_size << 4);
			temp = temp->next;
		}

		meminfo->internal = internalFragmentation;
		meminfo->external = externalFragmentaiton;
		meminfo->allocations = allocationCount;
		meminfo->frees = freeCount;
		meminfo->coalesce = coalesceCount;
		return 0;
	}else{
		errno = ENOMEM;
		return -1;
	}
	return -1;
}

sf_header * getPayload(void* ptr){
	return ptr + 8;
}

void changeHeaderStatus(void * ptr ,int alloc , int block_size , int unused_bits , int padding_size){
	sf_header * newType = ptr;
	newType->alloc = alloc;
	newType->block_size = block_size >> 4;
	if(unused_bits != -1){
		newType->unused_bits = unused_bits;	
	}
	if(padding_size != -1){
		newType->padding_size = padding_size;
	}

}

void changeFooterStatus(sf_footer * ptr ,int alloc , int block_size){
	ptr->alloc = alloc;
	ptr->block_size = block_size >> 4;
}

bool validatePointer(void * ptr){
	sf_header *ptrTesterHeader = ptr -8;
	errno = 0;
	if(ptr != NULL){
		if((void*)ptrTesterHeader < (void*)sbrkEnd && (void*)ptrTesterHeader >= (void*)sbrkStart){
			sf_footer *ptrTesterFooter = ((void*)ptrTesterHeader) + (ptrTesterHeader->block_size <<4);
			if((void*)ptrTesterFooter <= (void*)sbrkEnd && (void*)ptrTesterFooter > (void*)sbrkStart){
				if(((sf_footer*)(((void*)ptrTesterFooter) - 8))->block_size == ptrTesterHeader->block_size && ((sf_footer*)(((void*)ptrTesterFooter) - 8))->alloc == ptrTesterHeader->alloc){
					if(((sf_footer*)(((void*)ptrTesterFooter) - 8))->alloc != 0 && ptrTesterHeader->alloc != 0){
						return true;
					}
					else if(((sf_footer*)(((void*)ptrTesterFooter) - 8))->alloc != 1 && ptrTesterHeader->alloc != 1){
						return true;
					}
					else{
						errno = EFAULT;
						return false; //ERROR CODE
					}
				}
				else{
					errno = ENOMEM;
					return false; //ERROR CODE	
				}
			}
			else{
				errno = ENOMEM;
				return false; //ERROR CODE	
			}
		}
		else{
			errno = ENOMEM;
			return false; //ERROR CODE
		}
	}
	else{
		errno = ENOMEM;
		return false; //ERROR CODE
	}
}


void coalesceRight(sf_free_header * currentAllocatedBlockHeader){
	sf_free_header * nextFreeBlockHeader = getNextBlockHeader(currentAllocatedBlockHeader);

	sf_footer* oldFooter = ((void*)currentAllocatedBlockHeader)+((currentAllocatedBlockHeader->header.block_size<<4) - 8);//Changing the old footer
	oldFooter->alloc = 0;

	size_t currentBlockSize = getSize(currentAllocatedBlockHeader);
	size_t nextBlockSize = getSize(nextFreeBlockHeader);

	removeNode(nextFreeBlockHeader);

	sf_footer * nextFreeBlockFooter = getFooter(nextFreeBlockHeader);

	nextFreeBlockFooter->alloc = 0;
	nextFreeBlockFooter->block_size = (currentBlockSize + nextBlockSize) >> 4;

	currentAllocatedBlockHeader->header.alloc = 0;
	currentAllocatedBlockHeader->header.block_size = (currentBlockSize + nextBlockSize) >> 4;

	addNode(currentAllocatedBlockHeader);
}

void coalesceLeft(sf_free_header * currentAllocatedBlockHeader){
	sf_free_header * previousFreeBlockHeader = getPreviousBlockHeader(currentAllocatedBlockHeader);
	sf_footer * currentAllocatedBlockFooter = getFooter(currentAllocatedBlockHeader);

	sf_footer* oldFooter = ((void*)currentAllocatedBlockHeader)+((currentAllocatedBlockHeader->header.block_size<<4) - 8);//Changing the old footer
	oldFooter->alloc = 0;


	size_t previousBlockSize = getSize(previousFreeBlockHeader);
	size_t currentBlockSize = getSize(currentAllocatedBlockHeader);

	removeNode(previousFreeBlockHeader);

	previousFreeBlockHeader->header.alloc = 0;
	previousFreeBlockHeader->header.block_size = (currentBlockSize + previousBlockSize) >> 4;

	currentAllocatedBlockFooter->alloc = 0;
	currentAllocatedBlockFooter->block_size = (currentBlockSize + previousBlockSize) >> 4;

	addNode(previousFreeBlockHeader);
}

void addNode(sf_free_header * ptr){
	ptr->prev = NULL;
	ptr->next = NULL;
	if(freelist_head == NULL){
		freelist_head = ptr;
	}
	else{
		sf_free_header * nextToHead = freelist_head;
		freelist_head = ptr;
		nextToHead->prev = ptr;
		ptr->next = nextToHead;
	}

}
void removeNode(sf_free_header * ptr){
	if(ptr->next == NULL && ptr->prev == NULL){//Head with no elements
		freelist_head = NULL;//Deleting the whole list 
	}
	else if(ptr->next != NULL && ptr->prev == NULL){//Head of the list
		sf_free_header * nextToHead = ptr->next;//Making the next the head of the list
		nextToHead->prev = NULL;
		freelist_head->next = NULL;
		freelist_head = nextToHead;
	}
	else if(ptr->next == NULL && ptr->prev != NULL){//TAIL
		sf_free_header * prevToTail = ptr->prev;
		prevToTail->next = NULL;
		ptr->prev = NULL;
	}
	else if(ptr->next != NULL && ptr->prev != NULL){// MIDDLE OF THE LIST
		sf_free_header * nextToNode = ptr->next;
		sf_free_header * prevToNode = ptr->prev;
		nextToNode->prev = prevToNode;
		prevToNode->next = nextToNode;
	}
}
void * getNextBlockHeader(void* ptr){
	sf_header * blockToReturn = ptr;
	uint64_t sizeOfBlock = blockToReturn->block_size << 4;

	//This has been moved to the beginning of next Block
	blockToReturn = ((void*)blockToReturn) + sizeOfBlock;
	return blockToReturn;
}

void * getPreviousBlockHeader(void* ptr){
	ptr -= 8;//moving it to the foot of previous block
	sf_header * blockToReturn = ptr;
	uint64_t sizeOfBlock = blockToReturn->block_size << 4;

	//This has been moved to the beginning of next Block
	blockToReturn = ((void*)blockToReturn) - (sizeOfBlock-8);
	return blockToReturn;
}


int previousBlockAllocationChecker(void * ptr){
	//ptr is pointing at the head of the block
	ptr = ptr - 8;
	sf_footer * blockToCheck = ptr;
	if((void*)blockToCheck < (void*)sbrkStart){
		return 1;
	}
	else if(blockToCheck->alloc == 0){
		return 0;

	}
	else if(blockToCheck->alloc == 1){
		return 1;
	}
	return 1;
}

int nextBlockAllocationChecker(void *ptr){
	sf_footer *blockToCheck = getFooter(ptr);
	blockToCheck = ((void*)blockToCheck) + 8;
	if((void*)blockToCheck >= (void*)sbrkEnd){
		return 1;
	}
	else if(blockToCheck->alloc == 0){
		return 0;

	}
	else if(blockToCheck->alloc == 1){
		return 1;
	}
	return 1;
}


sf_footer * getFooter(void *ptr){
	sf_footer * temp = ptr;
	return ptr + ((temp->block_size << 4)-8);
}

uint64_t getSize(void *ptr){
	sf_footer * temp = ptr;
	return temp->block_size << 4;
}



void iterator(sf_free_header *temp){
	while(temp!=NULL){
		sf_blockprint(temp);
		temp = temp->next;
	}
}

