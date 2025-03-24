#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>
#include "Client.hpp"

class Channel {
private:
    std::string _name;
    std::string _topic;
    std::set<Client*> _clients;
    std::set<Client*> _operators;
    std::set<Client*> _invited;
    std::set<char> _modes;
    std::string _key;
    bool _hasKey;
    size_t _userLimit;
    bool _hasUserLimit;




public:
    Channel(const std::string& name);
    ~Channel();

    const std::string& getName() const;
    const std::string& getTopic() const;
    void setTopic(const std::string& topic);

    void addClient(Client* client);
    void removeClient(Client* client);
    bool hasClient(Client* client) const;

    void addOperator(Client* client);
    bool isOperator(Client* client) const;

    const std::set<Client*>& getClients() const;
    // -- incvite---
    void invite(Client* client);
    bool isInvited(Client* client) const;
    void removeInvite(Client* client);

    // --mode-

    void addMode(char mode);
    void removeMode(char mode);
    bool hasMode(char mode) const;
    std::string getModes() const; // for replying with active modes

    // --key--

    void setKey(const std::string& key);
    void removeKey();
    bool hasKey() const;
    const std::string& getKey() const;

    // --lIMIT 

    void setUserLimit(size_t limit);
    void removeUserLimit();
    bool hasUserLimit() const;
    size_t getUserLimit() const;

    // operator

    // void addOperator(Client* client);
    void removeOperator(Client* client);
    // bool isOperator(Client* client) const;


};

#endif
