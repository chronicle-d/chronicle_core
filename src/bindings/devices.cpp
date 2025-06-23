#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include "core/device_factory.hpp"
#include "core/error_handler.hpp"
#include <memory>
#include <dlfcn.h>

namespace py = pybind11;

struct DeviceHandle {
    void* handle;
    deviceOperations* ops;

    DeviceHandle(void* h, deviceOperations* o) : handle(h), ops(o) {}
    ~DeviceHandle() {
        if (ops) delete ops;
        if (handle) dlclose(handle);
    }
};

std::shared_ptr<DeviceHandle> loadDeviceOps(const std::string& so_path, int device_id, int vendor_id) {
    void* handle = dlopen(so_path.c_str(), RTLD_NOW);
    if (!handle) THROW_CHRONICLE_EXCEPTION(303, "dlopen failed");

    using CreateFunc = deviceOperations* (*)(int, int);
    auto create_fn = (CreateFunc)dlsym(handle, "createDeviceOperations");
    if (!create_fn) THROW_CHRONICLE_EXCEPTION(303, "dlsym failed");

    auto* ops = create_fn(device_id, vendor_id);
    if (!ops) THROW_CHRONICLE_EXCEPTION(303, "Device did not return valid operations");

    return std::make_shared<DeviceHandle>(handle, ops);
}

void bind_device_loader(py::module_& m) {
    py::class_<OperationMap>(m, "OperationMap")
        .def_readwrite("command", &OperationMap::command)
        .def_readwrite("skip_head", &OperationMap::skip_head)
        .def_readwrite("skip_tail", &OperationMap::skip_tail)
        .def_readwrite("err_msg", &OperationMap::err_msg);

    py::class_<deviceOperations>(m, "deviceOperations")
        .def_readwrite("getConfig", &deviceOperations::getConfig);

    py::class_<DeviceHandle, std::shared_ptr<DeviceHandle>>(m, "DeviceHandle")
        .def_readonly("ops", &DeviceHandle::ops);

    m.def("loadDeviceOps", &loadDeviceOps, "Load a device plugin from a .cld file");
}
