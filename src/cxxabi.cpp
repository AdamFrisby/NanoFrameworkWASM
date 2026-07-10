#include <cstddef>

extern "C"
{

[[noreturn]] void *__cxa_allocate_exception(std::size_t size)
{
    (void)size;
    __builtin_trap();
}

[[noreturn]] void __cxa_throw(void *exception, void *typeInfo, void (*destructor)(void *))
{
    (void)exception;
    (void)typeInfo;
    (void)destructor;
    __builtin_trap();
}

} // extern "C"
