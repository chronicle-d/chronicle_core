#include "core/config.hpp"
#include "database_handler.hpp"
#include "core/error_handler.hpp"
#include <unordered_map>
#include "core/device_factory.hpp"

#include <bsoncxx/builder/basic/document.hpp>

int connectionInfo::getVendorId(const std::string& vendor_name) const {
  if (vendor_name == "Cisco") {
    return CISCO_ID;
  } else if (vendor_name == "Juniper") {
    return JUNIPER_ID;
  } else {
    return 0;
  }
}

int connectionInfo::getDeviceId(int vendor_id, const std::string& device_name) const {
    chronicleAssert(vendor_id > 0, 10000, "connectionInfo::getDeviceId", "No vendor ID found for " + device_name);

    using DeviceMap = std::unordered_map<std::string, int>;

    static const std::unordered_map<int, DeviceMap> vendorDeviceMap = {
        { CISCO_ID, {
            {"ENCS_5100",      ENCS_5100},
            {"C1700",          C1700},
            {"C7200",          C7200},
            {"CATALYST_8000V", CATALYST_8000V},
            {"CSR1000V",       CSR1000V},
            {"LINUX_TEST",     LINUX_TEST}
        }},
        { JUNIPER_ID, {
            {"VSRX",           VSRX}
        }}
    };

    const auto vendorIt = vendorDeviceMap.find(vendor_id);
    if (vendorIt == vendorDeviceMap.end())
        return 0;

    const auto& deviceMap = vendorIt->second;
    const auto deviceIt = deviceMap.find(device_name);

    return (deviceIt != deviceMap.end()) ? deviceIt->second : 0;
}

connectionInfo getConnectionInfo(const std::string& deviceNickname) {
  connectionInfo ci;

  /* Fetch device settings */
  ChronicleDB cdb;

  const auto& deviceSettings = cdb.getDeviceBson(deviceNickname);
  const auto& ssh = deviceSettings["ssh"].get_document().view();
  const auto& device = deviceSettings["device"].get_document().view();

  /* Initialize */
  std::string vendorName = std::string(device["vendorName"].get_string().value);
  std::string deviceName = std::string(device["deviceName"].get_string().value);

  ci.vendorName         = vendorName;
  ci.deviceName         = deviceName;
  ci.vendor             = ci.getVendorId(vendorName);
  ci.device             = ci.getDeviceId(ci.vendor, deviceName);
  ci.user               = std::string(ssh["user"].get_string().value);
  ci.password           = std::string(ssh["password"].get_string().value);
  ci.host               = std::string(ssh["host"].get_string().value);
  ci.port               = ssh["port"].get_int32();
  ci.verbosity          = ssh["verbosity"].get_int32();
  ci.kex_methods        = std::string(ssh["kexMethods"].get_string().value);
  ci.hostkey_algorithms = std::string(ssh["hostkeyAlgorithms"].get_string().value);

  return ci;
}

chronicleSettings getChronicleSettings() {
  chronicleSettings cs;

  /* Fetch chronicle settings */
  ChronicleDB cdb;

  const auto& settings = cdb.getSettingsBson();
  const auto& ssh = settings["ssh"].get_document().view();

  cs.ssh_idle_timeout   = ssh["sshIdleTimeout"].get_int32();
  cs.ssh_total_timeout  = ssh["sshTotalTimeout"].get_int32();

  return cs;
}
