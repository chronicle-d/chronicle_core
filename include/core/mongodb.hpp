#ifndef CHRONICLE_MONGODB_HPP
#define CHRONICLE_MONGODB_HPP
/*
 * MongoDB driver wrapper for easier use inside chronicle
 *
 * Database name:   chronicle_db
 * Databse user:    chronicle_user
 * Database passwd: w8K4<2Y~snE}n_+l}+
 * Collections:
 *  - devices  | All devices and their information
 *  - users    | All chroniucle users
 *  - settings | Chronicle settings
*/

#include <string>
#include <vector>
#include <optional>
 
#include <mongocxx/collection.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/types.hpp>
 

class MongoDB {
  private:
    static void ensureInstance();
    void initDatabase();
    inline static const std::string databaseName_ = "chronicle_db";
    inline static const std::string databaseUser_ = "chronicle_user";
    inline static const std::string databaseUserPassword_ = "w8K4<2Y~snE}n_+l}+";
    mongocxx::client client_;
    mongocxx::database db_;
  public:
    MongoDB();
    void insertDocument(mongocxx::collection& collection, const bsoncxx::document::view_or_value& doc);
    void updateDocument(mongocxx::collection& collection, bsoncxx::builder::basic::document& queryFilter, const bsoncxx::document::view_or_value& data);
    void deleteDocument(mongocxx::collection& collection, bsoncxx::builder::basic::document& queryFilter);
    void connect();
    std::vector<bsoncxx::document::value> findDocuments(
      mongocxx::collection& collection,
      const bsoncxx::document::view_or_value& filter,
      const bsoncxx::document::view_or_value& projection,
      std::optional<int> limit = std::nullopt
    );

    mongocxx::collection devices_c;
    mongocxx::collection users_c;
    mongocxx::collection settings_c;
    bool connected = false;
};
#endif // CHRONICLE_MONGODB_HPP
