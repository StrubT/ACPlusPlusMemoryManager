
#ifndef MemMngr_H
#define MemMngr_H

////////////////////
// MEMORY MANAGER //
////////////////////

enum MemAllocType { Single, Array };

struct MemAlloc {

	int number;
	MemAllocType type;
	size_t size;
	bool freed;
	char const* file;
	int line;

	void const* pointer;
};

struct MemAllocs {

	int size, length;

	MemAlloc* allocs;
};

class MemMngr {

	static bool isInstanciated;

	MemAllocs allocInfo;

	void enlargeAllocations() noexcept;

public:
	MemAlloc const* addPointer(MemAllocType type, size_t size, char const* file, int line, void const* pointer) noexcept;
	MemAlloc const* removePointer(void const* pointer) noexcept;

	MemAllocs const& getMemoryAllocations() const noexcept { return allocInfo; }
	MemAllocs fetchMemoryLeaks() const noexcept;

	void init() noexcept;
	void atExit() noexcept;
};

///////////////
// OPERATORS //
///////////////

void* operator new(size_t size, char const* file, int line) throw(std::bad_alloc);
void* operator new[](size_t size, char const* file, int line) throw(std::bad_alloc);

void operator delete(void* pointer, char const* file, int line) throw(); //needed to prevent warnings
void operator delete[](void* pointer, char const* file, int line) throw();

#endif

#if !defined(NOT_NEW_PUBLISH) && !defined(new)

#define new new(__FILE__, __LINE__)

#endif
