#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_ntoa
#include "CommandHandler.hpp"

Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _serverSocket(-1) {
    initSocket();
}

Server::~Server() {
    close(_serverSocket);
}

void Server::initSocket() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0)
        throw std::runtime_error("Failed to create socket");

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)// Allows reusing the same port quickly after restart
        throw std::runtime_error("setsockopt failed");

    fcntl(_serverSocket, F_SETFL, O_NONBLOCK);// Sets socket to non-blocking mode

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) // Binds socket to the specified port on the local machine
        throw std::runtime_error("Bind failed");

    if (listen(_serverSocket, 10) < 0) // Prepares it to listen for incoming connections
        throw std::runtime_error("Listen failed");

    pollfd pfd;
    pfd.fd = _serverSocket;
    pfd.events = POLLIN;
    _pollfds.push_back(pfd);

    std::cout << "Server is listening on port " << _port << "\n";
}

void Server::run() {
    while (true) {
        int ret = poll(&_pollfds[0], _pollfds.size(), -1);
        if (ret < 0)
            throw std::runtime_error("Poll failed");

        for (size_t i = 0; i < _pollfds.size(); ++i) {
            if (_pollfds[i].revents & POLLIN) {
                if (_pollfds[i].fd == _serverSocket) {
                    handleNewConnection();
                } else {
                    handleClientData(_pollfds[i].fd);
                }
            }
        }
    }
}

void Server::handleNewConnection() {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr *)&clientAddr, &len);
    if (clientFd < 0)
        return;

    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    _pollfds.push_back(clientPoll);

    std::string ip = inet_ntoa(clientAddr.sin_addr);
    _clients[clientFd] = new Client(clientFd, ip);

    std::cout << "New client connected: " << clientFd << " (IP: " << ip << ")\n";
}

void Server::handleClientData(int clientFd) {
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytes = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        std::cout << "Client disconnected: " << clientFd << "\n";
    
        // Cleanup nickname if set
        Client* client = _clients[clientFd];
        std::string nick = client->getNickname();
        if (!nick.empty())
            unregisterNickname(nick);
    
        // Close socket
        close(clientFd);
    
        // Remove from poll list
        for (size_t i = 0; i < _pollfds.size(); ++i) {
            if (_pollfds[i].fd == clientFd) {
                _pollfds.erase(_pollfds.begin() + i);
                break;
            }
        }
    
        // Delete client
        delete client;
        _clients.erase(clientFd);
        return;
    }
    

    Client* client = _clients[clientFd];
    client->appendToBuffer(std::string(buffer, bytes));

    std::string& fullBuffer = client->getBuffer();
    size_t pos;
    while ((pos = fullBuffer.find("\n")) != std::string::npos) {
        std::string commandLine = fullBuffer.substr(0, pos);
        if (!commandLine.empty() && commandLine[commandLine.length() - 1] == '\r')
            commandLine = commandLine.substr(0, commandLine.length() - 1);
        fullBuffer.erase(0, pos + 1);
    
        std::string displayName = client->getNickname().empty() ? "(unknown)" : client->getNickname();
        std::cout << "Parsing command from client " << clientFd << " (" << displayName << "): " << commandLine << "\n";
        
        CommandHandler::handleCommand(commandLine, client, *this);
    }
    
}

// nicknamme

bool Server::isNicknameTaken(const std::string &nickname) const {
    return _nickToClient.find(nickname) != _nickToClient.end();
}

void Server::registerNickname(const std::string &nickname, Client* client) {
    _nickToClient[nickname] = client;
}

void Server::unregisterNickname(const std::string &nickname) {
    _nickToClient.erase(nickname);
}

// Chanel 

Channel* Server::getOrCreateChannel(const std::string& name) {
    if (_channels.find(name) == _channels.end())
        _channels[name] = new Channel(name);
    return _channels[name];
}