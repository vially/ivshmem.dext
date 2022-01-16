#include <os/log.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IODMACommand.h>
#include <PCIDriverKit/PCIDriverKit.h>

#include "IVSHMEMDriver.h"

// This log to makes it easier to parse out individual logs from the driver, since all logs will be prefixed with the same word/phrase.
// DriverKit logging has no logging levels; some developers might want to prefix errors differently than info messages.
// Another option is to #define a "debug" case where some log messages only exist when doing a debug build.
// To search for logs from this driver, use either: `sudo dmesg | grep IVSHMEMDriver` or use Console.app search to find messages that start with "IVSHMEMDriver -".
#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "IVSHMEMDriver - " fmt "\n", ##__VA_ARGS__)

/// - Tag: Struct_IVSHMEMDriver_IVars
struct IVSHMEMDriver_IVars {
    IOPCIDevice* pciDevice = nullptr;
    BARInfo bar2Info;
};

bool IVSHMEMDriver::init()
{
    if (!super::init())
    {
        return false;
    }

    ivars = IONewZero(IVSHMEMDriver_IVars, 1);
    if (!ivars)
    {
        return false;
    }

    return true;
}

kern_return_t
IMPL(IVSHMEMDriver, Start)
{
    uint16_t commandRegister;
    kern_return_t ret;

    ret = Start(provider, SUPERDISPATCH);
    if(ret != kIOReturnSuccess)
    {
        Log("fail: Start");
        return kIOReturnNoDevice;
    }

    ivars->pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if(nullptr == ivars->pciDevice)
    {
        Log("fail: OSDynamicCast");
        return kIOReturnNoDevice;
    }

    ret = ivars->pciDevice->Open(this, 0);
    if(kIOReturnSuccess != ret)
    {
        Log("fail: Open");
        return kIOReturnError;
    }

    // Enabling Bus Master and Memory Space
    ivars->pciDevice->ConfigurationRead16(kIOPCIConfigurationOffsetCommand, &commandRegister);
    commandRegister |= (kIOPCICommandBusMaster | kIOPCICommandMemorySpace);
    ivars->pciDevice->ConfigurationWrite16(kIOPCIConfigurationOffsetCommand, commandRegister);

    uint16_t deviceId, vendorId;
    ivars->pciDevice->ConfigurationRead16(kIOPCIConfigurationOffsetDeviceID, &deviceId);
    ivars->pciDevice->ConfigurationRead16(kIOPCIConfigurationOffsetVendorID, &vendorId);
    Log("ivshmem: vendor ID: %hx device ID: %hx", vendorId, deviceId);

    uint32_t bar1Address, bar2Address;
    ivars->pciDevice->ConfigurationRead32(kIOPCIConfigurationOffsetBaseAddress1, &bar1Address);
    ivars->pciDevice->ConfigurationRead32(kIOPCIConfigurationOffsetBaseAddress2, &bar2Address);
    if (bar1Address != 0)
    {
        Log("warn: ivshmem device is configured for interrupts which are not implemented in the driver");
    }
    Log("ivshmem: bar2 address: %x", bar2Address);

    ret = ivars->pciDevice->GetBARInfo(kPCIMemoryRangeBAR2, &ivars->bar2Info.memoryIndex, &ivars->bar2Info.size, &ivars->bar2Info.barType);
    if(kIOReturnSuccess != ret)
    {
        Log("fail: Unable to read BAR 2 information");
        return ret;
    }
    Log("ivshmem: bar2 info: %lld mb %d %d", ivars->bar2Info.size/(1024*1024), ivars->bar2Info.memoryIndex, ivars->bar2Info.barType);

    if (kIOReturnSuccess != RegisterService())
    {
        Log("fail: RegisterService");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

kern_return_t
IMPL(IVSHMEMDriver, Stop)
{
    uint16_t commandRegister;

    // Disable Bus Master and Memory Space
    ivars->pciDevice->ConfigurationRead16(kIOPCIConfigurationOffsetCommand, &commandRegister);
    commandRegister &= ~(kIOPCICommandBusMaster | kIOPCICommandMemorySpace);
    ivars->pciDevice->ConfigurationWrite16(kIOPCIConfigurationOffsetCommand, commandRegister);

    ivars->pciDevice->Close(this, 0);

    Stop(provider, SUPERDISPATCH);

    return kIOReturnSuccess;
}

void
IVSHMEMDriver::free()
{
    IODelete(ivars, IVSHMEMDriver_IVars, 1);
    super::free();
}

// When an application attaches to the dext via IOServiceOpen, this method is called
kern_return_t IMPL(IVSHMEMDriver, NewUserClient)
{
    kern_return_t ret = kIOReturnSuccess;
    IOService* client = nullptr;

    ret = Create(this, "UserClientProperties", &client);
    if (ret != kIOReturnSuccess)
    {
        Log("NewUserClient() - Failed to create UserClientProperties with error: 0x%08x.", ret);
        return ret;
    }

    *userClient = OSDynamicCast(IOUserClient, client);
    if (*userClient == NULL)
    {
        Log("NewUserClient() - Failed to cast new client.");
        client->release();
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOMemoryDescriptor* IVSHMEMDriver::copyBAR2Memory()
{
    kern_return_t ret = kIOReturnSuccess;
    IOMemoryDescriptor* memory;

    ret = ivars->pciDevice->_CopyDeviceMemoryWithIndex(ivars->bar2Info.memoryIndex, &memory, this);
    if(kIOReturnSuccess != ret)
    {
        Log("fail: Unable to copy device memory: 0x%08x", ret);
    }

    return memory;
}

