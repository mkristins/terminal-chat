#pragma once
#include <iostream>

namespace duo
{
    const std::string DELIMITER = "::";

    enum MessageType
    {
        Unknown,
        Connected,

        SendUsername,
        AckUsername,
        ErrUsername,

        ListLobbies,
        ListUsers,

        CreateLobby,
        JoinLobby,
        LeaveLobby,

        SendMessage,
        RecvMessage,

        SendPrivateMessage,
        RecvPrivateMessage
    };

    std::string type_string(MessageType type);

    MessageType get_type(std::string body);

    class duo_message
    {
        MessageType type;
        std::string content;

    public:
        duo_message() = default;
        duo_message(MessageType type, std::string content);

        std::string dump();

        std::string get_content();
        MessageType get_type();
    };

    duo_message deserialize(std::string s);
}