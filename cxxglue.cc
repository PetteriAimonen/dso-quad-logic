/* Thin wrappers for using C++ without newlib or other heavy libraries. */

extern "C"
{
#include "ds203_io.h"

void __cxa_pure_virtual() {
    // Pure virtual function was called
    crash_with_message("C++: pure virtual", __builtin_return_address(0));
}

}

namespace std {
    void __throw_out_of_range(const char*) {
        // Vector .at() with argument out of range
        crash_with_message("C++: out of range", __builtin_return_address(0));
    }
    
    void __throw_length_error(const char*) {
        // Vector resize to excessive length
        crash_with_message("C++: length error", __builtin_return_address(0));
    }
    
    void __throw_bad_alloc() {
        crash_with_message("C++: bad alloc", __builtin_return_address(0));
    }
}

void abort(void)
{
    crash_with_message("abort", __builtin_return_address(0));
    while(1);
}

#include <stdlib.h>

void * operator new(size_t n)
{
  void * const p = malloc(n);
  if (p == 0)
      crash_with_message("out of memory", __builtin_return_address(0));
  
  return p;
}

void operator delete(void * p)
{
  free(p);
}

void * operator new[](size_t n)
{
    return ::operator new(n);
}

void operator delete[](void * p)
{
    free(p);
}
