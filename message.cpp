#include "message.h"
#include <iostream>

/*

class Message for handling TCP/UDP messages.

The messages are in format <MESSAGE_TYPE>|<MESSAGE_CONTENT>

*/

Message::Message(char buffer[])
{
    command_type = CommandType::Unknown;
    std::string current_argument;
    for (int i = 0; i < 1024; i++)
    {
        if (buffer[i] == '\0' || buffer[i] == '\n' || buffer[i] == '\r')
            break;
        if (command_type == CommandType::Unknown)
        {
            if (buffer[i] == ' ')
                continue;
            if (buffer[i] == '/')
            {
                command_type = CommandType::UnknownCommand;
            }
            else
            {
                command_type = CommandType::TextMessage;
                current_argument += buffer[i];
            }
        }
        else if (command_type == CommandType::UnknownCommand)
        {
            if (buffer[i] == ' ')
            {
                if (!current_argument.empty())
                {
                    args.emplace_back(current_argument);
                }
                current_argument.clear();
            }
            else
            {
                current_argument += buffer[i];
            }
        }
        else
        {
            current_argument += buffer[i];
        }
    }
    if (!current_argument.empty())
    {
        args.emplace_back(current_argument);
    }
    if (command_type == CommandType::UnknownCommand)
    {
        if (!args.empty())
        {
            if (args[0] == "help")
            {
                command_type = CommandType::HelpCommand;
            }
            else if (args[0] == "list")
            {
                command_type = CommandType::ListLobbies;
            }
            else if (args[0] == "users")
            {
                command_type = CommandType::ListUsers;
            }
            else if (args[0] == "create")
            {
                command_type = CommandType::CreateLobby;
            }
            else if (args[0] == "join")
            {
                command_type = CommandType::JoinLobby;
            }
            else if (args[0] == "leave")
            {
                command_type = CommandType::LeaveLobby;
            }
            else if (args[0] == "pm")
            {
                command_type = CommandType::PrivateMessage;
            }

            args.erase(args.begin());
        }
    }
}