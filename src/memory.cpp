#include <cstddef>
#include <cstdlib>

namespace
{
alignas(std::max_align_t) unsigned char s_managedHeap[NANOCLR_WASM_HEAP_SIZE];
} // namespace

void HeapLocation(unsigned char *&baseAddress, unsigned int &sizeInBytes)
{
    baseAddress = s_managedHeap;
    sizeInBytes = sizeof(s_managedHeap);
}

void CustomHeapLocation(unsigned char *&baseAddress, unsigned int &sizeInBytes)
{
    baseAddress = nullptr;
    sizeInBytes = 0;
}

extern "C" void *platform_malloc(size_t size)
{
    return std::malloc(size);
}

extern "C" void platform_free(void *address)
{
    std::free(address);
}
