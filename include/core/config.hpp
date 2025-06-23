#ifndef CHRONICLE_CONFIG_H
#define CHRONICLE_CONFIG_H

#include <string>

inline constexpr auto CHRONICLE_CONFIG_DEFAULT_KEX_METHODS =
    "curve25519-sha256@libssh.org,ecdh-sha2-nistp384,ecdh-sha2-nistp256,diffie-hellman-group14-sha256";
inline constexpr auto CHRONICLE_CONFIG_DEFAULT_HOSTKEYS =
    "ssh-ed25519,ecdsa-sha2-nistp521,ecdsa-sha2-nistp384,ecdsa-sha2-nistp256,rsa-sha2-512,rsa-sha2-256";
inline constexpr int CHRONICLE_CONFIG_DEFAULT_PORT = 22;
inline constexpr const char* CHRONICLE_CONFIG_DEFAULT_USER = "chronicle-runner";
inline constexpr int CHRONICLE_CONFIG_DEFAULT_VERBOSITY = 0;
inline constexpr int CHRONICLE_CONFIG_DEFAULT_SSH_IDLE_TIMEOUT = 1000;
inline constexpr int CHRONICLE_CONFIG_DEFAULT_SSH_TOTAL_TIMEOUT = 10000;

struct connectionInfo {

    int getVendorId(const std::string& vendor_name) const;
    int getDeviceId(int vendor_id, const std::string& device_name) const;

    std::string kex_methods = CHRONICLE_CONFIG_DEFAULT_KEX_METHODS;
    std::string hostkey_algorithms = CHRONICLE_CONFIG_DEFAULT_HOSTKEYS;
    std::string vendorName;
    int vendor;
    std::string deviceName;
    int device;
    std::string user = CHRONICLE_CONFIG_DEFAULT_USER;
    std::string password;
    std::string host;
    int port = CHRONICLE_CONFIG_DEFAULT_PORT;
    int verbosity = CHRONICLE_CONFIG_DEFAULT_VERBOSITY;
};

struct chronicleSettings {
    int ssh_idle_timeout = CHRONICLE_CONFIG_DEFAULT_SSH_IDLE_TIMEOUT;
    int ssh_total_timeout = CHRONICLE_CONFIG_DEFAULT_SSH_TOTAL_TIMEOUT;
};


connectionInfo getConnectionInfo(const std::string& deviceNickname);
chronicleSettings getChronicleSettings();
#endif // CHRONICLE_CONFIG_H
