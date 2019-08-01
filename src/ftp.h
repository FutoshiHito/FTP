#ifndef __FTP_H__
#define __FTP_H__

#include "client.h"
#include <string>

enum Type
{
	TYPE_FILE,
	TYPE_FOLDER
};

struct File
{
	Type type;
	std::string path;
};

class Client;

class Ftp
{
public:
	Ftp(Client *client);

	/**
	 * Đăng nhập vào FTP Server
	 * Hiển thị thông tin đăng nhập thành công hoặc thất bại 
	 */
	void login();
	
	/**
	 * Ngắt kết nối đến FTP Server và thoát chương trình 
	 */
	void disconnect();

	/**
	 * Hiển thị thư mục hiện tại trên FTP Server 
	 */
	void printWorkingDirectory();
	
	/**
	 * Chuyển đến thư mục mới
	 * @param directory Tên thư mục mới 
	 */
	void changeDirectory(std::string directory);
	
	/**
	 * Liệt kê danh sách file và thư mục trong thư mục hiện tại
	 * @param fileSpec Tên thư mục cần liệt kê
	 * @param isDisplay Có hiển thị kết quả lên Console hay không 
	 * @return Danh sách file và thư mục con trong thư mục hiện tại 
	 */
	std::string listFiles(std::string fileSpec, bool isDisplay);
	
	/**
	 * Tạo thư mục mới
	 * @param directory Tên thư mục mới
	 * @return Tên thư mục vừa được tạo
	 */
	std::string makeDirectory(std::string directory);
	
	/**
	 * Xóa file
	 * @param file Tên file cần xóa 
	 */
	void deleteFile(std::string file);
	
	/**
	 * Xóa thư mục
	 * @param directory Tên thư mục cần xóa
	 */
	void deleteFolder(std::string directory);
	
	/**
	 * Đổi tên file (thư mục)
	 * @param oldname Tên file (thư mục) cũ
	 * @param oldname Tên file (thư mục) mới
	 */
	void renameFile(std::string oldname, std::string newname);
	
	/**
	 * Tải xuống file
	 * @param filename Tên file cần tải (phải trùng với tên file trên FTP Server)
	 * @param local Tên file sau khi tải về máy
	 */
	void downloadFile(std::string filename, std::string local);
	
	/**
	 * Tải xuống thư mục
	 * @param folderName Tên thư mục trên FTP Server cần tải xuống
	 * @param local Tên thư mục sau khi tải về máy
	 */
	void downloadFolder(std::string folderName, std::string local);

	/**
	 * Tải lên file
	 * @param filename Tên file cần tải lên FTP Server
	 */
	void uploadFile(std::string filename);
	
	/**
	 * Tải lên thư mục
	 * @param folder Tên thư mục cần tải lên
	 */
	void uploadFolder(std::string folder);

	/**
	 * Gửi lệnh đến FTP Server thông qua Command Channel
	 * @param message Lệnh cần gửi
	 * @return Số lượng ký tự đã gửi nếu thành công, -1 nếu thất bại
	 */
	int sendMessage(std::string message);
	
	/**
	 * Nhận dữ liệu gửi từ FTP Server thông qua Command hoặc Data Channel
	 * @param buffer Bộ đệm lưu dữ liệu gửi từ Server
	 * @param sock Socket nhận dữ liệu, có thể là command hoặc data socket
	 */
	void receiveMessage(char *buffer, int sock);
private:
	bool isDataAvailable(int fd);

	Client *mClient;
};

#endif
