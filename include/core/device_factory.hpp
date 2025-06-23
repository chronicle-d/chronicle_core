#ifndef CHRONICLE_DEVICE_FACTORY_H
#define CHRONICLE_DEVICE_FACTORY_H
#include <string>
#include <vector>

/* ---------- Vendor & Device IDs ---------- */

// Cisco vendor and device IDs
inline constexpr int CISCO_ID          = 1;
inline constexpr int ENCS_5100         = 1;
inline constexpr int C1700             = 2;
inline constexpr int C7200             = 3;
inline constexpr int CATALYST_8000V    = 4;
inline constexpr int CSR1000V          = 5;
inline constexpr int LINUX_TEST        = 80;

// Juniper vendor and device IDs
inline constexpr int JUNIPER_ID        = 2;
inline constexpr int VSRX              = 1;

/* ---------- Data Structures ---------- */

struct OperationMap {
  std::string command;
  int skip_head;
  int skip_tail;
  std::string err_msg;
};

struct deviceOperations {
  std::vector<OperationMap> getConfig;

  void pushCommand(std::vector<OperationMap>& operation, const std::string& command,
                   int skip_head, int skip_tail, const std::string& err_msg);
};

/* ---------- Required Plugin Exports ---------- */

extern "C" deviceOperations *createDeviceOperations(int device_id,
                                                    int vendor_id);
extern "C" const char *getDeviceDetails();

#endif // CHRONICLE_DEVICE_FACTORY_H
