#ifndef TARGET_OS_H
#define TARGET_OS_H

#define CONFIG_RTOS WASI
#define TARGET_HAS_NANOBOOTER 0

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0U
#endif
#ifndef VERSION_MINOR
#define VERSION_MINOR 1U
#endif
#ifndef VERSION_BUILD
#define VERSION_BUILD 0U
#endif
#ifndef VERSION_REVISION
#define VERSION_REVISION 0U
#endif

#define TARGETINFOSTRING "WASM-WASI reactor"

#include "nf_config.h"

#endif
