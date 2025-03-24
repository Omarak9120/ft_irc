/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oabdelka <oabdelka@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 22:07:31 by oabdelka          #+#    #+#             */
/*   Updated: 2025/03/24 22:07:32 by oabdelka         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(const std::string& name) : _name(name), _topic(""), _hasKey(false), _userLimit(0), _hasUserLimit(false) {}


Channel::~Channel() {}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

void Channel::addClient(Client* client) {
    _clients.insert(client);
}

void Channel::removeClient(Client* client) {
    _clients.erase(client);
    _operators.erase(client);
}

bool Channel::hasClient(Client* client) const {
    return _clients.find(client) != _clients.end();
}

// void Channel::addOperator(Client* client) {
//     _operators.insert(client);
// }

bool Channel::isOperator(Client* client) const {
    return _operators.find(client) != _operators.end();
}

const std::set<Client*>& Channel::getClients() const {
    return _clients;
}

void Channel::invite(Client* client) {
    _invited.insert(client);
}

bool Channel::isInvited(Client* client) const {
    return _invited.find(client) != _invited.end();
}

void Channel::removeInvite(Client* client) {
    _invited.erase(client);
}

void Channel::addMode(char mode) {
    _modes.insert(mode);
}

void Channel::removeMode(char mode) {
    _modes.erase(mode);
}

bool Channel::hasMode(char mode) const {
    return _modes.find(mode) != _modes.end();
}

std::string Channel::getModes() const {
    std::string result = "+";
    for (std::set<char>::iterator it = _modes.begin(); it != _modes.end(); ++it)
        result += *it;
    return result;
}

void Channel::setKey(const std::string& key) {
    _key = key;
    _hasKey = true;
    addMode('k');
}

void Channel::removeKey() {
    _key.clear();
    _hasKey = false;
    removeMode('k');
}

bool Channel::hasKey() const {
    return _hasKey;
}

const std::string& Channel::getKey() const {
    return _key;
}

void Channel::setUserLimit(size_t limit) {
    _userLimit = limit;
    _hasUserLimit = true;
    addMode('l');
}

void Channel::removeUserLimit() {
    _userLimit = 0;
    _hasUserLimit = false;
    removeMode('l');
}

bool Channel::hasUserLimit() const {
    return _hasUserLimit;
}

size_t Channel::getUserLimit() const {
    return _userLimit;
}

void Channel::addOperator(Client* client) {
    _operators.insert(client);
    addMode('o');
}

void Channel::removeOperator(Client* client) {
    _operators.erase(client);
    if (_operators.empty())
        removeMode('o'); // optionally remove the 'o' mode if no ops left
}

// bool Channel::isOperator(Client* client) const {
//     return _operators.find(client) != _operators.end();
// }
