#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "utils.h"
#include "ftp.h"

const int PORT = 21;
const int ACTIVE_PORT = 2121;
const int BUF_SIZE = BUFSIZ;

enum Mode
{
    ACTIVE,
    PASSIVE,
};

class Ftp;

class Client
{
public:
	inline int getCmdSock() { return mCmdSock; }
	inline int getDataSock() { return mDataSock; }
	inline void setDataSock(int sock) { mDataSock = sock; }
	inline Mode getMode() { return mMode; }
	inline void setMode(Mode mode) { mMode = mode; }

	/**
	 * Khởi tạo các thành phần cần thiết để kết nối và nhận dữ liệu từ FTP Server: socket, ftp handler object, transfer mode...
	 */
	bool init(const std::string serverName, const int serverPort);
	
	/**
	 * Nhận lệnh nhập từ người dùng, và thực hiện các chức năng tương ứng
	 */
	void handleConn();

	/**
	 * Khởi tạo ACTIVE mode cho data channel, Client mở cổng chờ Server kết nối
	 */
	void activeMode(bool &flag);
	
	/**
	 * Khởi tạo PASSIVE mode cho data channel, Client kết nối đến Server dựa vào thông tin Server gửi đến
	 */
	void passiveMode();

private:
	std::string getIpAddress();		// get client ip v4 address

	int mCmdSock;
	int mDataSock;

	Mode mMode;

	Ftp *mFtp;
};

#endif
