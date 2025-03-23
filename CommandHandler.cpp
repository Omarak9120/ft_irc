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
    else if (command == "JOIN")
        handleJOIN(tokens, client, server);
    else if (command == "PRIVMSG")
        handlePRIVMSG(tokens, client, server);
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
    checkAndWelcome(client);

}

void CommandHandler::handleNICK(const std::vector<std::string>& args, Client* client, Server& server) {
    if (args.size() < 2) {
        std::cout << "NICK: Not enough parameters\n";
        return;
    }

    std::string newNick = args[1];

    if (server.isNicknameTaken(newNick)) {
        std::string error = ":ircserv 433 " + newNick + " :Nickname is already in use\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        std::cout << "Rejected NICK '" << newNick << "' — already in use\n";
        return;
    }

    // If client already had a nickname, unregister the old one
    std::string oldNick = client->getNickname();
    if (!oldNick.empty())
        server.unregisterNickname(oldNick);

    client->setNickname(newNick);
    server.registerNickname(newNick, client);

    std::cout << "Client " << client->getFd() << " set nickname to " << newNick << "\n";
    checkAndWelcome(client);

}




void CommandHandler::handleUSER(const std::vector<std::string>& args, Client* client, Server& server) {
    (void)server;
    if (args.size() < 5) {
        std::cout << "USER: Not enough parameters\n";
        return;
    }

    client->setUsername(args[1]);

    std::cout << "Client " << client->getFd() << " registered with username: " << args[1] << "\n";

    checkAndWelcome(client);

}

void CommandHandler::handleJOIN(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 JOIN :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 2) {
        std::string error = ":ircserv 461 JOIN :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = args[1];
    if (channelName[0] != '#') {
        std::string error = ":ircserv 476 " + channelName + " :Invalid channel name\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    Channel* channel = server.getOrCreateChannel(channelName);

    if (!channel->hasClient(client)) {
        channel->addClient(client);

        // Make first user an operator
        if (channel->getClients().size() == 1)
            channel->addOperator(client);

        std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
        for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
            send((*it)->getFd(), joinMsg.c_str(), joinMsg.length(), 0);
        }

        // Send topic (if set)
        if (!channel->getTopic().empty()) {
            std::string topicMsg = ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
            send(client->getFd(), topicMsg.c_str(), topicMsg.length(), 0);
        } else {
            std::string notopicMsg = ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
            send(client->getFd(), notopicMsg.c_str(), notopicMsg.length(), 0);
        }

        // Send names list
        std::string names = ":ircserv 353 " + client->getNickname() + " = " + channelName + " :";
        for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
            if (channel->isOperator(*it))
                names += "@" + (*it)->getNickname() + " ";
            else
                names += (*it)->getNickname() + " ";
        }
        names += "\r\n";
        send(client->getFd(), names.c_str(), names.length(), 0);

        std::string endOfNames = ":ircserv 366 " + client->getNickname() + " " + channelName + " :End of /NAMES list\r\n";
        send(client->getFd(), endOfNames.c_str(), endOfNames.length(), 0);
    }
}

void CommandHandler::handlePRIVMSG(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 PRIVMSG :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 3) {
        std::string error = ":ircserv 461 PRIVMSG :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string target = args[1];

    // Reconstruct message with spaces
    std::string msg;
    for (size_t i = 2; i < args.size(); ++i) {
        if (i == 2 && args[i][0] == ':')
            msg = args[i].substr(1);
        else
            msg += " " + args[i];
    }

    std::string formatted = ":" + client->getNickname() + " PRIVMSG " + target + " :" + msg + "\r\n";

    if (target[0] == '#') {
        // Channel message
        Channel* channel = server.getOrCreateChannel(target);

        // 🔐 Sender must be in the channel
        if (!channel->hasClient(client)) {
            std::string error = ":ircserv 404 " + target + " :Cannot send to channel\r\n";
            send(client->getFd(), error.c_str(), error.length(), 0);
            return;
        }

        // 🔊 Broadcast to everyone else in the channel
        for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
            if (*it != client) {
                send((*it)->getFd(), formatted.c_str(), formatted.length(), 0);
            }
        }
    } else {
        // Private message to another user
        Client* targetClient = NULL;
        for (std::map<int, Client*>::const_iterator it = server.getClients().begin(); it != server.getClients().end(); ++it) {
            if (it->second->getNickname() == target) {
                targetClient = it->second;
                break;
            }
        }

        if (!targetClient) {
            std::string error = ":ircserv 401 " + target + " :No such nick/channel\r\n";
            send(client->getFd(), error.c_str(), error.length(), 0);
            return;
        }

        send(targetClient->getFd(), formatted.c_str(), formatted.length(), 0);
    }
}



void CommandHandler::checkAndWelcome(Client* client) {
    if (client->isAuthenticated()
        && !client->getNickname().empty()
        && !client->getUsername().empty()
        && !client->isRegistered()) {

        client->setRegistered(true);
        std::string welcome = ":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server\r\n";
        send(client->getFd(), welcome.c_str(), welcome.length(), 0);
        std::cout << "Welcome sent to " << client->getNickname() << "\n";
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
