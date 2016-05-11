
#if defined(DEBUG) || defined(_DEBUG) || defined(__debug) || !defined(NDEBUG)

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <new>

using namespace std;

#define POINTERS_DEFAULT_SIZE 128
#define POINTERS_ENLARGE_FACTOR 2

#define ALLOC_NUMBER_LENGTH 5
#define ALLOC_SIZE_LENGTH 5

////////////////////
// MEMORY MANAGER //
////////////////////

#define NOT_NEW_PUBLISH
#include "MemMngr_strut1.h"
#undef NOT_NEW_PUBLISH

template <class T>
T* mallocMemMngr(int length = 1) {

	T* allocations = (T*)malloc(sizeof(T) * length);

	if (!allocations) {
		cerr << "Not enough memory space to run memory manager!" << endl;
		exit(EXIT_FAILURE);
	}

	return allocations;
}

MemMngr* memMngr = nullptr;

bool MemMngr::isInstanciated = false;

void MemMngr::enlargeAllocations() noexcept {

	this->allocInfo.size *= POINTERS_ENLARGE_FACTOR;

	MemAlloc* allocations = mallocMemMngr<MemAlloc>(this->allocInfo.size);
	memcpy(allocations, this->allocInfo.allocs, sizeof(MemAlloc) * this->allocInfo.length);
	free(this->allocInfo.allocs);
	this->allocInfo.allocs = allocations;
}

MemAlloc const* MemMngr::addPointer(MemAllocType type, size_t size, char const* file, int line, void const* pointer) noexcept {

	while (this->allocInfo.size <= this->allocInfo.length)
		enlargeAllocations();

	MemAlloc* alloc = this->allocInfo.allocs + this->allocInfo.length++;
	alloc->number = this->allocInfo.length;
	alloc->type = type;
	alloc->size = size;
	alloc->freed = false;
	alloc->file = file;
	alloc->line = line;
	alloc->pointer = pointer;

	return alloc;
}

MemAlloc const* MemMngr::removePointer(void const* pointer) noexcept {

	for (int i = 0; i < this->allocInfo.length; i++)
		if (this->allocInfo.allocs[i].pointer == pointer) {
			this->allocInfo.allocs[i].freed = true;
			return this->allocInfo.allocs + i;
		}

	return nullptr;
}

MemAllocs MemMngr::fetchMemoryLeaks() const noexcept {

	int count = 0;
	for (int i = 0; i < this->allocInfo.length; i++)
		if (!this->allocInfo.allocs[i].freed)
			count++;

	MemAllocs leaks;
	leaks.length = leaks.size = count;
	leaks.allocs = mallocMemMngr<MemAlloc>(leaks.size);
	if (!leaks.allocs) {
		cerr << "Not enough memory space to run memory manager!" << endl;
		exit(EXIT_FAILURE);
	}

	for (int i = 0, j = 0; i < this->allocInfo.length && j < leaks.length; i++)
		if (!this->allocInfo.allocs[i].freed)
			leaks.allocs[j++] = this->allocInfo.allocs[i];

	return leaks;
}

void MemMngr::init() noexcept {

	cout << "MemMngr::init()" << endl;

	if (MemMngr::isInstanciated) {
		cerr << "Cannot instanciate more than one memory manager!" << endl;
		exit(EXIT_FAILURE);
	}

	MemMngr::isInstanciated = true;

	this->allocInfo.length = 0;
	this->allocInfo.size = POINTERS_DEFAULT_SIZE;
	this->allocInfo.allocs = mallocMemMngr<MemAlloc>(this->allocInfo.size);

	atexit([]() { memMngr->atExit(); });
}

void MemMngr::atExit() noexcept {

	cout << "--------------------------------------------------------" << endl;
	cout << "MemMngr::atExit()";

	MemAllocs const& allocs = this->getMemoryAllocations();
	size_t sizeTotal = 0;
	for (int i = 0; i < allocs.length; i++)
		sizeTotal += allocs.allocs[i].size;

	MemAllocs leaks = this->fetchMemoryLeaks();
	size_t sizeOfLeaks = 0;
	for (int i = 0; i < leaks.length; i++)
		sizeOfLeaks += leaks.allocs[i].size;

	if (!leaks.length)
		cout << ": No leaks found." << endl;

	else {
		cout << endl << endl;
		cout << "No. of memory leaks: " << leaks.length << endl;
		cout << "Total size of leaks: " << sizeOfLeaks << " bytes" << endl;
		cout << "Max. memory used:    " << sizeTotal << " bytes" << endl;

		if (leaks.length > 0)
			cout << endl;
		for (int i = 0; i < leaks.length; i++) {
			cout << "#" << setw(ALLOC_NUMBER_LENGTH) << left << leaks.allocs[i].number << ": 0x" << leaks.allocs[i].pointer << ", size: " << setw(ALLOC_SIZE_LENGTH) << right << leaks.allocs[i].size << " bytes";
			if (leaks.allocs[i].line >= 0)
				cout << " -- " << leaks.allocs[i].file << ":" << leaks.allocs[i].line;
			cout << endl;
		}
	}

	free(leaks.allocs);
	free(this->allocInfo.allocs);

	cout << "--------------------------------------------------------" << endl;
}

///////////////
// OPERATORS //
///////////////

void initMemMngr() {

	if (!memMngr) {
		memMngr = mallocMemMngr<MemMngr>();
		memMngr->init();
	}
}

void* operator new(size_t size, char const* file, int line) throw(std::bad_alloc) {

	void* pointer = malloc(size);
	if (!pointer)
		throw std::bad_alloc();

	initMemMngr();
	MemAlloc const* alloc = memMngr->addPointer(MemAllocType::Single, size, file, line, pointer);

	int sizeSpaces = ALLOC_SIZE_LENGTH - (int)log10(size);
	cout << "operator new   (" << alloc->size << "):";
	for (int i = 0; i < sizeSpaces; i++) cout << " ";
	cout << " 0x" << alloc->pointer << ":";
	cout << " #" << alloc->number << endl;

	return pointer;
}

void* operator new[](size_t size, char const* file, int line) throw(std::bad_alloc) {

	void* pointer = malloc(size);
	if (!pointer)
		throw std::bad_alloc();

	initMemMngr();
	MemAlloc const* alloc = memMngr->addPointer(MemAllocType::Array, size, file, line, pointer);

	int sizeSpaces = ALLOC_SIZE_LENGTH - (int)log10(size);
	cout << "operator new[] (" << alloc->size << "):";
	for (int i = 0; i < sizeSpaces; i++) cout << " ";
	cout << " 0x" << alloc->pointer << ":";
	cout << " #" << alloc->number << endl;

	return pointer;
}

void* operator new(size_t size) throw(std::bad_alloc) {

	return operator new(size, "unknown", -1);
}

void* operator new[](size_t size) throw(std::bad_alloc) {

	return operator new[](size, "unknown", -1);
}

void operator delete(void* pointer) throw() {

	free(pointer);

	MemAlloc const* alloc = memMngr->removePointer(pointer);
	cout << "operator delete:         0x" << alloc->pointer << ": #" << setw(ALLOC_NUMBER_LENGTH) << left << alloc->number;
	if (alloc->type != MemAllocType::Single) //useless, application will abort
		cout << " -- type of allocation and deallocation differ!";
	cout << endl;
}

void operator delete[](void* pointer) throw() {

	free(pointer);

	MemAlloc const* alloc = memMngr->removePointer(pointer);
	cout << "operator delete[]:       0x" << alloc->pointer << ": #" << setw(ALLOC_NUMBER_LENGTH) << left << alloc->number;
	if (alloc->type != MemAllocType::Array) //useless, application will abort
		cout << " -- type of allocation and deallocation differ!";
	cout << endl;
}

void operator delete(void* pointer, char const* file, int line) throw() {

	operator delete(pointer);
}

void operator delete[](void* pointer, char const* file, int line) throw() {

	operator delete(pointer);
}

#endif
