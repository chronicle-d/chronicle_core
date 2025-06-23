#ifndef CHRONICLE_DATABASE_HANDLER_HPP
#define CHRONICLE_DATABASE_HANDLER_HPP

#include "core/config.hpp"
#include <optional>
#include <vector>

#include <bsoncxx/document/view_or_value.hpp>

class ChronicleDB {
  public:
 
    void connect();

    struct MongoProjections {
      static const bsoncxx::document::view_or_value device();
      static const bsoncxx::document::view_or_value settings();
      static const bsoncxx::document::view_or_value users();
    };

    /* Global */
    void initDB() const;

    /* Chronicle settings */
    void updateSettings(
      std::optional<int> sshIdleTimeout = CHRONICLE_CONFIG_DEFAULT_SSH_IDLE_TIMEOUT,
      std::optional<int> sshTotalTimeout = CHRONICLE_CONFIG_DEFAULT_SSH_TOTAL_TIMEOUT
    ) const;
    std::string getSettings() const;

    /* Devices */
    void addDevice(
      // General
      const std::string& deviceNickname,
      const std::string& deviceName,
      const std::string& vendor,

      // SSH
      const std::string& user,
      const std::string& password,
      const std::string& host,
      const int& port = CHRONICLE_CONFIG_DEFAULT_PORT,
      const int& sshVerbosity = 0,
      const std::string& kexMethods = CHRONICLE_CONFIG_DEFAULT_KEX_METHODS,
      const std::string& hostkeyAlgorithms = CHRONICLE_CONFIG_DEFAULT_HOSTKEYS
    ) const;

    void modifyDevice(
      const std::string& deviceNickname,

      // Device
      std::optional<std::string> deviceName = std::nullopt,
      std::optional<std::string> vendor = std::nullopt,

      // SSH fields
      std::optional<std::string> user = std::nullopt,
      std::optional<std::string> password = std::nullopt,
      std::optional<std::string> host = std::nullopt,
      std::optional<int> port = std::nullopt,
      std::optional<int> sshVerbosity = std::nullopt,
      std::optional<std::string> kexMethods = std::nullopt,
      std::optional<std::string> hostkeyAlgorithms = std::nullopt
    ) const;

    void deleteDevice(const std::string& deviceNickname) const;
    std::vector<std::string> listDevices() const;
    std::string getDevice(const std::string& deviceNickname) const;

    // Users
    void addUser(const std::string& username, const std::string& password, bool connected) const;
    void modifyUser(
      const std::string& username,
      std::optional<std::string> password,
      std::optional<bool> connected
    ) const;
    void deleteUser(const std::string& username) const;
    std::vector<std::string> listUsers() const;
    std::string getUser(const std::string& username) const;


    // C++ Internal methods
    bsoncxx::document::value getDeviceBson(const std::string& deviceNickname) const;
    bsoncxx::document::value getSettingsBson() const;
    
};
#endif // CHRONICLE_DATABASE_HANDLER_HPP
