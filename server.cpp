#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include "command.h"
#include "duo.h"

#define MEMBER_LIMIT 100

std::string to_str(int x)
{
    std::string result;
    do
    {
        result += char('0' + (x % 10));
        x /= 10;
    } while (x > 0);
    std::reverse(result.begin(), result.end());
    return result;
}

class Member
{
public:
    int id;
    std::string name;
    int lobbyId;
    int socketFd;

    Member(std::string name, int memberId, int socketFd) : id(memberId), socketFd(socketFd), name(name), lobbyId(-1) {}

    void set_lobby_id(int id)
    {
        lobbyId = id;
    }
};

class Lobby
{

public:
    int id;
    std::string name;
    std::vector<int> memberIds;
    int capacity;
    Lobby(int id, std::string name) : id(id), name(name), capacity(10) {}
    Lobby(int id, std::string name, int cap) : id(id), name(name), capacity(cap) {}
};

class Manager
{
    int nextMemberId;
    int nextLobbyId;
    std::vector<Member> memberList;
    std::vector<Lobby> lobbyList;

    std::mutex mutex;
    std::string write_user_id(int number)
    {
        std::string result;
        do
        {
            int dig = number % 10;
            char f = char(dig + '0');
            result += f;
            number /= 10;
        } while (number > 0);
        std::reverse(result.begin(), result.end());
        return result;
    }

public:
    Manager() : nextMemberId(0), nextLobbyId(0)
    {
    }

    int add_new_member(const char *name, int socketFd)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int currentId = nextMemberId;
        nextMemberId++;

        memberList.push_back(Member(name, currentId, socketFd));
        return currentId;
    }

    int add_new_lobby(const char *name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int currentId = nextLobbyId;
        nextLobbyId++;

        lobbyList.push_back(Lobby(currentId, name));
        return currentId;
    }

    std::string produce_lobby_list()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::string lobby_list;
        for (auto &m : lobbyList)
        {
            lobby_list += "# ";
            lobby_list += m.name;
            lobby_list += "(" + to_str(m.id) + ")";
            lobby_list += " " + to_str(m.memberIds.size()) + "/" + to_str(m.capacity);
            lobby_list += "\n";
        }
        return lobby_list;
    }

    std::string produce_user_list()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::string user_list;
        for (auto &m : memberList)
        {
            std::string member_string = "= " + m.name + "#" + write_user_id(m.id) + "\n";
            user_list += member_string;
        }
        std::cout << user_list << "?\n";
        return user_list;
    }

    int add_member_to_lobby(int memberId, int lobbyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto &m : lobbyList)
        {
            if (m.id == lobbyId)
            {
                if (m.memberIds.size() >= m.capacity)
                {
                    return 0;
                }
                m.memberIds.push_back(memberId);
                for (auto &member : memberList)
                {
                    if (member.id == memberId)
                    {
                        member.set_lobby_id(lobbyId);
                    }
                }
                return 1;
            }
        }
        return 0;
    }

    int leave_lobby(int memberId)
    {
        for (auto &m : memberList)
        {
            if (m.lobbyId != -1)
            {
                for (auto &l : lobbyList)
                {
                    if (l.id == m.lobbyId)
                    {
                        int index = -1;
                        for (int i = 0; i < l.memberIds.size(); i++)
                        {
                            if (l.memberIds[i] == memberId)
                            {
                                index = i;
                            }
                        }
                        if (index != -1)
                        {
                            l.memberIds.erase(l.memberIds.begin() + index);
                        }
                        break;
                    }
                }
                m.lobbyId = -1;
                return 1;
            }
        }
        return 0;
    }

    std::string get_lobby_name(int lobbyId)
    {
        for (auto &m : lobbyList)
        {
            if (m.id == lobbyId)
            {
                return m.name;
            }
        }
        return "Lobby not found";
    }

    int send_message_to_user(int memberId, std::string message)
    {
        for (auto &m : memberList)
        {
            if (m.id == memberId)
            {
                std::string new_message = "\n" + message + "\n>";
                send(m.socketFd, message.c_str(), message.size(), 0);
                return 1;
            }
        }
        return 0;
    }
};

const std::string opening =
    R"(Welcome to TerminalChat!
Commands:
  /help             Show available commands
  /list             List all lobbies
  /users            List all users
  /create [name]    Create a lobby
  /join [lobbyCode] Join a lobby
  /leave            Leave the current lobby
  /pm [userId]      Private message an user
Enter your username: )";

const std::string command_list =
    R"(Commands:
  /help             Show available commands
  /list             List all lobbies
  /users            List all users
  /create [name]    Create a lobby
  /join [lobbyCode] Join a lobby
  /leave            Leave the current lobby
  /pm [userId]      Private message an user)";

enum CommunicationState
{
    WaitUsername,
    WaitMessage
};

std::string sanitize_string(std::string str)
{
    while (!str.empty() && (str.back() == '\r' || str.back() == '\n' || str.back() == ' '))
    {
        str.pop_back();
    }
    return str;
}

int parse_string_int(std::string str)
{
    int result = 0;
    for (char x : str)
    {
        if ('0' <= x && x <= '9')
        {
            result = 10 * result;
            result += x - '0';
            if (result > MEMBER_LIMIT)
            {
                return -1;
            }
        }
    }
    return result;
}

void talk_to_client(int client_fd, Manager &manager)
{
    std::cout << "Manage client communication!\n";
    char buffer[1024];
    // send(client_fd, opening.c_str(), opening.size(), 0);
    // std::cout << "Sent this string: " << opening << "\n";

    duo::duo_message conn_message{duo::MessageType::Connected, ""};
    std::string conn_message_str = conn_message.dump();
    send(client_fd, conn_message_str.c_str(), conn_message_str.size(), 0);

    CommunicationState state = WaitUsername;
    std::string name;
    int userId = -1;
    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            std::cout << "Client gone\n";
            break;
        }

        std::string client_message(buffer, bytes_received);
        duo::duo_message recv_message = duo::deserialize(client_message);

        std::string text;
        if (state == WaitUsername)
        {
            if (recv_message.get_type() == duo::MessageType::SendUsername)
            {
                std::string username = recv_message.get_content();
                duo::duo_message ack_message(duo::MessageType::AckUsername, "");
                std::string ack_message_str = ack_message.dump();
                send(client_fd, ack_message_str.c_str(), ack_message_str.size(), 0);
                state = WaitMessage;
                continue;
            }
            else
            {
                continue;
            }
        }
        else
        {
            Command message(buffer);
            if (message.command_type == CommandType::HelpCommand)
            {
                if (message.args.size() == 0)
                {
                    text += command_list;
                    text += "\n";
                }
            }
            if (message.command_type == CommandType::ListLobbies)
            {
                if (message.args.size() == 0)
                {
                    text += manager.produce_lobby_list();
                }
            }
            else if (message.command_type == CommandType::ListUsers)
            {
                if (message.args.size() == 0)
                {
                    text += manager.produce_user_list();
                }
            }
            else if (message.command_type == CommandType::CreateLobby)
            {
                if (message.args.size() == 1)
                {
                    std::string name = message.args[0];
                    int lobbyId = manager.add_new_lobby(name.c_str());
                    manager.add_member_to_lobby(userId, lobbyId);
                    text += "Created a new lobby with name: " + name + "\n";
                    text += "Others can join this lobby with a code: " + to_str(lobbyId) + "\n";
                }
            }
            else if (message.command_type == CommandType::JoinLobby)
            {
                std::cout << "JoinLobby\n";
                if (message.args.size() == 1)
                {
                    manager.leave_lobby(userId);
                    std::string strLobbyId = message.args[0];
                    int lobbyId = parse_string_int(strLobbyId);
                    int joinSuccessful = manager.add_member_to_lobby(userId, lobbyId);
                    if (joinSuccessful)
                    {
                        text += "Joined the lobby with the name: " + manager.get_lobby_name(lobbyId) + "\n";
                    }
                    else
                    {
                        text += "Lobby not found\n";
                    }
                }
            }
            else if (message.command_type == CommandType::LeaveLobby)
            {
                std::cout << "LeaveLobby\n";
                if (message.args.size() == 0)
                {
                    int ok = manager.leave_lobby(userId);
                    if (ok)
                    {
                        text += "Successfully left the lobby!\n";
                    }
                    else
                    {
                        text += "Nothing left to do.\n";
                    }
                }
            }
            else if (message.command_type == CommandType::PrivateMessage)
            {
                std::cout << "Private Message\n";
                if (message.args.size() == 2)
                {
                    std::string userIdStr = message.args[0];
                    int targetUser = parse_string_int(userIdStr);
                    std::string textMessage = message.args[1];
                    manager.send_message_to_user(targetUser, textMessage);
                }
            }
            text += ">";
        }
        std::cout << "The sent text: \n";
        std::cout << text << "\n";
        send(client_fd, text.c_str(), text.size(), 0);
    }
    close(client_fd);
}

int main()
{
    std::vector<std::thread> testers;
    Manager manager;

    const char *port = "4040";

    addrinfo hints, *server_address;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port, &hints, &server_address);

    int server_fd = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);
    std::cout << "Server file descriptor: " << server_fd << std::endl;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    if (bind(server_fd, server_address->ai_addr, server_address->ai_addrlen) == -1)
    {
        std::cerr << "Failed to bind socket\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Socket bound to 127.0.0.1:" << port << "\n";

    if (listen(server_fd, 10) == -1)
    {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }

    int client_index = 0;

    std::vector<std::thread> workers;
    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
        if (client_fd == -1)
        {
            std::cout << "Unsuccessful client!" << std::endl;
            continue;
        }
        std::thread t(talk_to_client, client_fd, std::ref(manager));
        workers.emplace_back(std::move(t));
    }
    for (auto &t : workers)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    close(server_fd);

    return 0;
}