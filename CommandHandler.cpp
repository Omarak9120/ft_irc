/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oabdelka <oabdelka@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 22:07:37 by oabdelka          #+#    #+#             */
/*   Updated: 2025/03/24 22:07:38 by oabdelka         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
    else if (command == "KICK")
        handleKICK(tokens, client, server);
    else if (command == "TOPIC")
        handleTOPIC(tokens, client, server);
    else if (command == "INVITE")
        handleINVITE(tokens, client, server);
    else if (command == "MODE")
        handleMODE(tokens, client, server);

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

    // Check +i (invite-only)
    if (channel->hasMode('i') && !channel->isInvited(client)) {
        std::string error = ":ircserv 473 " + client->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    // Check +k (password)
    if (channel->hasKey()) {
        if (args.size() < 3 || args[2] != channel->getKey()) {
            std::string error = ":ircserv 475 " + client->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n";
            send(client->getFd(), error.c_str(), error.length(), 0);
            return;
        }
    }

    // Check +l (user limit)
    if (channel->hasUserLimit() && channel->getClients().size() >= channel->getUserLimit()) {
        std::string error = ":ircserv 471 " + client->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->hasClient(client)) {
        channel->addClient(client);

        if (channel->getClients().size() == 1)
            channel->addOperator(client);

        channel->removeInvite(client); // remove invite if used

        std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
        for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
            send((*it)->getFd(), joinMsg.c_str(), joinMsg.length(), 0);
        }

        if (!channel->getTopic().empty()) {
            std::string topicMsg = ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
            send(client->getFd(), topicMsg.c_str(), topicMsg.length(), 0);
        } else {
            std::string notopicMsg = ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
            send(client->getFd(), notopicMsg.c_str(), notopicMsg.length(), 0);
        }

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

        //  Sender must be in the channel
        if (!channel->hasClient(client)) {
            std::string error = ":ircserv 404 " + target + " :Cannot send to channel\r\n";
            send(client->getFd(), error.c_str(), error.length(), 0);
            return;
        }

        //  Broadcast to everyone else in the channel
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

void CommandHandler::handleKICK(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 KICK :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 3) {
        std::string error = ":ircserv 461 KICK :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = args[1];
    std::string targetNick = args[2];
    std::string reason = (args.size() >= 4) ? args[3] : "Kicked";

    Channel* channel = server.getOrCreateChannel(channelName);

    if (!channel->hasClient(client)) {
        std::string error = ":ircserv 442 " + channelName + " :You're not on that channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(client)) {
        std::string error = ":ircserv 482 " + channelName + " :You're not a channel operator\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    Client* targetClient = NULL;
    for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
        if ((*it)->getNickname() == targetNick) {
            targetClient = *it;
            break;
        }
    }

    if (!targetClient) {
        std::string error = ":ircserv 441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    // Broadcast KICK message to all channel users
    std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
        send((*it)->getFd(), kickMsg.c_str(), kickMsg.length(), 0);
    }

    // Remove client from channel
    channel->removeClient(targetClient);
}

void CommandHandler::handleTOPIC(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 TOPIC :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 2) {
        std::string error = ":ircserv 461 TOPIC :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = args[1];
    Channel* channel = server.getOrCreateChannel(channelName);

    if (!channel->hasClient(client)) {
        std::string error = ":ircserv 442 " + channelName + " :You're not on that channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    //  SET TOPIC
    if (args.size() >= 3) {
        // 🔐 If +t is set, only operators can set the topic
        if (channel->hasMode('t') && !channel->isOperator(client)) {
            std::string error = ":ircserv 482 " + channelName + " :You're not a channel operator\r\n";
            send(client->getFd(), error.c_str(), error.length(), 0);
            return;
        }

        // Combine topic text
        std::string newTopic;
        for (size_t i = 2; i < args.size(); ++i) {
            if (i == 2 && args[i][0] == ':')
                newTopic = args[i].substr(1);
            else
                newTopic += " " + args[i];
        }

        channel->setTopic(newTopic);

        std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
        for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
            send((*it)->getFd(), topicMsg.c_str(), topicMsg.length(), 0);
        }
    }

    //  VIEW TOPIC
    else {
        if (!channel->getTopic().empty()) {
            std::string topic = ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
            send(client->getFd(), topic.c_str(), topic.length(), 0);
        } else {
            std::string notopic = ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
            send(client->getFd(), notopic.c_str(), notopic.length(), 0);
        }
    }
}


void CommandHandler::handleINVITE(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 INVITE :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 3) {
        std::string error = ":ircserv 461 INVITE :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string targetNick = args[1];
    std::string channelName = args[2];

    Channel* channel = server.getOrCreateChannel(channelName);
    if (!channel->hasClient(client)) {
        std::string error = ":ircserv 442 " + channelName + " :You're not on that channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(client)) {
        std::string error = ":ircserv 482 " + channelName + " :You're not a channel operator\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    Client* targetClient = NULL;
    for (std::map<int, Client*>::const_iterator it = server.getClients().begin(); it != server.getClients().end(); ++it) {
        if (it->second->getNickname() == targetNick) {
            targetClient = it->second;
            break;
        }
    }

    if (!targetClient) {
        std::string error = ":ircserv 401 " + targetNick + " :No such nick\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (channel->hasClient(targetClient)) {
        std::string error = ":ircserv 443 " + targetNick + " " + channelName + " :is already on channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    // Add to invite list
    channel->invite(targetClient);

    std::string msg = ":" + client->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    send(targetClient->getFd(), msg.c_str(), msg.length(), 0);
}

void CommandHandler::handleMODE(const std::vector<std::string>& args, Client* client, Server& server) {
    if (!client->isAuthenticated() || !client->isRegistered()) {
        std::string error = ":ircserv 451 MODE :You have not registered\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (args.size() < 3) {
        std::string error = ":ircserv 461 MODE :Not enough parameters\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = args[1];
    std::string modeString = args[2];

    Channel* channel = server.getOrCreateChannel(channelName);

    if (!channel->hasClient(client)) {
        std::string error = ":ircserv 442 " + channelName + " :You're not on that channel\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(client)) {
        std::string error = ":ircserv 482 " + channelName + " :You're not a channel operator\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    char sign = modeString[0];
    if (sign != '+' && sign != '-') {
        std::string error = ":ircserv 472 " + modeString + " :is unknown mode char\r\n";
        send(client->getFd(), error.c_str(), error.length(), 0);
        return;
    }

    size_t argIndex = 3; // Extra parameters start after mode string

    std::string modesConfirmed = ":" + client->getNickname() + " MODE " + channelName + " " + modeString;

    for (size_t i = 1; i < modeString.length(); ++i) {
        char mode = modeString[i];

        switch (mode) {
            case 'i':
                if (sign == '+')
                    channel->addMode('i');
                else
                    channel->removeMode('i');
                break;

            case 't':
                if (sign == '+')
                    channel->addMode('t');
                else
                    channel->removeMode('t');
                break;

            case 'k':
                if (sign == '+') {
                    if (args.size() <= argIndex) {
                        std::string error = ":ircserv 461 MODE :Missing key parameter for +k\r\n";
                        send(client->getFd(), error.c_str(), error.length(), 0);
                        return;
                    }
                    channel->setKey(args[argIndex++]);
                } else {
                    channel->removeKey();
                }
                break;

            case 'l':
                if (sign == '+') {
                    if (args.size() <= argIndex) {
                        std::string error = ":ircserv 461 MODE :Missing parameter for +l\r\n";
                        send(client->getFd(), error.c_str(), error.length(), 0);
                        return;
                    }

                    std::istringstream iss(args[argIndex++]);
                    size_t limit;
                    if (!(iss >> limit)) {
                        std::string error = ":ircserv 461 MODE :Invalid limit value\r\n";
                        send(client->getFd(), error.c_str(), error.length(), 0);
                        return;
                    }

                    channel->setUserLimit(limit);
                } else {
                    channel->removeUserLimit();
                }
                break;

            case 'o':
                if (args.size() <= argIndex) {
                    std::string error = ":ircserv 461 MODE :Missing nickname parameter for +o/-o\r\n";
                    send(client->getFd(), error.c_str(), error.length(), 0);
                    return;
                }

                {
                    std::string targetNick = args[argIndex++];
                    Client* target = server.getClientByNickname(targetNick);

                    if (!target || !channel->hasClient(target)) {
                        std::string error = ":ircserv 441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
                        send(client->getFd(), error.c_str(), error.length(), 0);
                        return;
                    }

                    if (sign == '+')
                        channel->addOperator(target);
                    else
                        channel->removeOperator(target);

                    // Inform everyone about the operator change
                    std::string opChange = ":" + client->getNickname() + " MODE " + channelName + " " + sign + "o " + targetNick + "\r\n";
                    for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it)
                        send((*it)->getFd(), opChange.c_str(), opChange.length(), 0);
                }
                break;

            default:
                std::string error = ":ircserv 472 ";
                error += mode;
                error += " :is unknown mode char\r\n";
                send(client->getFd(), error.c_str(), error.length(), 0);
                return;
        }
    }

    modesConfirmed += "\r\n";

    // Broadcast the overall mode change to all users in the channel
    for (std::set<Client*>::iterator it = channel->getClients().begin(); it != channel->getClients().end(); ++it) {
        send((*it)->getFd(), modesConfirmed.c_str(), modesConfirmed.length(), 0);
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
