#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

extern Area heap;
static bool first_malloc = true;
static void *addr = NULL;
void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  //panic("Not implemented");
  if(first_malloc)
    {
      addr = heap.start;
      first_malloc = false;
    }
  if(size == 0) return NULL; //size == 0, let malloc return NULL pointer
  void *alloced_mem = addr; //else malloc the memory
  addr = (void *)ROUNDUP((uintptr_t)addr + (uintptr_t)size, 8);
  return alloced_mem;
#endif
  return NULL;
}

void free(void *ptr) {
  //Current we needn't implement it
}

#endif