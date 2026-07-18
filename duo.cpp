#include "duo.h"
#include <iostream>

namespace duo
{
    std::string type_string(MessageType type)
    {
        switch (type)
        {
        case MessageType::Unknown:
            return "Unknown";
        case MessageType::Connected:
            return "Connected";
        case MessageType::SendUsername:
            return "SendUsername";
        case MessageType::AckUsername:
            return "AckUsername";
        case MessageType::ErrUsername:
            return "ErrUsername";
        case MessageType::ListLobbies:
            return "ListLobbies";
        case MessageType::ListUsers:
            return "ListUsers";
        case MessageType::CreateLobby:
            return "CreateLobby";
        case MessageType::JoinLobby:
            return "JoinLobby";
        case MessageType::LeaveLobby:
            return "LeaveLobby";
        case MessageType::SendMessage:
            return "SendMessage";
        case MessageType::RecvMessage:
            return "RecvMessage";
        case MessageType::SendPrivateMessage:
            return "SendPrivateMessage";
        case MessageType::RecvPrivateMessage:
            return "RecvPrivateMessage";
        }
        throw std::invalid_argument("Unknown MessageType");
    }

    MessageType get_type(std::string body)
    {
        if (body == "Unknown")
        {
            return MessageType::Unknown;
        }
        else if (body == "Connected")
        {
            return MessageType::Connected;
        }
        else if (body == "SendUsername")
        {
            return MessageType::SendUsername;
        }
        else if (body == "AckUsername")
        {
            return MessageType::AckUsername;
        }
        else if (body == "ErrUsername")
        {
            return MessageType::ErrUsername;
        }
        else if (body == "ListLobbies")
        {
            return MessageType::ListLobbies;
        }
        else if (body == "ListUsers")
        {
            return MessageType::ListUsers;
        }
        else if (body == "CreateLobby")
        {
            return MessageType::CreateLobby;
        }
        else if (body == "JoinLobby")
        {
            return MessageType::JoinLobby;
        }
        else if (body == "LeaveLobby")
        {
            return MessageType::LeaveLobby;
        }
        else if (body == "SendMessage")
        {
            return MessageType::SendMessage;
        }
        else if (body == "RecvMessage")
        {
            return MessageType::RecvMessage;
        }
        else if (body == "SendPrivateMessage")
        {
            return MessageType::SendPrivateMessage;
        }
        else if (body == "RecvPrivateMessage")
        {
            return MessageType::RecvPrivateMessage;
        }
        throw std::invalid_argument("Unrecognizable type");
    }

    duo_message::duo_message(MessageType type, std::string content) : type(type), content(content) {}

    std::string duo_message::dump()
    {
        return "[" + type_string(type) + DELIMITER + content + "]\n";
    }

    std::string duo_message::get_content()
    {
        return content;
    }

    MessageType duo_message::get_type()
    {
        return type;
    }

    duo_message deserialize(std::string s)
    {
        if (s.empty())
        {
            throw std::invalid_argument("Empty string received");
        }
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n'))
            s.pop_back();

        if (s[0] != '[' || s.back() != ']')
        {
            throw std::invalid_argument("Invalid start/end sequence");
        }
        s = s.substr(1, s.size() - 2);
        size_t position = s.find(DELIMITER);
        if (position == std::string::npos)
        {
            throw std::invalid_argument("Delimiter not found");
        }
        std::string type = s.substr(0, position);
        std::string content = s.substr(position + DELIMITER.size());
        return duo_message(get_type(type), content);
    }

}