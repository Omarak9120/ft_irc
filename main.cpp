/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mkaterji <mkaterji@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 22:07:40 by oabdelka          #+#    #+#             */
/*   Updated: 2025/04/05 15:23:13 by mkaterji         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstdlib>

int check_password(const char *pass)
{
    while(*pass)
    {
        if(*pass != ' ' || *pass != '\t')
            return 0;
        else
            pass++;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);
    
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number.\n";
        return 1;
    }

    if(!check_password(argv[2]))
    {
        std::cerr << "Password can't contain spaces or tabs!\n";
        return 1;
    }
    
    std::string password = argv[2];
    
    try {
        Server server(port, password);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
    
    return 0;
}