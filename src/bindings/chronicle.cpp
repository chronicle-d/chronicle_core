#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/chronicle.hpp"
#include "core/config.hpp"
#include "core/error_handler.hpp"
#include "core/mongodb.hpp"
#include "database_handler.hpp"

namespace py = pybind11;

extern void bind_device_loader(py::module_ &);

PYBIND11_MODULE(chronicle, m) {
  m.doc() = "Chronicle";

  // Chronicle Exception
  static py::exception<ChronicleException> chronicle_exc(m,
                                                         "ChronicleException");

  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p)
        std::rethrow_exception(p);
    } catch (const ChronicleException &e) {
      PyObject *exc_inst =
          PyObject_CallFunction(chronicle_exc.ptr(), "s", e.what());
      if (exc_inst) {
        py::object obj = py::reinterpret_steal<py::object>(exc_inst);
        obj.attr("code") = e.getCode();
        obj.attr("function") = e.getFunction();
        obj.attr("details") = e.getDetails();
        obj.attr("file") = e.getFile();
        obj.attr("line") = e.getLine();
        PyErr_SetObject(chronicle_exc.ptr(), obj.ptr());
      } else {
        PyErr_SetString(PyExc_RuntimeError,
                        "Failed to construct ChronicleException");
      }
    }
  });

  m.def("getErrorMsg", &getErrorMsg,
        "Returns the message of a given error code.");

  m.def("getConnectionInfo", &getConnectionInfo,
        "Get connectionInfo for a given config section.");
  m.def("getChronicleSettings", &getChronicleSettings,
        "Get chronicleSettings from config.");

  // Structs
  py::class_<connectionInfo>(m, "connectionInfo")
      .def(py::init<>())
      .def_readwrite("vendor", &connectionInfo::vendor)
      .def_readwrite("device", &connectionInfo::device)
      .def_readwrite("deviceName", &connectionInfo::deviceName)
      .def_readwrite("vendorName", &connectionInfo::vendorName)
      .def_readwrite("user", &connectionInfo::user)
      .def_readwrite("password", &connectionInfo::password)
      .def_readwrite("host", &connectionInfo::host)
      .def_readwrite("port", &connectionInfo::port)
      .def_readwrite("kex_methods", &connectionInfo::kex_methods)
      .def_readwrite("hostkey_algorithms", &connectionInfo::hostkey_algorithms)
      .def_readwrite("verbosity", &connectionInfo::verbosity)
      .def("getVendorId", &connectionInfo::getVendorId)
      .def("getDeviceId", &connectionInfo::getDeviceId);

  py::class_<chronicleSettings>(m, "chronicleSettings")
      .def_readwrite("ssh_idle_timeout", &chronicleSettings::ssh_idle_timeout)
      .def_readwrite("ssh_total_timeout",
                     &chronicleSettings::ssh_total_timeout);

  m.def("getConfig", &getConfig, py::arg("ConnectionInfo"),
        py::arg("OperationMap"), "Returns the current device configuration.");

  // ChronicleDB
  // - Devices
  py::class_<ChronicleDB>(m, "ChronicleDB")
      .def("connect", &ChronicleDB::connect, "Connects to chronicle db.")
      .def(py::init<>())
      .def("addDevice", &ChronicleDB::addDevice, py::arg("deviceNickname"),
           py::arg("deviceName"), py::arg("vendor"), py::arg("user"),
           py::arg("password"), py::arg("host"),
           py::arg("port") = CHRONICLE_CONFIG_DEFAULT_PORT,
           py::arg("sshVerbosity") = 0,
           py::arg("kexMethods") = CHRONICLE_CONFIG_DEFAULT_KEX_METHODS,
           py::arg("hostkeyAlgorithms") = CHRONICLE_CONFIG_DEFAULT_HOSTKEYS,
           "Add a new device to the Chronicle database.")
      .def("modifyDevice", &ChronicleDB::modifyDevice,
           py::arg("deviceNickname"), py::arg("deviceName"), py::arg("vendor"),
           py::arg("user"), py::arg("password"), py::arg("host"),
           py::arg("port"), py::arg("sshVerbosity"), py::arg("kexMethods"),
           py::arg("hostkeyAlgorithms"),
           "Modify a device in the Chronicle database.")
      .def("deleteDevice", &ChronicleDB::deleteDevice,
           py::arg("deviceNickname"),
           "Delete a device in the Chronicle database.")
      .def("getDevice", &ChronicleDB::getDevice, py::arg("deviceNickname"),
           "Get a device in the Chronicle database.")
      .def("listDevices", &ChronicleDB::listDevices,
           "Lists all devices in the Chronicle database.")

      // Settings
      .def("getSettings", &ChronicleDB::getSettings,
           "Returns the current chronicle settings.")
      .def("updateSettings", &ChronicleDB::updateSettings,
           py::arg("sshIdleTimeout"), py::arg("sshTotalTimeout"),
           "Updates the current chronicle settings.")
      .def("updateSettings", &ChronicleDB::updateSettings,
           py::arg("sshIdleTimeout"), py::arg("sshTotalTimeout"),
           "Updates the current chronicle settings.")

      // Users
      .def("addUser", &ChronicleDB::addUser, py::arg("username"),
           py::arg("password"), py::arg("connect"),
           "Adds a new user to the Chronicle database.")
      .def("modifyUser", &ChronicleDB::modifyUser, py::arg("username"),
           py::arg("password"), py::arg("connect"),
           "Modify a new user to the Chronicle database.")
      .def("deleteUser", &ChronicleDB::deleteUser, py::arg("username"),
           "Delete a user from the Chronicle database.")
      .def("getUser", &ChronicleDB::getUser, py::arg("username"),
           "Get a user from the Chronicle database.")
      .def("listUsers", &ChronicleDB::listUsers,
           "List all users from the Chronicle database.")

      .def("initDB", &ChronicleDB::initDB, "Initiates the chronicle db.");

  bind_device_loader(m);
}
