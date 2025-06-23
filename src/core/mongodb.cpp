#include "core/mongodb.hpp"
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/options/find.hpp>
#include <bsoncxx/json.hpp>
#include "core/error_handler.hpp"


namespace {
  mongocxx::instance& getInstance() {
    static mongocxx::instance instance;
    return instance;
  }
}

void MongoDB::ensureInstance() {
  (void)getInstance();
}

void MongoDB::initDatabase() {
  db_.create_collection("users");

  db_.create_collection("devices");
  auto index_keys = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("device.name", 1));
  mongocxx::options::index index_options{};
  index_options.unique(true);
  db_["devices"].create_index(index_keys.view(), index_options);

  db_.create_collection("settings");
}

MongoDB::MongoDB() {ensureInstance();}

void MongoDB::connect() {
  client_ = mongocxx::client{mongocxx::uri{
    "mongodb://" +
    databaseUser_ + ":" + databaseUserPassword_ +
    "@localhost:27017/" + databaseName_ + "?authSource=admin"
  }};

  db_ = client_[databaseName_];
  users_c = db_["users"];
  devices_c = db_["devices"];
  settings_c = db_["settings"];

  try {
    initDatabase();
    connected = true;
  } catch (const mongocxx::exception& e) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_CONNECT_TO_DB, e.what());
  }
}

void MongoDB::insertDocument(mongocxx::collection& collection, const bsoncxx::document::view_or_value& doc) {
	try {
		auto result = collection.insert_one(doc.view());
		if (!result) {
			THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_INSERT_FAILED, "Insert failed with no result.");
		}
  } catch (const mongocxx::exception& e) {
      const int code = e.code().value();
      const std::string errorStr = e.what();
      std::string message;
      switch (code) {
          case 11000: THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_DUPLICATE, errorStr); break;
          case 121: message = "Document validation failed.\nError: " + errorStr; break;
          case 50: message = "Operation timed out.\nError: " + errorStr; break;
          case 13: message = "Unauthorized access.\nError: " + errorStr; break;
      }

      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_INSERT_FAILED, message);
  } catch (const std::exception& e) {
      std::string fullMessage =
          "Generic insert failure.\n"
          "Collection: " + std::string(collection.name()) + "\n"
          "Document: " + bsoncxx::to_json(doc) + "\n"
          "Error: " + e.what();
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_INSERT_FAILED, fullMessage);
  }
}

void MongoDB::updateDocument(mongocxx::collection& collection, bsoncxx::builder::basic::document& queryFilter, const bsoncxx::document::view_or_value& doc) {
  bsoncxx::builder::basic::document updateDoc;
  updateDoc.append(bsoncxx::builder::basic::kvp("$set", doc));

	try {
		auto result = collection.update_one(queryFilter.view(), updateDoc.view());
		if (!result) {
      std::string fullMessage =
        "No results from query filter.\n"
			  "Collection: " + std::string(collection.name()) + "\n"
			  "Query filter: " + bsoncxx::to_json(queryFilter);
			THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_UPDATE_FAILED, fullMessage);
		}
    if (result->matched_count() == 0) {
        std::string fullMessage =
            "No matching document found to update.\n"
            "Collection: " + std::string(collection.name()) + "\n"
            "Query filter: " + bsoncxx::to_json(queryFilter);
        THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND, fullMessage);
    }
  } catch (const ChronicleException& e) {
    // Re-throw unchanged
    throw;
	} catch (const std::exception& e) {
		std::string fullMessage =
			"MongoDB update failed.\n"
			"Collection: " + std::string(collection.name()) + "\n"
      "Query filter: " + bsoncxx::to_json(queryFilter) + "\n"
			"Update document: " + bsoncxx::to_json(updateDoc) + "\n"
			"Error: " + e.what();
		THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_UPDATE_FAILED, fullMessage);
	}
}

void MongoDB::deleteDocument(mongocxx::collection& collection,  bsoncxx::builder::basic::document& queryFilter) {
  try {
    auto result = collection.delete_one(queryFilter.view());

    if (!result) {
      std::string fullMessage =
        "No result returned from delete_one.\n"
        "Collection: " + std::string(collection.name()) + "\n"
        "Query filter: " + bsoncxx::to_json(queryFilter);
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_DELETE_DOCUMENT, fullMessage);
    }

    if (result->deleted_count() == 0) {
      std::string fullMessage =
        "No matching document found to delete.\n"
        "Collection: " + std::string(collection.name()) + "\n"
        "Query filter: " + bsoncxx::to_json(queryFilter);
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND, fullMessage);
    }

  } catch (const ChronicleException& e) {
    throw;
  } catch (const std::exception& e) {
    std::string fullMessage =
      "MongoDB delete failed.\n"
      "Collection: " + std::string(collection.name()) + "\n"
      "Query filter: " + bsoncxx::to_json(queryFilter) + "\n"
      "Error: " + e.what();
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_MONGO_DELETE_DOCUMENT, fullMessage);
  }
}

std::vector<bsoncxx::document::value> MongoDB::findDocuments(
  mongocxx::collection& collection,
  const bsoncxx::document::view_or_value& filter,
  const bsoncxx::document::view_or_value& projection,
  std::optional<int> limit
) {
  mongocxx::options::find opts;

  if (limit.has_value()) {
      opts.limit(*limit);
  }

  opts.projection(projection);

  std::vector<bsoncxx::document::value> results;

  auto cursor = collection.find(filter.view(), opts);
  for (auto&& doc : cursor) {
      results.emplace_back(bsoncxx::document::value(doc));
  }

  return results;
}
