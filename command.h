#pragma once

#include <iostream>
#include <vector>
#include <string>

enum CommandType
{
    HelpCommand,
    ListLobbies,
    ListUsers,
    CreateLobby,
    JoinLobby,
    LeaveLobby,
    PrivateMessage,
    QuitCommand,
    Message,
    Unknown,
    UnknownCommand
};

class Command
{
    void initialize(std::string str);

public:
    CommandType command_type;
    std::vector<std::string> args;
    Command(char buffer[]);
    Command(std::string str);
};