#include <iostream>
#define SECURITY_WIN32

#include <winsock2.h>  
#include <schannel.h>
#include <security.h>
#include <sspi.h>

#define SECURITY_WIN32
#define IO_BUFFER_SIZE  0x10000
#define DLL_NAME TEXT("Secur32.dll")
#define NT4_DLL_NAME TEXT("Security.dll")

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>

#pragma comment(lib, "WSock32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "MSVCRTD.lib")
#pragma comment(lib, "Secur32.lib")

#include <string>
#include <thread>
#include <fstream>
#include <ws2tcpip.h> // For getaddrinfo
#include <cstdlib> // for the system function
#include <vector>


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")  // Link against the Winsock library

// Define the server's IPv4 address and port to listen on
#define SERVER_IP "192.168.1.129" // Use your desired IPv4 address
#define SERVER_PORT 27022
//23.241.31.105, 27022
//http://192.168.1.129:27022

int numberOfClientsJoined = 0;
void print(std::string x) {
	std::cout << x << std::endl;
}

class Client {
public:
	SOCKET clientSocket;
	int id = 0;
	std::string ip;
	int port;
	// Function to get the client's IP address as a string
	void updateClientIpAndPort() {
		sockaddr_in clientAddr; // Use sockaddr_in for IPv4
		int addrLen = sizeof(clientAddr);
		getpeername(clientSocket, (struct sockaddr*)&clientAddr, &addrLen);

		char clientIP[INET_ADDRSTRLEN]; // Use INET_ADDRSTRLEN for IPv4 addresses
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		int clientPort = ntohs(clientAddr.sin_port); // For IPv4
		ip = clientIP;
		port = clientPort;
	}
};
// Function to send an HTTP response with the content of a file
void sendFile(SOCKET clientSocket, const std::string& filePath) {
	print("sending file to space");
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(fileSize) + "\r\n\r\n";

		send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);

		char buffer[1024];
		while (!file.eof()) {
			file.read(buffer, sizeof(buffer));
			send(clientSocket, buffer, file.gcount(), 0);
		}

		file.close();
	}
	else {
		// Handle file not found
		std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
		send(clientSocket, notFoundResponse.c_str(), notFoundResponse.size(), 0);
	}
}

// Handle HTTP GET requests
void handleHttpGetRequest(SOCKET clientSocket) {
	print("handleHTTPgetRequest");
	// Open and read the HTML file (index.html)
	std::ifstream htmlFile("C:/Users/graha/PycharmProjects/chess/HTTPServerforClientChessGame/HTTPServerforClientChessGame/index.html");
	if (!htmlFile.is_open()) {
		// If the file doesn't exist, respond with a 404 error
		std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
		send(clientSocket, notFoundResponse.c_str(), notFoundResponse.size(), 0);
		closesocket(clientSocket);
		print("html file not found");
		return;
	}
	else {
		print("html file found");
	}

	// Read the content of the HTML file
	std::string htmlContent((std::istreambuf_iterator<char>(htmlFile)), std::istreambuf_iterator<char>());

	// Construct the HTTP response header
	std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(htmlContent.size()) + "\r\n\r\n";

	if (send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0) == SOCKET_ERROR) {
		std::cerr << "Error sending response header: " << WSAGetLastError() << std::endl;
	}
	else {
		print("sent httpReponse");
	}
	if (send(clientSocket, htmlContent.c_str(), htmlContent.size(), 0) == SOCKET_ERROR) {
		std::cerr << "Error sending response header: " << WSAGetLastError() << std::endl;
	}
	else {
		print("sent httpContent");
	}
}

std::vector<Client> clients; // Vector to store client instances

void handleClient(Client* client) {
	std::cout << "Handling client" << std::endl;
	
	handleHttpGetRequest(client->clientSocket);
	//sendFile(client->clientSocket, "images/board.png");
	//clients.push_back(*client);
	while (true) {
		// Receive data from the client
		char buffer[1024];
		int bytesReceived = recv(client->clientSocket, buffer, sizeof(buffer), 0);

		if (bytesReceived == SOCKET_ERROR) {
			int errorCode = WSAGetLastError();
			if (errorCode == WSAEWOULDBLOCK) {
				continue;
			}
			else {
				// Handle other socket errors
				std::cerr << "Socket error: " << errorCode << std::endl;
				break; // Exit the loop on error
			}
		}
		else if (bytesReceived == 0) {
			// Client has disconnected
			std::cout << "Client disconnected." << std::endl;
			numberOfClientsJoined -=1;
			break; // Exit the loop when the client disconnects
		}
	}

	numberOfClientsJoined -= 1;
	//delete client;
	closesocket(client->clientSocket);
	std::cout << "Number of Clients = " << numberOfClientsJoined << std::endl; //end thread
	return;
}

SOCKET setupServerSocket() {
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Use AF_INET for IPv4
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed." << std::endl;
	}
	else {
		std::cout << "Socket Created." << std::endl;
	}
	sockaddr_in serverAddr; // Use sockaddr_in for IPv4
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT); // Choose a port number
	if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
		std::cerr << "Invalid address or address family" << std::endl;
		closesocket(serverSocket);
		WSACleanup();
	}
	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Binding failed." << std::endl;
		closesocket(serverSocket);
	}
	else {
		std::cout << "Binding Completed" << std::endl;
	}
	return serverSocket;
}
SOCKET acceptClient(SOCKET serverSocket) {
	// Accept incoming client connection
	SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Accepting connection failed." << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return NULL;
	}
	else {
		return clientSocket;
	}
}

// Function to check if a client is already in the list
bool isAlreadyJoined(const std::string ip) {
	// Iterate through the list of clients
	for (const Client& x : clients) {
		if (ip == x.ip) {
			return true; // Client with the same IP address found
		}
	}
	// Client not found in the list
	return false;
}
void run_socket() {
	std::cout << "Running socket";
	bool serverRunning = true;
	SOCKET serverSocket;
	SOCKET clientSocket;
	serverSocket = setupServerSocket();
	std::string lastIP;
	while (serverRunning) {
		
		std::cout << "Listening for Clients..." << std::endl;
		if (listen(serverSocket, 1) == SOCKET_ERROR) {
			std::cerr << "Listening failed." << std::endl;
			closesocket(serverSocket);
		}
		else {
			clientSocket = acceptClient(serverSocket);
			Client* client = new Client();
			client->clientSocket = clientSocket;
			client->updateClientIpAndPort();
			numberOfClientsJoined++;
			client->id = numberOfClientsJoined - 1;

			if (clientSocket != INVALID_SOCKET && !isAlreadyJoined(client->ip)) {
				print(client->ip);
				clients.push_back(*client);
				std::thread client_thread(handleClient, client);
				client_thread.detach(); // Wait for the thread to finish
			}
		}
		Sleep(1000);
	}

	// Close the server socket and perform cleanup
	closesocket(serverSocket);
	WSACleanup();
}
void getLocalIpAdress() {
	char hostName[NI_MAXHOST];
	if (gethostname(hostName, NI_MAXHOST) == 0) {
		struct addrinfo* result = nullptr;
		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET; // Request IPv4 address

		if (getaddrinfo(hostName, nullptr, &hints, &result) == 0) {
			struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
			char ipBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(addr->sin_addr), ipBuffer, INET_ADDRSTRLEN);
			std::cout << "Local IPv4 Address: " << ipBuffer << " on port: " << SERVER_PORT << std::endl;
			freeaddrinfo(result);
		}
		else {
			std::cerr << "getaddrinfo failed." << std::endl;
		}
	}
	else {
		std::cerr << "gethostname failed." << std::endl;
	}
}

int main() {
	std::cout << "Running HTTP Server for Chess Game";
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup Failed." << std::endl;
	}
	else {
		std::cout << "WSAStartup Success." << std::endl;
	}

	getLocalIpAdress();
	run_socket();

	return 0;
}
