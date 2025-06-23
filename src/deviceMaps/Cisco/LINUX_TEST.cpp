/*
    Vendor: Cisco
    Device: LINUX_TEST
*/
#include "core/device_factory.hpp"
#include "core/error_handler.hpp"

extern "C" deviceOperations* createDeviceOperations(int device_id, int vendor_id) {
    auto* devOps = new deviceOperations();

    if (vendor_id != CISCO_ID) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_VENDOR_ID, "Vendor ID does not match Cisco. (" + std::to_string(vendor_id) + " instead of " + std::to_string(CISCO_ID) + ")");
        return nullptr;
    };
    
    if (device_id != LINUX_TEST) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_DEVICE_ID, "Device ID does not match LINUX_TEST. (" + std::to_string(device_id) + " instead of " + std::to_string(LINUX_TEST) + ")");
        return nullptr;
    };

    // getConfig
    devOps->pushCommand(devOps->getConfig, "cat ~/.bashrc", 0, 1, "Failed to open .bashrc");

    return devOps;
}

extern "C" const char* getDeviceDetails() {
    return "Vendor: Cisco\nDevice: LINUX_TEST";
}
