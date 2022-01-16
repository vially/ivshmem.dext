#include <os/log.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IODMACommand.h>
#include <PCIDriverKit/PCIDriverKit.h>

#include "IVSHMEMDriver.h"
#include "IVSHMEMDriverUserClient.h"

// This log to makes it easier to parse out individual logs from the driver, since all logs will be prefixed with the same word/phrase.
// DriverKit logging has no logging levels; some developers might want to prefix errors differently than info messages.
// Another option is to #define a "debug" case where some log messages only exist when doing a debug build.
// To search for logs from this driver, use either: `sudo dmesg | grep IVSHMEMDriverUserClient` or use Console.app search to find messages that start with "IVSHMEMDriverUserClient -".
#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "IVSHMEMDriverUserClient - " fmt "\n", ##__VA_ARGS__)

/// - Tag: Struct_IVSHMEMDriverUserClient_IVars
struct IVSHMEMDriverUserClient_IVars {
    IVSHMEMDriver* ivshmem = nullptr;
};

bool IVSHMEMDriverUserClient::init()
{
    if (!super::init())
    {
        return false;
    }

    ivars = IONewZero(IVSHMEMDriverUserClient_IVars, 1);
    if (!ivars)
    {
        return false;
    }

    return true;
}

kern_return_t
IMPL(IVSHMEMDriverUserClient, Start)
{
    kern_return_t ret;

    ret = Start(provider, SUPERDISPATCH);
    if(ret != kIOReturnSuccess)
    {
        Log("fail: Start");
        return kIOReturnNoDevice;
    }

    ivars->ivshmem = OSDynamicCast(IVSHMEMDriver, provider);
    if(nullptr == ivars->ivshmem)
    {
        Log("fail: OSDynamicCast");
        return kIOReturnNoDevice;
    }

    return kIOReturnSuccess;
}

kern_return_t
IMPL(IVSHMEMDriverUserClient, Stop)
{
    Stop(provider, SUPERDISPATCH);
    return kIOReturnSuccess;
}

void
IVSHMEMDriverUserClient::free()
{
    IODelete(ivars, IVSHMEMDriverUserClient_IVars, 1);
    super::free();
}

kern_return_t
IMPL(IVSHMEMDriverUserClient, CopyClientMemoryForType) //(uint64_t type, uint64_t *options, IOMemoryDescriptor **memory)
{
    *memory = ivars->ivshmem->copyBAR2Memory();

    return kIOReturnSuccess;
}

