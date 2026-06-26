#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    std::cout << "Hello, client!\n";
    addrinfo hints;
    addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    int status = getaddrinfo("localhost", "4040", &hints, &result);
    std::cout << status << " !\n";
    for (addrinfo *current = result; current != nullptr; current = current->ai_next)
    {
        std::cout << "One of the results: ";

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

        std::cout << ipstr << " --- ";
        if (inet_ntop(current->ai_family, addr, ipstr, sizeof ipstr))
        {
            std::cout << " " << ipver << " " << ipstr << "\n";
        }
    }
    int conn_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    std::cout << conn_fd << "\n";

    connect(conn_fd, result->ai_addr, result->ai_addrlen);
    close(conn_fd);
    freeaddrinfo(result);
    return 0;
}