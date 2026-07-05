#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <limits>

#include "duo.h"
#include "command.h"

const std::string welcome_message =
    R"(Welcome to TerminalChat!
Commands:
  /help             Show available commands
  /list             List all lobbies
  /users            List all users
  /create [name]    Create a lobby
  /join [lobbyCode] Join a lobby
  /leave            Leave the current lobby
  /pm [userId]      Private message an user
)";

std::string colors[] = {
    "\033[30m", // black
    "\033[31m", // red
    "\033[32m", // green
    "\033[33m", // yellow
    "\033[34m", // blue
    "\033[35m", // magenta
    "\033[36m", // cyan
    "\033[37m", // white
};

std::string color_escape = "\033[0m";

int main(int argc, char *argv[])
{
    addrinfo hints;
    addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    int status = getaddrinfo("localhost", "4040", &hints, &result);
    for (addrinfo *current = result; current != nullptr; current = current->ai_next)
    {
        sockaddr_in *ipv4;
        sockaddr_in6 *ipv6;

        std::string ipver;
        char ipstr[INET6_ADDRSTRLEN];
        void *addr = nullptr;

        if (current->ai_family == AF_INET)
        {
            ipver = "IPv4";
            ipv4 = reinterpret_cast<sockaddr_in *>(current->ai_addr);
            addr = &(ipv4->sin_addr);
        }
        else
        {
            ipver = "IPv6";
            ipv6 = reinterpret_cast<sockaddr_in6 *>(current->ai_addr);
            addr = &(ipv6->sin6_addr);
        }
    }
    int conn_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    connect(conn_fd, result->ai_addr, result->ai_addrlen);
    bool connection_open = true;

    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(conn_fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
    {
        std::cout << "Server not responded" << std::endl;
        return 0;
    }
    std::string body(buffer, bytes_received);
    duo::duo_message server_message = duo::deserialize(body);
    if (server_message.get_type() == duo::MessageType::Connected)
    {
        std::cout << colors[2] << "Successfully connected to the server" << color_escape << std::endl;
        std::cout << colors[4] << welcome_message << color_escape << std::endl;

        bool ack_username = false;
        int attempt = 0;
        std::string username;
        while (!ack_username)
        {

            ++attempt;
            if (attempt > 1)
            {
                std::cout << colors[1] << "Unsuccesfull try, try again" << color_escape << "\n";
            }
            std::cout << colors[5] << "What is your username? " << color_escape;
            std::cin >> username;

            duo::duo_message username_message(duo::MessageType::SendUsername, username);
            std::string username_message_str = username_message.dump();

            send(conn_fd, username_message_str.c_str(), username_message_str.size(), 0);

            std::memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(conn_fd, buffer, sizeof(buffer), 0);
            std::string str_response(buffer, bytes_received);

            duo::duo_message username_response = duo::deserialize(str_response);
            if (username_response.get_type() == duo::MessageType::AckUsername)
            {
                ack_username = true;
            }
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Welcome to the TerminalChat, " << colors[4] << username << color_escape << "!" << std::endl;

        while (true)
        {
            std::cout << ">";
            std::string content;
            std::getline(std::cin, content);

            Command command(content);
            std::cout << command.command_type << "\n";
            if (command.command_type == CommandType::Message)
            {
            }
            else if (command.command_type == CommandType::Unknown)
            {
            }
            else if (command.command_type == CommandType::HelpCommand)
            {
            }
        }
    }
    else
    {
        std::cout << "Server unrecognizable" << std::endl;
        return 0;
    }

    close(conn_fd);
    freeaddrinfo(result);
    return 0;
}