#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string _ip;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    bool _authenticated;
    bool _registered;
    std::string _buffer; // accumulates incoming data
    

public:
    Client(int fd, const std::string &ip);
    ~Client();

    int getFd() const;
    const std::string &getNickname() const;
    const std::string &getUsername() const;
    bool isAuthenticated() const;
    bool isRegistered() const;

    void setNickname(const std::string &nickname);
    void setUsername(const std::string &username);
    void setAuthenticated(bool status);
    void setRegistered(bool status);
    std::string &getBuffer();
    void appendToBuffer(const std::string &data);
    void clearBuffer();

};

#endif
