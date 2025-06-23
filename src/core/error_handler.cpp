#include "core/error_handler.hpp"
#include <sstream>
#include <iostream>

std::string getErrorMsg(int internal_exit_code) {
    switch (internal_exit_code) {
        // Internal errors
        case CHRONICLE_ERROR_UNKNOWN_CORE_ERROR: return "Unknown core level error";
        case CHRONICLE_ERROR_ASSERTION_FAILED: return "Assertion failed, core level error";

        // SSH errors
        case CHRONICLE_ERROR_SSH_UNKNOWN: return "Unknown ssh error";
        case CHRONICLE_ERROR_SSH_CLOSED_REMOTE: return "Connection closed by remote";
        case CHRONICLE_ERROR_SSH_SESSION_FAILED: return "Failed creating an SSH session";
        case CHRONICLE_ERROR_SSH_CONNECTION_FAILED: return "Could not connect to host";
        case CHRONICLE_ERROR_SSH_COMMAND_FAILED: return "Command failed";

        // Device factory
        case CHRONICLE_ERROR_DEVICE_FACTORY_FAILED: return "Error while getting device operations";
        case CHRONICLE_ERROR_INVALID_DEVICE_ID: return "Wrong device ID provided";
        case CHRONICLE_ERROR_INVALID_VENDOR_ID: return "Wrong vendor ID provided";
        case CHRONICLE_ERROR_LOAD_DEVICE_MAP_FAILED: return "Error while loading device map";

        // MongoDB
        case CHRONICLE_ERROR_MONGO_UNKNOWN: return "Unknown error while using MongoDB";
        case CHRONICLE_ERROR_MONGO_CONNECT_TO_DB: return "Cannot connect to database";
        case CHRONICLE_ERROR_MONGO_INSERT_FAILED: return "Failed to insert a document into a collection";
        case CHRONICLE_ERROR_MONGO_UPDATE_FAILED: return "Failed to update a document in a collection";
        case CHRONICLE_ERROR_MONGO_DUPLICATE: return "Duplicate document";
        case CHRONICLE_ERROR_MONGO_DOCUMENT_NOT_FOUND: return "Collection not found";
        case CHRONICLE_ERROR_MONGO_DELETE_DOCUMENT: return "Could not delete document";

        // ChronicleDB
        case CHRONICLE_ERROR_CHRONICLE_DB_ADD_FAILED: return "Failed while trying to add";
        case CHRONICLE_ERROR_CHRONICLE_DB_MODIFY_FAILED: return "Failed while trying to modify";
        case CHRONICLE_ERROR_CHRONICLE_DB_DELETE_FAILED: return "Failed while trying to delete";
        case CHRONICLE_ERROR_CHRONICLE_DB_NX_DOCUMENT: return "Faild while trying to access a non-exitant document";

        default: return "Unknown error.";
    }
}

// Accessors
int ChronicleException::getCode() const {
    return code_;
}

std::string ChronicleException::getFunction() const {
    return function_name_;
}

std::string ChronicleException::getDetails() const {
    return details_;
}

std::string ChronicleException::getFile() const {
    return file_;
}

int ChronicleException::getLine() const {
    return line_;
}

const char* ChronicleException::what() const noexcept {
    return full_message_.c_str();
}


ChronicleException::ChronicleException(int code, const std::string& message, const std::string& function, const std::string& details, const std::string& file, int line)
    : std::runtime_error(message), code_(code), function_name_(function), details_(details), file_(file), line_(line)
{
    std::ostringstream oss;
    oss << "[ChronicleError: " << code_ << "]";

    if (!file_.empty())
        oss << "[" << file_ << ":" << line_ << "]";

    if (!function_name_.empty())
        oss << "[" << function_name_ << "]";

    oss << ": ";

    oss << message;
    if (!details_.empty())
        oss << " => " << details_;

    full_message_ = oss.str();
}

void throwChronicleException(int internal_exit_code, const std::string& details, const std::string& function, int line, const std::string& filename) {
    throw ChronicleException(internal_exit_code, getErrorMsg(internal_exit_code), function, details, filename, line);
}

void chronicleAssert(bool statement, int internal_exit_code, const std::string& function, const std::string& details) {
    if (!statement) {
        throwChronicleException(internal_exit_code, details, function);
    }
}
