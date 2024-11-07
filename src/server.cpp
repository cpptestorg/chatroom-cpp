#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

// Define the client struct.
struct terminal
{
	unsigned id;
	std::string name;
	int socket;
	std::thread th;
};

// Define the constants used by the server.
constexpr unsigned maxMsgLength {200U};
constexpr unsigned colorCount {6U};
constexpr unsigned maxClients {5U};

std::array<terminal, maxClients> clients;
std::array<std::string, colorCount> colors {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
std::string resetColor {"\033[0m"};

std::mutex coutMutex, clientMutex;

// Function forward declarations (make a separate file later).
const std::string& getColor(int code);
void setName(int id, const std::string& name);
void sharedPrint(const std::string& string, bool endLine = true);
void broadcastMessage(const std::string& message, int senderId);
void broadcastMessage(int num, int senderId);
void endConnection(int id);
void handleClient(int clientSocket, int id);

// Main program entry point.
int main()
{
	int server_socket;
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(10000);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&server.sin_zero, size_t(0));

	if ((bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in))) == -1)
	{
		perror("bind error: ");
		exit(-1);
	}

	if ((listen(server_socket, 8)) == -1)
	{
		perror("listen error: ");
		exit(-1);
	}

	struct sockaddr_in client;
	int client_socket;
	unsigned int len = sizeof(sockaddr_in);

	std::cout << colors.back() << "\n\t  ====== Welcome to the chat-room ======   " << resetColor << std::endl;
	unsigned seed {};

	while (true)
	{
		if ((client_socket = accept(server_socket, (struct sockaddr*)&client, &len)) == -1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		std::thread t(handleClient, client_socket, seed);
		std::lock_guard<std::mutex> guard(clientMutex);

		clients.at(seed) = {seed, "Anonymous", client_socket, std::move(t)};
	}

	for (const auto& client : clients)
		if (client.th.joinable())
			client.th.joinable();

	close(server_socket);
	return 0;
}

const std::string& getColor(int code)
{
	return colors.at(code % colorCount);
}

// Set name of client, because we use an array, we can use the index as the id and avoid a loop.
void setName(int id, const std::string& name)
{
	if (clients.at(id).socket != 0)
		clients.at(id).name = name;
}

// For synchronisation of cout statements
void sharedPrint(const std::string& str, bool endLine)
{
	std::lock_guard<std::mutex> guard(coutMutex);
	std::cout << str;
	if (endLine)
		std::cout << std::endl;
}

// Broadcast message to all clients except the sender
void broadcastMessage(const std::string& message, int senderId)
{
	char temp[maxMsgLength];
	strcpy(temp, message.c_str());

	for (int i {}; i < maxClients; i++)
		if (clients.at(i).socket != 0 && clients.at(i).id != senderId)
			send(clients.at(i).socket, temp, sizeof(temp), 0);
}

// Broadcast a number to all clients except the sender
void broadcastMessage(int num, int senderId)
{
	for (int i {}; i < maxClients; i++)
		if (clients.at(i).socket != 0 && clients.at(i).id != senderId)
			send(clients.at(i).socket, &num, sizeof(num), 0);
}

void endConnection(int id)
{
	std::lock_guard<std::mutex> guard(clientMutex);
	clients.at(id).th.detach();
	clients.at(id) = {};
	close(clients.at(id).socket);
}

void handleClient(int clientSocket, int id)
{
	char name[maxMsgLength], str[maxMsgLength];
	recv(clientSocket, name, sizeof(name), 0);
	setName(id, name);

	// Display welcome message
	std::string welcome_message = std::string(name) + std::string(" has joined");
	broadcastMessage("#NULL", id);
	broadcastMessage(id, id);
	broadcastMessage(welcome_message, id);
	sharedPrint(getColor(id) + welcome_message + resetColor);

	while (true)
	{
		int bytes_received = recv(clientSocket, str, sizeof(str), 0);
		if (bytes_received <= 0)
			return;

		if (strcmp(str, "#exit") == 0)
		{
			// Display leaving message
			std::string message = std::string(name) + std::string(" has left");
			broadcastMessage("#NULL", id);
			broadcastMessage(id, id);
			broadcastMessage(message, id);
			sharedPrint(getColor(id) + message + resetColor);
			endConnection(id);
			return;
		}
		broadcastMessage(std::string(name), id);
		broadcastMessage(id, id);
		broadcastMessage(std::string(str), id);
		sharedPrint(getColor(id) + name + " : " + resetColor + str);
	}
}
