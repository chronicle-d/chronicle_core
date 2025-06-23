/*
    Vendor: Juniper
    Device: VSRX
*/
#include "core/device_factory.hpp"
#include "core/error_handler.hpp"

extern "C" deviceOperations* createDeviceOperations(int device_id, int vendor_id) {
    auto* devOps = new deviceOperations();

    if (vendor_id != JUNIPER_ID) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_DEVICE_ID, "Vendor ID does not match Juniper. (" + std::to_string(vendor_id) + " instead of " + std::to_string(JUNIPER_ID) + ")");
        return nullptr;
    };
    
    if (device_id != VSRX) {
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_INVALID_VENDOR_ID, "Device ID does not match CATALYST_8000V. (" + std::to_string(device_id) + " instead of " + std::to_string(VSRX) + ")");
        return nullptr;
    };

    // getConfig
    devOps->pushCommand(devOps->getConfig, "show configuration | display set | no-more", 1, 2, "Failed to get configuration");

    return devOps;
}

extern "C" const char* getDeviceDetails() {
    return "Vendor: Juniper\nDevice: VSRX";
}
