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
    TextMessage,
    QuitCommand,
    Unknown,
    UnknownCommand
};

class Message
{
public:
    CommandType command_type;
    std::vector<std::string> args;
    Message(char buffer[]);
};