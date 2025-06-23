#include "database_handler.hpp"
#include "core/mongodb.hpp"
#include "core/error_handler.hpp"
#include <bsoncxx/json.hpp>

MongoDB mdb;

void ChronicleDB::connect() {
  mdb.connect();
  if (!mdb.connected) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Failed connecting to DB.");
  }
}

void ChronicleDB::initDB() const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Cannot initialise DB since connection to database was not established."); }

  bsoncxx::builder::basic::document matchAllFilter{};

  // Settings
  auto settingsExisting = mdb.findDocuments(
    mdb.settings_c,
    matchAllFilter.view(),
    ChronicleDB::MongoProjections::settings(),
    1
  );
  
  if (settingsExisting.empty()) {
    bsoncxx::builder::basic::document settingsDoc;

    settingsDoc.append(
      bsoncxx::builder::basic::kvp(
        "ssh",
        bsoncxx::builder::basic::make_document(
          bsoncxx::builder::basic::kvp("sshIdleTimeout", CHRONICLE_CONFIG_DEFAULT_SSH_IDLE_TIMEOUT),
          bsoncxx::builder::basic::kvp("sshTotalTimeout", CHRONICLE_CONFIG_DEFAULT_SSH_TOTAL_TIMEOUT)
        )
      )
    );

    try {
      mdb.insertDocument(mdb.settings_c, settingsDoc.view());
    } catch (const ChronicleException& e) {

      std::string fullMessage;

      fullMessage =
        "Chronicle exception:\n"
        "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
        "Details: " + e.getDetails();
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
    } catch (const std::exception& e) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, e.what());
    } 
  }
}

const bsoncxx::document::view_or_value ChronicleDB::MongoProjections::device() {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Cannot create mongo projection since connection to database was not established."); }

  return bsoncxx::builder::basic::make_document(
    bsoncxx::builder::basic::kvp("_id", 0),
    bsoncxx::builder::basic::kvp("ssh", bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("host", 1),
      bsoncxx::builder::basic::kvp("port", 1),
      bsoncxx::builder::basic::kvp("user", 1),
      bsoncxx::builder::basic::kvp("password", 1),
      bsoncxx::builder::basic::kvp("verbosity", 1),
      bsoncxx::builder::basic::kvp("kexMethods", 1),
      bsoncxx::builder::basic::kvp("hostkeyAlgorithms", 1)
    )),
    bsoncxx::builder::basic::kvp("device", bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("name", 1),
      bsoncxx::builder::basic::kvp("deviceName", 1),
      bsoncxx::builder::basic::kvp("vendorName", 1)
    ))
  );
}

const bsoncxx::document::view_or_value ChronicleDB::MongoProjections::settings() {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Cannot create mongo projection since connection to database was not established."); }

  return bsoncxx::builder::basic::make_document(
    bsoncxx::builder::basic::kvp("_id", 0),
    bsoncxx::builder::basic::kvp("ssh", bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("sshIdleTimeout", 1),
      bsoncxx::builder::basic::kvp("sshTotalTimeout", 1)
    ))
  );
}

const bsoncxx::document::view_or_value ChronicleDB::MongoProjections::users() {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Cannot create mongo projection since connection to database was not established."); }

  return bsoncxx::builder::basic::make_document(
    bsoncxx::builder::basic::kvp("_id", 0),
    bsoncxx::builder::basic::kvp("username", 1),
    bsoncxx::builder::basic::kvp("password", 1),
    bsoncxx::builder::basic::kvp("connected", 1)
  );
}


/* Settings */
void ChronicleDB::updateSettings(std::optional<int> sshIdleTimeout, std::optional<int> sshTotalTimeout) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document filter{};
  bsoncxx::builder::basic::document updateDoc;
  
  // SSH fields
  if (sshIdleTimeout)           updateDoc.append(bsoncxx::builder::basic::kvp("ssh.sshIdleTimeout", *sshIdleTimeout));
  if (sshTotalTimeout)          updateDoc.append(bsoncxx::builder::basic::kvp("ssh.sshTotalTimeout", *sshTotalTimeout));

  try {
    mdb.updateDocument(mdb.settings_c, filter, updateDoc.view());
  } catch (const ChronicleException& e) {

    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND) {
      fullMessage = "No settings found for chronicle, cannot modify.";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_NX_DOCUMENT, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, e.what());
  } 
}

std::string ChronicleDB::getSettings() const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::document::view_or_value filter = bsoncxx::builder::basic::make_document(); // List all
  auto results = mdb.findDocuments(mdb.settings_c, filter, ChronicleDB::MongoProjections::settings());

  if (results.empty()) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_NX_DOCUMENT, "No Chronicle settings found in database.");
  }

  return bsoncxx::to_json(results[0].view());
}

/* Devices */
void ChronicleDB::addDevice(
  // General
  const std::string& deviceNickname,
  const std::string& deviceName,
  const std::string& vendor,

  // SSH
  const std::string& user,
  const std::string& password,
  const std::string& host,
  const int& port,
  const int& sshVerbosity,
  const std::string& kexMethods,
  const std::string& hostkeyAlgorithms
) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document deviceData;

  deviceData.append(
    bsoncxx::builder::basic::kvp("device", bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("name", deviceNickname),
      bsoncxx::builder::basic::kvp("deviceName", deviceName),
      bsoncxx::builder::basic::kvp("vendorName", vendor)
      
    ))
  );

  deviceData.append(
    bsoncxx::builder::basic::kvp("ssh", bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("user", user),
      bsoncxx::builder::basic::kvp("password", password),
      bsoncxx::builder::basic::kvp("host", host),
      bsoncxx::builder::basic::kvp("port", port),
      bsoncxx::builder::basic::kvp("verbosity", sshVerbosity),
      bsoncxx::builder::basic::kvp("kexMethods", kexMethods),
      bsoncxx::builder::basic::kvp("hostkeyAlgorithms", hostkeyAlgorithms)
    ))
  );

  try {
    mdb.insertDocument(mdb.devices_c, deviceData.view());
  } catch (const ChronicleException& e) {

    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DUPLICATE) {
      fullMessage = "Device named '" + deviceNickname + "' exists already.";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, e.what());
  }
}

void ChronicleDB::modifyDevice(
  const std::string& deviceNickname,

  // Device
  std::optional<std::string> deviceName,
  std::optional<std::string> vendor,

  // SSH fields
  std::optional<std::string> user,
  std::optional<std::string> password,
  std::optional<std::string> host,
  std::optional<int> port,
  std::optional<int> sshVerbosity,
  std::optional<std::string> kexMethods,
  std::optional<std::string> hostkeyAlgorithms
) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }
  
  bsoncxx::builder::basic::document updateDoc;
  
  // Device fields
  if (deviceName)         updateDoc.append(bsoncxx::builder::basic::kvp("device.deviceName", *deviceName));
  if (vendor)             updateDoc.append(bsoncxx::builder::basic::kvp("device.vendorName", *vendor));

  // SSH fields
  if (user)               updateDoc.append(bsoncxx::builder::basic::kvp("ssh.user", *user));
  if (password)           updateDoc.append(bsoncxx::builder::basic::kvp("ssh.password", *password));
  if (host)               updateDoc.append(bsoncxx::builder::basic::kvp("ssh.host", *host));
  if (port)               updateDoc.append(bsoncxx::builder::basic::kvp("ssh.port", *port));
  if (sshVerbosity)       updateDoc.append(bsoncxx::builder::basic::kvp("ssh.verbosity", *sshVerbosity));
  if (kexMethods)         updateDoc.append(bsoncxx::builder::basic::kvp("ssh.kexMethods", *kexMethods));
  if (hostkeyAlgorithms)  updateDoc.append(bsoncxx::builder::basic::kvp("ssh.hostkeyAlgorithms", *hostkeyAlgorithms));

  if (updateDoc.view().empty()) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, "No fields provided to modify.");
  }

  bsoncxx::builder::basic::document queryFilter;
  queryFilter.append(bsoncxx::builder::basic::kvp("device.name", deviceNickname));

  try {
    mdb.updateDocument(mdb.devices_c, queryFilter, updateDoc.view());
  } catch (const ChronicleException& e) {

    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND) {
      fullMessage = "Device " + deviceNickname + " not found";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, e.what());
  }
}

void ChronicleDB::deleteDevice(const std::string& deviceNickname) const {

  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document queryFilter;
  queryFilter.append(bsoncxx::builder::basic::kvp("device.name", deviceNickname));

  try {
    mdb.deleteDocument(mdb.devices_c, queryFilter);
  } catch (const ChronicleException& e) {
    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND) {
      fullMessage = "Device " + deviceNickname + " not found";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, e.what());
  }
}

std::vector<std::string> ChronicleDB::listDevices() const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::document::view_or_value filter = bsoncxx::builder::basic::make_document(); // List all
  auto results = mdb.findDocuments(mdb.devices_c, filter, ChronicleDB::MongoProjections::device());

  std::vector<std::string> listOfDevices;

  for (const auto r : results) {
    listOfDevices.push_back( bsoncxx::to_json(r));
  }

  return listOfDevices;
}

std::string ChronicleDB::getDevice(const std::string& deviceNickname) const {

  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document filter;

  filter.append(bsoncxx::builder::basic::kvp("device.name", deviceNickname));

  auto results = mdb.findDocuments(mdb.devices_c, filter.view(), ChronicleDB::MongoProjections::device());

  std::string deviceData;

  if (!results.empty()) {
      deviceData = bsoncxx::to_json(results[0].view());
  }

  return deviceData;
}

/* Users */
void ChronicleDB::addUser(const std::string& username, const std::string& password, bool connected) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document userData;

  userData.append(
    bsoncxx::builder::basic::kvp("username", username),
    bsoncxx::builder::basic::kvp("password", password),
    bsoncxx::builder::basic::kvp("connected", connected)
  );

  try {
    mdb.insertDocument(mdb.users_c, userData.view());
  } catch (const ChronicleException& e) {

    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DUPLICATE) {
      fullMessage = "User named '" + username + "' exists already.";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED, e.what());
  }
}

void ChronicleDB::modifyUser(
  const std::string& username,
  std::optional<std::string> password,
  std::optional<bool> connected
) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document updateDoc;

  if (password)             updateDoc.append(bsoncxx::builder::basic::kvp("password", *password));
  if (connected)            updateDoc.append(bsoncxx::builder::basic::kvp("connected", *connected));

  if (updateDoc.view().empty()) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, "No fields provided to modify.");
  }

  bsoncxx::builder::basic::document queryFilter;
  queryFilter.append(bsoncxx::builder::basic::kvp("username", username));

  try {
    mdb.updateDocument(mdb.users_c, queryFilter, updateDoc.view());
  } catch (const ChronicleException& e) {

    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND) {
      fullMessage = "User " + username + " not found";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED, e.what());
  }
}

void ChronicleDB::deleteUser(const std::string& username) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document queryFilter;
  queryFilter.append(bsoncxx::builder::basic::kvp("username", username));

  try {
    mdb.deleteDocument(mdb.users_c, queryFilter);
  } catch (const ChronicleException& e) {
    std::string fullMessage;

    if (e.getCode() == CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND) {
      fullMessage = "User " + username + " not found";
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, fullMessage);
    }

    fullMessage =
      "Chronicle exception:\n"
      "ChronicleCode: " + std::to_string(e.getCode()) + "\n"
      "Details: " + e.getDetails();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, fullMessage);
  } catch (const std::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED, e.what());
  }
}

std::vector<std::string> ChronicleDB::listUsers() const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::document::view_or_value filter = bsoncxx::builder::basic::make_document(); // List all
  auto results = mdb.findDocuments(mdb.users_c, filter, ChronicleDB::MongoProjections::users());

  std::vector<std::string> listOfUsers;

  for (const auto r : results) {
    listOfUsers.push_back( bsoncxx::to_json(r));
  }

  return listOfUsers;
}

std::string ChronicleDB::getUser(const std::string& username) const {
  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document filter;

  filter.append(bsoncxx::builder::basic::kvp("username", username));

  auto results = mdb.findDocuments(mdb.users_c, filter.view(), ChronicleDB::MongoProjections::users());

  std::string userData;

  if (!results.empty()) {
      userData = bsoncxx::to_json(results[0].view());
  }

  return userData;
}



// Internal C++ methods
bsoncxx::document::value ChronicleDB::getDeviceBson(const std::string& deviceNickname) const {

  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::builder::basic::document filter;

  filter.append(bsoncxx::builder::basic::kvp("device.name", deviceNickname));

  auto results = mdb.findDocuments(mdb.devices_c, filter.view(), ChronicleDB::MongoProjections::device());

  if (results.empty()) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_NX_DOCUMENT, "Device not found.");
  }

  return bsoncxx::document::value(results[0]);
 }

bsoncxx::document::value ChronicleDB::getSettingsBson() const {

  if (!mdb.connected) { THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, "Connection to database was not established."); }

  bsoncxx::document::view_or_value filter = bsoncxx::builder::basic::make_document(); // List all
  auto results = mdb.findDocuments(mdb.settings_c, filter, ChronicleDB::MongoProjections::settings());

  if (results.empty()) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_CHRONICLE_DB_NX_DOCUMENT, "No Chronicle settings found in database.");
  }

  return bsoncxx::document::value(results[0]);
}
