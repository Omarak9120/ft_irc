#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <sys/socket.h> // for send()

// std::string &Client::getBuffer() {
//     return _buffer;
// }

// void Client::appendToBuffer(const std::string &data) {
//     _buffer += data;
// }

// void Client::clearBuffer() {
//     _buffer.clear();
// }


void CommandHandler::handleCommand(const std::string& line, Client* client, Server& server) {
    std::vector<std::string> tokens = split(line);
    if (tokens.empty())
        return;

    std::string command = tokens[0];

    if (command == "PASS")
        handlePASS(tokens, client, server);
    else if (command == "NICK")
        handleNICK(tokens, client, server);
    else if (command == "USER")
        handleUSER(tokens, client, server);
    else
        std::cout << "Unknown command: " << command << std::endl;
}

// --- Private ---

void CommandHandler::handlePASS(const std::vector<std::string>& args, Client* client, Server& server) {
    if (args.size() < 2) {
        std::cout << "PASS: Not enough parameters\n";
        return;
    }

    if (client->isAuthenticated()) {
        std::cout << "PASS: Already authenticated\n";
        return;
    }

    if (args[1] == server.getPassword()) {
        client->setAuthenticated(true);
        std::cout << "PASS accepted from client " << client->getFd() << "\n";
    } else {
        std::cout << "PASS rejected (wrong password)\n";
        // optionally disconnect the client here
    }
}

void CommandHandler::handleNICK(const std::vector<std::string>& args, Client* client, Server& server) {
    (void)server;
    if (args.size() < 2) {
        std::cout << "NICK: Not enough parameters\n";
        return;
    }

    std::string nick = args[1];
    client->setNickname(nick);
    std::cout << "Client " << client->getFd() << " set nickname to " << nick << "\n";
}

void CommandHandler::handleUSER(const std::vector<std::string>& args, Client* client, Server& server) {
    (void)server;
    if (args.size() < 5) {
        std::cout << "USER: Not enough parameters\n";
        return;
    }

    client->setUsername(args[1]);
    client->setRegistered(true);

    std::cout << "Client " << client->getFd() << " registered with username: " << args[1] << "\n";

    // Optionally check if client is fully ready (PASS + NICK + USER)
    if (client->isAuthenticated() && !client->getNickname().empty()) {
        std::string welcome = ":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server\r\n";
        send(client->getFd(), welcome.c_str(), welcome.length(), 0);
    }
}

// --- Helpers ---

std::vector<std::string> CommandHandler::split(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
