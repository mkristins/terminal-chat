#include "command.h"
#include <iostream>

/*

class Command for handling messages and commands
*/

void Command::initialize(std::string str)
{
    command_type = CommandType::Unknown;
    std::string current_argument;
    for (char cur_char : str)
    {
        if (command_type == CommandType::Unknown)
        {
            if (cur_char == ' ')
                continue;
            if (cur_char == '/')
            {
                command_type = CommandType::Unknown;
            }
            else
            {
                command_type = CommandType::Message;
                current_argument += cur_char;
            }
        }
        else if (command_type == CommandType::Unknown)
        {
            if (cur_char == ' ')
            {
                if (!current_argument.empty())
                {
                    args.emplace_back(current_argument);
                }
                current_argument.clear();
            }
            else
            {
                current_argument += cur_char;
            }
        }
        else
        {
            current_argument += cur_char;
        }
    }
    if (!current_argument.empty())
    {
        args.emplace_back(current_argument);
    }
    if (command_type == CommandType::Unknown)
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

Command::Command(std::string str)
{
    initialize(str);
}

Command::Command(char buffer[])
{
    std::string filtered_str = "";
    for (int i = 0; i < 1024; i++)
    {
        if (buffer[i] == '\0' || buffer[i] == '\n' || buffer[i] == '\r')
            break;
        filtered_str += buffer[i];
    }
    initialize(filtered_str);
}