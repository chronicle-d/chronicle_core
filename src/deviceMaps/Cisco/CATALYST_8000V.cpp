/*
    Vendor: Cisco
    Device: CATALYST_8000V
*/
#include "core/device_factory.hpp"
#include "core/error_handler.hpp"

extern "C" deviceOperations* createDeviceOperations(int device_id, int vendor_id) {
    auto* devOps = new deviceOperations();

    if (vendor_id != CISCO_ID) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_VENDOR_ID, "Vendor ID does not match Cisco. (" + std::to_string(vendor_id) + " instead of " + std::to_string(CISCO_ID) + ")");
        return nullptr;
    };
    
    if (device_id != CATALYST_8000V) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_DEVICE_ID, "Device ID does not match CATALYST_8000V. (" + std::to_string(device_id) + " instead of " + std::to_string(CATALYST_8000V) + ")");
        return nullptr;
    };

    // getConfig
    devOps->pushCommand(devOps->getConfig, "terminal length 0", 1, 1, "Failed to set terminal length to 0");
    devOps->pushCommand(devOps->getConfig, "show running all", 4, 1, "Failed to get configuration");

    return devOps;
}

extern "C" const char* getDeviceDetails() {
    return "Vendor: Cisco\nDevice: CATALYST_8000V";
}
