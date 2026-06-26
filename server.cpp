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
#include "message.h"

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
    const char *name;
    int lobbyId;

    Member(const char *name) : name(name), lobbyId(0) {}
};

class Lobby
{

public:
    int id;
    const char *name;
    std::vector<int> memberIds;
    int capacity;
    Lobby(int id, const char *name) : id(id), name(name), capacity(10) {}
    Lobby(int id, const char *name, int cap) : id(id), name(name), capacity(cap) {}
};

class Manager
{
    int nextMemberId;
    int nextLobbyId;
    std::vector<Member> memberList;
    std::vector<Lobby> lobbyList;

    std::mutex mutex;

public:
    Manager() : nextMemberId(0), nextLobbyId(0)
    {
    }

    int add_new_member(const char *name)
    {
        mutex.lock();
        int currentId = nextMemberId;
        nextMemberId++;

        memberList.push_back(Member(name));
        mutex.unlock();

        return currentId;
    }

    int add_new_lobby(const char *name)
    {
        mutex.lock();
        int currentId = nextLobbyId;
        nextLobbyId++;

        lobbyList.push_back(Lobby(currentId, name));

        mutex.unlock();
        return currentId;
    }

    std::string produce_lobby_list()
    {
        mutex.lock();
        std::string lobby_list;
        for (auto &m : lobbyList)
        {
            lobby_list += "# ";
            lobby_list += m.name;
            lobby_list += " " + to_str(m.memberIds.size()) + "/" + to_str(m.capacity);
            lobby_list += "\n";
        }
        mutex.unlock();
        return lobby_list;
    }

    std::string produce_user_list()
    {
        mutex.lock();
        std::string user_list;
        for (auto &m : memberList)
        {
            user_list += "# ";
            user_list += m.name;
            user_list += "\n";
        }
        mutex.unlock();
        return user_list;
    }

    int add_member_to_lobby(int memberId, int lobbyId)
    {
        for (auto &m : lobbyList)
        {
            if (m.id == lobbyId)
            {
                if (m.memberIds.size() >= m.capacity)
                {
                    return 0;
                }
                m.memberIds.push_back(memberId);
                return 1;
            }
        }
        return 0;
    }
};

const std::string opening =
    R"(Welcome to ChatServer!
Commands:
  /help             Show available commands
  /list             List all lobbies
  /users            List all users
  /create [name]    Create a lobby
  /join [lobbyCode] Join a lobby
  /leave            Leave current lobby
  /pm [userId]      Private message an user
Enter your username: )";

enum CommunicationState
{
    WaitUsername,
    WaitMessage
};

void talk_to_client(int client_fd, Manager &manager)
{
    char buffer[1024];
    send(client_fd, opening.c_str(), opening.size(), 0);
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
        std::string text;
        if (state == WaitUsername)
        {
            text = "Hi, ";
            for (int x = 0; x < 1024; x++)
            {
                if (buffer[x] == '\0' || buffer[x] == '\n')
                    break;
                name += buffer[x];
            }

            userId = manager.add_new_member(name.c_str());
            text += name + "\n" + ">";
            state = WaitMessage;
        }
        else
        {
            Message message(buffer);
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
            }
            else if (message.command_type == CommandType::LeaveLobby)
            {
                std::cout << "LeaveLobby\n";
            }
            text += ">";
        }
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