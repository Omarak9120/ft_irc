#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>
#include <map>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
private:
    int _port;
    std::string _password;
    int _serverSocket;
    std::vector<pollfd> _pollfds;
	std::map<int, Client*> _clients; // key: fd
    std::map<std::string, Client*> _nickToClient; // nickname -> client
    std::map<std::string, Channel*> _channels;

    void initSocket();
    void handleNewConnection();
    void handleClientData(int clientFd);

public:
    Server(int port, const std::string &password);
    ~Server();

    void run();

    const std::string &getPassword() const { return _password; }
    bool isNicknameTaken(const std::string &nickname) const;
    void registerNickname(const std::string &nickname, Client* client);
    void unregisterNickname(const std::string &nickname);
    Channel* getOrCreateChannel(const std::string& name);
    const std::map<int, Client*>& getClients() const { return _clients; }
    Client* getClientByNickname(const std::string& nickname) const;

};

#endif
