#include "core/ssh.hpp"

#include "core/error_handler.hpp"

#include <sstream>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <thread>

ssh_session Ssh::startSession(connectionInfo ci) const {
  ssh_session session;

  session = ssh_new();
  if (session == NULL) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED,ssh_get_error(session));
  }

  ssh_options_set(session, SSH_OPTIONS_HOST, ci.host.c_str());
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &ci.verbosity);
  ssh_options_set(session, SSH_OPTIONS_PORT, &ci.port);
  ssh_options_set(session, SSH_OPTIONS_KEY_EXCHANGE, ci.kex_methods.c_str());
  ssh_options_set(session, SSH_OPTIONS_HOSTKEYS, ci.hostkey_algorithms.c_str());

  int rc = ssh_connect(session);
  if (rc != SSH_OK)
  {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_CONNECTION_FAILED,ssh_get_error(session));
  }

  std::string known_host_msg = verifyKnownHost(session);
  if (known_host_msg.size() > 0) {
      endSession(session);
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED,known_host_msg);
  }

  rc = ssh_userauth_password(session, ci.user.c_str(), ci.password.c_str());
  if (rc != SSH_AUTH_SUCCESS) {
      endSession(session);
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, "Password for user \"" + ci.user + "\" is wrong");
  }

  return session;
}

ssh_channel Ssh::startChannel(ssh_session session) const {
  ssh_channel channel;

  if (!ssh_is_connected(session)) {
    endSession(session);
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_UNKNOWN, "SSH session died, could not create channel.");
  }
  
  channel = ssh_channel_new(session);
  if (channel == NULL)
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, ssh_get_error(session));

  if (ssh_channel_open_session(channel) != SSH_OK) {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, ssh_get_error(session));
  }

  if (ssh_channel_request_pty(channel) != SSH_OK) {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, ssh_get_error(session));
  }

  if (ssh_channel_request_shell(channel) != SSH_OK) {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED,ssh_get_error(session));
  }

  return channel;
}

void Ssh::endSession(ssh_session session) const {
  ssh_disconnect(session);
  ssh_free(session);
}

void Ssh::closeChannel(ssh_channel channel) const {
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
}

std::vector<std::string> Ssh::executeCommand(OperationMap operation_map, ssh_session session, ssh_channel channel) const {
  int rc;
  char buffer[4096];
  int nbytes;
  std::string output, error_output;
  std::vector<std::string> output_lines;

  chronicleSettings cs = getChronicleSettings();

  std::string full_command = std::string(operation_map.command) + "\n";
  if (ssh_channel_write(channel, full_command.c_str(), full_command.size()) == SSH_ERROR) {
    closeChannel(channel);
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED,ssh_get_error(session));
  }

  auto start = std::chrono::steady_clock::now();
  auto last_data = start;

  while (true) {
    // Read stdout (stream 0)
    rc = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);
    if (rc == SSH_ERROR) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, "SSH non-blocking read failed");
    }
    if (rc > 0) {
      output.append(buffer, rc);
      last_data = std::chrono::steady_clock::now();
    }

    // Read stderr (stream 1)
    rc = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 1);
    if (rc == SSH_ERROR) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_SESSION_FAILED, "SSH non-blocking read failed (stderr)");
    }
    if (rc > 0) {
      error_output.append(buffer, rc);
      last_data = std::chrono::steady_clock::now();
    }

    // Check idle and total timeout
    auto now = std::chrono::steady_clock::now();
    auto idle_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_data);
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    if (idle_time.count() >= cs.ssh_idle_timeout || total_time.count() > cs.ssh_total_timeout) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!error_output.empty()) {
    THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_COMMAND_FAILED, operation_map.err_msg + " (stderr: " + error_output + ")");
  }

  std::istringstream iss(output);
  std::string line;
  int line_index = 0;
  
  while (std::getline(iss, line)) {
      if (!line.empty() && line.back() == '\r') {
          line.pop_back();
      }
  
      if (line_index++ < operation_map.skip_head)
          continue;
  
      if (hasError(line))
          THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_COMMAND_FAILED, operation_map.err_msg + " (" + line + ")");
  
      if (std::regex_match(line, std::regex(R"(^%)", std::regex::icase)))
          continue;
  
      output_lines.push_back(std::move(line));
  }

  if (operation_map.skip_tail > 0 && operation_map.skip_tail < static_cast<int>(output_lines.size())) {
    output_lines.resize(output_lines.size() - operation_map.skip_tail);
  }

  return output_lines;
}

std::string Ssh::verifyKnownHost(ssh_session session) {
  enum ssh_known_hosts_e state;
  unsigned char *hash = NULL;
  ssh_key srv_pubkey = NULL;
  size_t hlen;
  char *p;
  int cmp;
  int rc;

  rc = ssh_get_server_publickey(session, &srv_pubkey);
  if (rc < 0) {
    return "Could not get server public key.";
  }

  rc = ssh_get_publickey_hash(srv_pubkey,
                              SSH_PUBLICKEY_HASH_SHA1,
                              &hash,
                              &hlen);
  ssh_key_free(srv_pubkey);
  if (rc < 0) {
    return "Could not get server public key hash.";
  }

  state = ssh_session_is_known_server(session);
  switch (state) {
    case SSH_KNOWN_HOSTS_OK:
      /* OK */

      break;
    case SSH_KNOWN_HOSTS_CHANGED: ssh_clean_pubkey_hash(&hash);
      return "Host key for server changed.";
    case SSH_KNOWN_HOSTS_OTHER|SSH_KNOWN_HOSTS_NOT_FOUND:
      ssh_clean_pubkey_hash(&hash);
      return "The host key for this server was not found.";
    case SSH_KNOWN_HOSTS_UNKNOWN:
      ssh_clean_pubkey_hash(&hash);
      rc = ssh_session_update_known_hosts(session);
      if (rc < 0) {
          return strerror(errno);
      }
      break;
    case SSH_KNOWN_HOSTS_ERROR:
      ssh_clean_pubkey_hash(&hash);
      return ssh_get_error(session);
  }

  ssh_clean_pubkey_hash(&hash);
  return "";
}

void Ssh::flushBanner(ssh_session session, ssh_channel channel) const {
  char buffer[256];
  int rc;
  auto start = std::chrono::steady_clock::now();
  auto last_data = start;
  chronicleSettings cs = getChronicleSettings();
 
  while (true) {
    rc = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);
    if (rc == SSH_ERROR) {
      THROW_CHRONICLE_EXCEPTION(CHRONICLE_ERROR_SSH_UNKNOWN, "SSH non-blocking read failed during flushBanner");
    }
    if (rc > 0) {
      last_data = std::chrono::steady_clock::now();
      // Just discard the data
    } else {
      auto now = std::chrono::steady_clock::now();
      auto idle_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_data);
      auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
      if (idle_time.count() > cs.ssh_idle_timeout || total_time.count() > cs.ssh_total_timeout) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }
}
