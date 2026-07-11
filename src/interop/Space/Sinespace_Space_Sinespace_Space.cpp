//
// Sinespace world API — native method implementations. The managed stub assembly (Sinespace.Space, Space.pe)
// declares these as [MethodImpl(InternalCall)]; nanoCLR dispatches here via the generated marshaling. This first
// method proves the managed -> native -> host path end to end; production methods will call the host through a
// dedicated import namespace with object-id handles (see docs/architecture.md, Capability Model).
//
#include "Sinespace_Space.h"
#include "Sinespace_Space_Sinespace_Space.h"

#include <cstdio>

using namespace Sinespace_Space::Sinespace_Space;

void Space::HostReport(signed int param0, HRESULT &hr)
{
    hr = S_OK;
    // WASI fd_write -> the host observes it. The host boundary the real Space.* API will use for callbacks.
    std::fprintf(stdout, "HOST_REPORT %d\n", param0);
    std::fflush(stdout);
}
