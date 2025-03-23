#include "Client.hpp"
#include "CommandHandler.hpp"

Client::Client(int fd, const std::string &ip)
    : _fd(fd), _ip(ip), _authenticated(false), _registered(false) {}

Client::~Client() {}

int Client::getFd() const { return _fd; }
const std::string &Client::getNickname() const { return _nickname; }
const std::string &Client::getUsername() const { return _username; }
bool Client::isAuthenticated() const { return _authenticated; }
bool Client::isRegistered() const { return _registered; }

void Client::setNickname(const std::string &nickname) { _nickname = nickname; }
void Client::setUsername(const std::string &username) { _username = username; }
void Client::setAuthenticated(bool status) { _authenticated = status; }
void Client::setRegistered(bool status) { _registered = status; }

std::string &Client::getBuffer() { return _buffer; }
void Client::appendToBuffer(const std::string &data) { _buffer += data; }
void Client::clearBuffer() { _buffer.clear(); }

