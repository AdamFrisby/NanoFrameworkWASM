#ifndef NANOPAL_H
#define NANOPAL_H

#include <cmath>
#include <cstring>

#include <nanoHAL.h>
#include <nanoPAL_AsyncProcCalls_decl.h>
#include <nanoPAL_BlockStorage.h>
#include <nanoPAL_COM.h>
#include <nanoPAL_Events.h>
#include <nanoPAL_FileSystem.h>
#include <nanoPAL_Time.h>

#define hal_strlen_s(value) strlen(value)

inline int hal_strcpy_s(char *destination, size_t destinationSize, const char *source)
{
    if (destination == nullptr || source == nullptr || destinationSize == 0)
    {
        return 1;
    }

    const size_t sourceLength = strlen(source);
    const size_t copyLength = sourceLength < destinationSize ? sourceLength : destinationSize - 1;
    memcpy(destination, source, copyLength);
    destination[copyLength] = '\0';
    return sourceLength < destinationSize ? 0 : 1;
}

inline int hal_strncpy_s(char *destination, size_t destinationSize, const char *source, size_t count)
{
    if (destination == nullptr || source == nullptr || destinationSize == 0)
    {
        return 1;
    }

    const size_t copyLength = count < destinationSize ? count : destinationSize - 1;
    memcpy(destination, source, copyLength);
    destination[copyLength] = '\0';
    return count < destinationSize ? 0 : 1;
}

#ifndef __isnanf
#define __isnanf(value) std::isnan(static_cast<float>(value))
#endif

void HeapLocation(unsigned char *&baseAddress, unsigned int &sizeInBytes);

#endif
