#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "protocol.pb.h"

using namespace std;

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port>" << endl;
        return 1;
    }

    // Parse command line arguments
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return 1;
    }

    // Initialize server address
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton failed");
        return 1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("connection failed");
        return 1;
    }

    // Prepare request
    chat::ClientPetition request;
    // Fill in request fields here...

    // Serialize request
    string serialized_request;
    if (!request.SerializeToString(&serialized_request)) {
        cerr << "Failed to serialize request." << endl;
        return 1;
    }

    // Send request to server
    if (send(sockfd, serialized_request.c_str(), serialized_request.size(), 0) == -1) {
        perror("send failed");
        return 1;
    }

    // Receive response from server
    char buffer[8500];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        perror("recv failed");
        return 1;
    }

    // Parse response
    chat::ServerResponse response;
    if (!response.ParseFromString(buffer)) {
        cerr << "Failed to parse response." << endl;
        return 1;
    }

    // Display response
    cout << "Response received:" << endl;
    cout << "Option: " << response.option() << endl;
    cout << "Code: " << response.code() << endl;
    cout << "Server Message: " << response.servermessage() << endl;

    // Close socket
    close(sockfd);

    return 0;
}
