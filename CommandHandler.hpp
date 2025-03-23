#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <vector>
#include "Client.hpp"
#include "Server.hpp"

class CommandHandler {
public:
    static void handleCommand(const std::string& line, Client* client, Server& server);

private:
    static void handlePASS(const std::vector<std::string>& args, Client* client, Server& server);
    static void handleNICK(const std::vector<std::string>& args, Client* client, Server& server);
    static void handleUSER(const std::vector<std::string>& args, Client* client, Server& server);
    static void checkAndWelcome(Client* client);
    static void handleJOIN(const std::vector<std::string>& args, Client* client, Server& server);
    static void handlePRIVMSG(const std::vector<std::string>& args, Client* client, Server& server);




    static std::vector<std::string> split(const std::string& s);
};

#endif
