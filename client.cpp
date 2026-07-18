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
#include <thread>

#include "duo.h"
#include "command.h"

const std::string welcome_message =
    R"(Welcome to TerminalChat!
Commands:
  /help                   Show available commands
  /list                   List all lobbies
  /users                  List all users
  /create [name]          Create a lobby
  /join [lobbyId]         Join a lobby
  /leave                  Leave the current lobby
  /pm [userId] [message]  Private message an user
  /quit                   Leave the application (gracefully)
)";

const std::string help_string = R"(Command list:
  /help                   Show available commands
  /list                   List all lobbies
  /users                  List all users
  /create [name]          Create a lobby
  /join [lobbyId]         Join a lobby
  /leave                  Leave the current lobby
  /pm [userId] [message]  Private message an user
  /quit                   Leave the application (gracefully)
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

void reading_messages(int conn_fd)
{
    while (true)
    {
        char buffer[1024];
        int bytes_received = recv(conn_fd, buffer, sizeof(buffer), 0);
    }
}

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

        std::thread t(reading_messages, conn_fd);
        t.detach();

        while (true)
        {
            std::cout << ">";
            std::string content;
            std::getline(std::cin, content);

            Command command(content);

            switch (command.command_type)
            {
            case CommandType::Message:
            {
                std::string message = "";
                for (size_t i = 0; i < command.args.size(); i++)
                {
                    message += command.args[i];
                }
                duo::duo_message send_message(duo::MessageType::SendMessage, message);
                std::string send_message_str = send_message.dump();
                send(conn_fd, send_message_str.c_str(), send_message_str.size(), 0);
                break;
            }
            case CommandType::HelpCommand:
            {
                std::cout << colors[4] << help_string << color_escape << std::endl;
                break;
            }

            case CommandType::ListLobbies:
            {
                duo::duo_message list_lobbies(duo::MessageType::ListLobbies, "");
                std::string list_lobbies_str = list_lobbies.dump();
                send(conn_fd, list_lobbies_str.c_str(), list_lobbies_str.size(), 0);
                break;
            }
            case CommandType::ListUsers:
            {
                duo::duo_message list_users(duo::MessageType::ListUsers, "");
                std::string list_users_str = list_users.dump();
                send(conn_fd, list_users_str.c_str(), list_users_str.size(), 0);
                break;
            }
            case CommandType::CreateLobby:
            {

                std::string lobby_name = "";
                for (size_t i = 0; i < command.args.size(); i++)
                {
                    if (i > 0)
                    {
                        lobby_name += " ";
                    }
                    lobby_name += command.args[i];
                }
                duo::duo_message create_lobby(duo::MessageType::CreateLobby, lobby_name);
                std::string create_lobby_str = create_lobby.dump();
                send(conn_fd, create_lobby_str.c_str(), create_lobby_str.size(), 0);
                break;
            }
            case CommandType::JoinLobby:
            {
                if (command.args.size() == 1)
                {
                    std::string code = command.args[0];
                    duo::duo_message join_lobby(duo::MessageType::JoinLobby, code);
                    std::string join_lobby_str = join_lobby.dump();
                    send(conn_fd, join_lobby_str.c_str(), join_lobby_str.size(), 0);
                    break;
                }
                break;
            }
            case CommandType::PrivateMessage:
            {
                if (command.args.size() >= 2)
                {
                    std::string code = command.args[0];
                    std::string message = "";
                    for (size_t i = 1; i < command.args.size(); i++)
                    {
                        message += command.args[i];
                        message += " ";
                    }
                    std::string content = code + "##" + message;
                    duo::duo_message private_message(duo::MessageType::SendPrivateMessage, content);
                    std::string private_message_str = private_message.dump();
                    send(conn_fd, private_message_str.c_str(), private_message_str.size(), 0);
                    break;
                }
                break;
            }
            case CommandType::LeaveLobby:
            {
                duo::duo_message leave_lobby(duo::MessageType::LeaveLobby, "");
                std::string leave_lobby_str = leave_lobby.dump();

                send(conn_fd, leave_lobby_str.c_str(), leave_lobby_str.size(), 0);
                break;
            }
            case CommandType::QuitCommand:
            {
                return 0; // issue = early return
            }
            default:
                break;
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