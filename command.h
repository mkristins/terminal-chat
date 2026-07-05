#pragma once

#include <iostream>
#include <vector>
#include <string>
/*

class Message for handling TCP/UDP messages.

The messages are in format <MESSAGE_TYPE>|<MESSAGE_CONTENT>

*/
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
    Unknown
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