#ifndef CHRONICLE_SSH_H
#define CHRONICLE_SSH_H
#include "core/config.hpp"
#include <vector>
#include <string>
#include <regex>
#include <libssh/libssh.h>
#include "core/device_factory.hpp"

#define SSH_FLUSH_BANNER(session, channel) \
    Sss::executeCommand("", session, channel);
/*

    # ssh.hpp
    This file is a spesfic wrapper for ssh inteded to serve only chronicles needs, it is not a full on wrapper.

*/

class Ssh {
    public:
        ssh_session startSession(connectionInfo ci) const;
        void endSession(ssh_session session) const;
        std::vector<std::string> executeCommand(OperationMap opartion_map, ssh_session session, ssh_channel channel) const;
        ssh_channel startChannel(ssh_session session) const;
        void closeChannel(ssh_channel channel) const;
        void flushBanner(ssh_session session, ssh_channel channel) const;
    private:
        static std::string verifyKnownHost(ssh_session session);
        /* Should be taken from here: https://www.cisco.com/c/en/us/support/switches/catalyst-9300-series-switches/products-system-message-guides-list.html */
        static bool hasError(const std::string& line) {
            static const std::vector<std::regex> common_error_patterns = {
                std::regex(R"(^%\w+-3-\w+:.*)", std::regex::icase),
                std::regex(R"(Invalid input detected)", std::regex::icase),
                std::regex(R"(Incomplete command)", std::regex::icase),
                std::regex(R"(Ambiguous command)", std::regex::icase),
                std::regex(R"(Unrecognized command)", std::regex::icase),
                std::regex(R"(Unknown command)", std::regex::icase),
                std::regex(R"(Command rejected)", std::regex::icase),
                std::regex(R"(^%?Error.*)", std::regex::icase)
            };
        
            for (const auto& re : common_error_patterns) {
                if (std::regex_search(line, re)) return true;
            }
        
            return false;
        }        
};

#endif // CHRONICLE_SSH_H
