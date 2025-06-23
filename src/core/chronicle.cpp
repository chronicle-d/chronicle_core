#include "core/chronicle.hpp"
#include "core/ssh.hpp"

std::vector<std::string> getConfig(connectionInfo ci, std::vector<OperationMap> getConfig) {
    Ssh ssh;
    ssh_session session = ssh.startSession(ci);
    ssh_channel channel = ssh.startChannel(session);
    std::vector<std::string> output;

    ssh.flushBanner(session, channel);
    for (auto cmdMap : getConfig) {
        output.clear();
        output = ssh.executeCommand(cmdMap, session, channel);
    }

    ssh.closeChannel(channel);
    ssh.endSession(session);

    return output;
}
