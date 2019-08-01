#include "client.h"
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <locale>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char **argv)
{
	if (argc < 2)
		error("Missing arguments\n");

	char *hostName = argv[1];

	Client client;
	client.init(hostName, PORT);

	// default active mode
	client.setMode(ACTIVE);

	client.handleConn();

	pthread_exit(NULL);
	return 0;
}

bool Client::init(const string serverName, const int serverPort)
{

	struct sockaddr_in serverAddr;
	struct hostent *serverInfo;

	memset(&serverAddr, 0, sizeof(serverAddr));

	if((serverInfo = gethostbyname(serverName.c_str())) == NULL)
	{
		error("Invalid FTP server address!\n");
	}

	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)serverInfo->h_addr)));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);

	// create socket
	if((mCmdSock = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
	{
		error("Can not create socket!\n");
	}

	// connect on socket
	if((connect(mCmdSock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
	{
		close(mCmdSock);
		error("Connection failed!\n");
	}

    mMode = ACTIVE;
    mFtp = new Ftp(this);

	return true;
}

void Client::activeMode(bool &flag)
{
	string ip = getIpAddress();
	stringstream ss(ip);
	vector<string> elements;
	string number;
	while (getline(ss, number, '.'))
	{
		elements.push_back(number);
	}
	string activeCommand = "PORT " + elements[0] + "," + elements[1] + "," + elements[2] + "," + elements[3] + ",1,1865\r\n";

	mFtp->sendMessage(activeCommand);
	char buffer[BUF_SIZE];
	mFtp->receiveMessage(buffer, mCmdSock);
	println(buffer);
	// error
	if (strncmp("500", buffer, 3) == 0)
	{
		flag = false;
        return;
	}

	struct sockaddr_in servAddr;
	bzero((char*)&servAddr, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(ACTIVE_PORT);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		error("Can't initialize Socket!\n");

	if (bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
		error("Error binding Socket!\n");

	listen(sock, 5);
	mMode = ACTIVE;

	// accept connection
	struct sockaddr_in cliAddr;
	bzero((char*)&cliAddr, sizeof(cliAddr));
	int cliLen = sizeof(cliAddr);

	mDataSock = accept(sock, (struct sockaddr*)&cliAddr, (socklen_t*)&cliLen);

	if (mDataSock < 0)
	{
		close(sock);
		error("Problem in accepting connection");
	}
}

void Client::passiveMode()
{
    mFtp->sendMessage("PASV\r\n");
    char buffer[BUF_SIZE];
    mFtp->receiveMessage(buffer, mCmdSock);
    println(buffer);

    // get full address: a1,a2,a3,a4,p1,p2
    string respond(buffer);
    stringstream ss(respond);
    string address, token;
    while (getline(ss, token, '(') && getline(ss, token, ')'))
    {
        address = token;
    }
    ss.str("");

    // get ip address & port number
    string ip;
    stringstream ass(address);
    string number;
    int count = 0, port = 0;
    while (getline(ass, number, ','))
    {
        count++;
        if (count <= 4)
        {
            ip = ip + number + ".";
        }
        else
        {
            count == 5
                ? port += atoi(number.c_str()) * 256
                : port += atoi(number.c_str());
        }
    }
    ip = ip.substr(0, ip.length() - 1);

    // connect to data channel
    struct sockaddr_in serverAddr;
	struct hostent *serverInfo;

	memset(&serverAddr, 0, sizeof(serverAddr));

	if((serverInfo = gethostbyname(ip.c_str())) == NULL)
	{
		error("Invalid FTP passive mode address!\n");
	}

	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)serverInfo->h_addr)));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	// create socket
	if((mDataSock = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
	{
		error("Can not create data socket!\n");
	}

	// connect on socket
	if((connect(mDataSock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
	{
		close(mDataSock);
		error("Connection failed!\n");
	}

	mMode = PASSIVE;
}

void Client::handleConn()
{
	mFtp->login();

	bool isExit = false;
	while (!isExit)
	{
        // read command from user
        string input;
        cout << "hkt-mFtp> ";
        getline(cin, input);

        // get main command
        string token;
        vector<string> commandList;
        stringstream ss(input);
        while (getline(ss, token, ' '))
        {
            commandList.push_back(token);
        }
        string command = commandList[0];

        // convert to uppercase
        locale loc;
        for (string::size_type i = 0; i < command.length(); ++i)
        {
            command[i] = toupper(command[i], loc);
        }

        if (command == "PWD")
            mFtp->printWorkingDirectory();
        else if (command == "CD")
        {
            string destination = commandList[1];
            mFtp->changeDirectory(destination);
        }
        else if (command == "LS")
        {
        	string fileSpec = commandList.size() == 2 ? commandList[1]
        											  : ".";

            string result = mFtp->listFiles(fileSpec, true);
        }
        else if (command == "MKDIR")
        {
        	string directory = commandList[1];
        	string result = mFtp->makeDirectory(directory);
        	cout << result << endl;
        }
        else if (command == "PASS")
            passiveMode();
        else if (command == "GET")
        {
            string remoteFile = commandList[1];
            string localFile =
                commandList.size() == 3 ? commandList[2]
                                        : remoteFile;
            mFtp->downloadFile(remoteFile, localFile);
        }
        else if (command == "GETDIR")
        {
        	string remoteFolder = commandList[1];
            string localFolder =
                commandList.size() == 3 ? commandList[2]
                                        : remoteFolder;
        	mFtp->downloadFolder(remoteFolder, localFolder);
        }
        else if (command == "PUT")
        {
        	string filename = commandList[1];
        	mFtp->uploadFile(filename);
        }
        else if (command == "PUTDIR")
        {
        	string folder = commandList[1];
        	mFtp->uploadFolder(folder);
        }
	else if (command == "DELETE" || command == "RM")
	{
		string filename = commandList[1];
		mFtp->deleteFile(filename);
	}
	else if (command == "RMD")
	{
		string folder = commandList[1];
		mFtp->deleteFolder(folder);
	}
	else if (command == "RENAME")
	{
		string oldname = commandList[1];
		string newname = commandList[2];
		mFtp->renameFile(oldname, newname);
	}
	else if (command == "EXIT")
		mFtp->disconnect();
    	else
            println("Invalid command!");
	}
}

string Client::getIpAddress()
{
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&ifr.ifr_addr;
	struct in_addr ipAddr = pV4Addr->sin_addr;

	char str[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );

	return string(str);
}
