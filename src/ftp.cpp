#include "ftp.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <locale>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

using namespace std;

Ftp::Ftp(Client *client)
{
	mClient = client;
}

void Ftp::login()
{
	string username, password;
	char buffer[BUF_SIZE];

	int cmdSock = mClient->getCmdSock();

	receiveMessage(buffer, cmdSock);
	println(buffer);

	print("Name: ");
	cin >> username;
	sendMessage("USER " + username + "\r\n");
	receiveMessage(buffer, cmdSock);
	println(buffer);

    // temporatory solution
    if (username != "anonymous")
    {
    	print("Password: ");
    	cin >> password;
    	sendMessage("PASS " + password + "\r\n");
    	receiveMessage(buffer, cmdSock);
    	println(buffer);
    }

	cin.ignore();
}

void Ftp::disconnect()
{
    int cmdSock = mClient->getCmdSock();
    sendMessage("QUIT\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);
    exit(0);
}

void Ftp::printWorkingDirectory()
{
	int cmdSock = mClient->getCmdSock();

    sendMessage("PWD\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);
}

void Ftp::changeDirectory(string directory)
{
	int cmdSock = mClient->getCmdSock();

    sendMessage("CWD " + directory + "\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);
}

string Ftp::listFiles(string fileSpec, bool isDisplay)
{
    if (mClient->getMode() == ACTIVE)
    {
        bool flag;
        mClient->activeMode(flag);
        if (flag == false)
            return "";
    }
    else
    {
        close(mClient->getDataSock());
        mClient->passiveMode();
    }

    int dataSock = mClient->getDataSock();
    int cmdSock = mClient->getCmdSock();

    sendMessage("LIST " + fileSpec + "\r\n");

    char mes1[BUF_SIZE];
    receiveMessage(mes1, cmdSock);

    char buffer[BUF_SIZE];
    receiveMessage(buffer, dataSock);

    char mes2[BUF_SIZE];
    receiveMessage(mes2, cmdSock);

    println(mes1);
    println(mes2);

    if (isDisplay)
    {
        println(buffer);
    }

    // reset and clean up
    close(dataSock);

    return string(buffer);
}

std::string Ftp::makeDirectory(std::string directory)
{
    int cmdSock = mClient->getCmdSock();

    sendMessage("MKD " + directory + "\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);

    return string(buffer);
}

void Ftp::deleteFile(std::string file)
{
    int cmdSock = mClient->getCmdSock();
    sendMessage("DELE " + file + "\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);
}

void Ftp::deleteFolder(std::string directory)
{

    string folderInfo = listFiles(directory, false);
    stringstream ss(folderInfo);
    string item;
    vector<File> fileList;
    while (getline(ss, item, '\n'))
    {
        Type type;
        if (item[0] == '-')
            type = TYPE_FILE;
        else if (item[0] == 'd')
            type = TYPE_FOLDER;

        string name;
        stringstream itemStream(item);
        while (getline(itemStream, name, ' ')) { }
        if (name[name.length() - 1] == '\r')
        {
            name = name.substr(0, name.length() - 1);
        }

        File file = { type, directory + "/" + name};
        fileList.push_back(file);
    }

    int cmdSock = mClient->getCmdSock();
    for (int i = 0; i < fileList.size(); ++i)
    {
        File file = fileList[i];
        if (file.type == TYPE_FILE)
        {
            sendMessage("DELE " + file.path + "\r\n");
            char buffer[BUF_SIZE];
            receiveMessage(buffer, cmdSock);
            println(buffer);
        }
        else if (file.type == TYPE_FOLDER)
        {
            mClient->passiveMode();
            deleteFolder(file.path);
            sendMessage("RMD " + file.path + "\r\n");
            char buffer[BUF_SIZE];
            receiveMessage(buffer, cmdSock);
            println(buffer);
        }
    }
    sendMessage("RMD " + directory + "\r\n");
    char buffer[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);
    
}

void Ftp::renameFile(std::string oldname, std::string newname)
{
    int cmdSock = mClient->getCmdSock();
    sendMessage("RNFR " + oldname + "\r\n");
    char buffer1[BUF_SIZE];
    receiveMessage(buffer1, cmdSock);
    println(buffer1);
    sendMessage("RNTO " + newname + "\r\n");
    char buffer2[BUF_SIZE];
    receiveMessage(buffer2, cmdSock);
    println(buffer2);
}

void Ftp::downloadFile(string filename, string local)
{
    if (mClient->getMode() == ACTIVE)
    {
        bool flag;
        mClient->activeMode(flag);
        if (flag == false)
            return;
    }
    else
    {
        close(mClient->getDataSock());
        mClient->passiveMode();
    }
    sendMessage("RETR " + filename + "\r\n");

    int dataSock = mClient->getDataSock();
    int cmdSock = mClient->getCmdSock();

    ofstream out;
    out.open(local.c_str(), ofstream::out | ofstream::binary);

    char buffer[BUF_SIZE];
    int n;
    while (isDataAvailable(dataSock))
	{
        n = recv(dataSock, buffer, BUF_SIZE, 0);
        if (n < 0)
        {
            char msg[50];
            sprintf(msg, "recv() error %s", strerror(errno));
            error(msg);
        }
        else if (n == 0)
            break;
        else
        {
            out.write(buffer, n);
            bzero(buffer, BUF_SIZE);
        }
	}

    receiveMessage(buffer, cmdSock);
    println(buffer);

    // reset and clean up
    close(dataSock);
    out.close();
}

void Ftp::downloadFolder(std::string folderName, std::string local)
{
    string folderInfo = listFiles(folderName, false);
    stringstream ss(folderInfo);
    string item;
    vector<File> fileList;
    while (getline(ss, item, '\n'))
    {
        Type type;
        if (item[0] == '-')
            type = TYPE_FILE;
        else if (item[0] == 'd')
            type = TYPE_FOLDER;

        string name;
        stringstream itemStream(item);
        while (getline(itemStream, name, ' ')) { }
        if (name[name.length() - 1] == '\r')
        {
            name = name.substr(0, name.length() - 1);
        }

        File file = { type, folderName + "/" + name};
        fileList.push_back(file);
    }

    mkdir(folderName.c_str(), 0777);

    for (int i = 0; i < fileList.size(); ++i)
    {
        mClient->setMode(PASSIVE);
        close(mClient->getDataSock());
        mClient->passiveMode();

        File file = fileList[i];
        if (file.type == TYPE_FILE)
            downloadFile(file.path, file.path);
        else if (file.type == TYPE_FOLDER)
        {
            downloadFolder(file.path, file.path);
        }
    }
}

void Ftp::uploadFile(std::string filename)
{
    if (mClient->getMode() == ACTIVE)
    {
        bool flag;
        mClient->activeMode(flag);
        if (flag == false)
            return;
    }
    else
    {
        close(mClient->getDataSock());
        mClient->passiveMode();
    }

    int dataSock = mClient->getDataSock();
    int cmdSock = mClient->getCmdSock();

    // ask server to create file
    sendMessage("STOR " + filename + "\r\n");
    char *buffer = new char[BUF_SIZE];
    receiveMessage(buffer, cmdSock);
    println(buffer);

    ifstream in(filename.c_str(), ifstream::in | ifstream::binary);
    // get length of local file:
    in.seekg (0, in.end);
    int length = in.tellg();
    in.seekg (0, in.beg);
    // write remote file
    char *data = new char[length + 1];
    in.read(data, length);
    data[length] = '\0';
    int n = 0;
    while (true)
    {
        n = send(dataSock, data, length - n, 0);
        if (n <= 0)
            break;
        data += n;
    }

    close(dataSock);
}

void Ftp::uploadFolder(string folder)
{
    std::vector<File> files;

    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(folder.c_str());
    while ((ent = readdir(dir)) != NULL)
    {
        const string file_name = ent->d_name;
        const string full_file_name = folder + "/" + file_name;

        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        Type type = TYPE_FILE;
        if (is_directory)
            type = TYPE_FOLDER;

        File file = { type, full_file_name };

        files.push_back(file);
    }
    closedir(dir);

    makeDirectory(folder);

    for (int i = 0; i < files.size(); ++i)
    {
        mClient->setMode(PASSIVE);
        mClient->passiveMode();
        File file = files[i];
        if (file.type == TYPE_FILE)
            uploadFile(file.path);
        else if (file.type == TYPE_FOLDER)
            uploadFolder(file.path);
    }
}

int Ftp::sendMessage(string message)
{
	int cmdSock = mClient->getCmdSock();
	return send(cmdSock, message.c_str(), message.length(), 0);
}

void Ftp::receiveMessage(char *buffer, int sock)
{
	bzero(buffer, BUF_SIZE);

    int n = 0;
	while (isDataAvailable(sock))
	{
        n = recv(sock, buffer + n, BUF_SIZE - n, 0);
        if (n < 0)
        {
            char msg[50];
            sprintf(msg, "recv() error %s", strerror(errno));
            error(msg);
        }
        else if (n == 0)
            break;
	}
}

bool Ftp::isDataAvailable(int fd)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    /* Wait up to five seconds. */
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    retval = select(fd + 1, &rfds, NULL, NULL, &tv);
    /* Don’t rely on the value of tv now! */

    if (retval == -1)
        perror("select()");
    else if (retval)
        return true;
        /* FD_ISSET(0, &rfds) will be true. */
    else
        return false;
    return 0;
}
