#include "nanoCLR_native.h"

#include <nanoCLR_Runtime.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace
{
void WriteTrace(const char *text, size_t length)
{
#if NANOCLR_WASM_ENABLE_TRACE
    if (text != nullptr && length != 0)
    {
        fwrite(text, 1, length, stdout);
        fflush(stdout);
    }
#else
    (void)text;
    (void)length;
#endif
}
} // namespace

void CLR_Debug::Emit(const char *text, int length)
{
    if (text == nullptr)
    {
        return;
    }

    if (g_DebugPrintCallback != nullptr)
    {
        g_DebugPrintCallback(text);
        return;
    }

    WriteTrace(text, length < 0 ? strlen(text) : static_cast<size_t>(length));
}

int CLR_Debug::PrintfV(const char *format, va_list arguments)
{
    char buffer[384];
    const int length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    if (length > 0)
    {
        CLR_Debug::Emit(buffer, length < static_cast<int>(sizeof(buffer)) ? length : sizeof(buffer) - 1);
    }
    return length;
}

int CLR_Debug::Printf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int length = PrintfV(format, arguments);
    va_end(arguments);
    return length;
}

void HAL_Windows_Debug_Print(const char *text)
{
    CLR_Debug::Emit(text, -1);
}
