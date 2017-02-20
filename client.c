#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include <ctype.h>

#include"cJSON.h"

#define BUFFER_SIZE 1024
#define USER_SIZE 50

typedef struct User{
	char *name;
	char *pwd;
	int login;
}User;
User *user;
char *login_id;

void compileMethod();
void getMsgFromServer(char *msg);
void sendMsgToOthers(char *input_msg, int server_sock_fd);
void sendMsgToLogin(char *input_msg, int server_sock_fd);
void sendMsgToRegister(char *input_msg, int server_sock_fd);
void loadAllOnlineUsers(char *input_msg, int server_sock_fd);

void delEnter(char *info);

int main(int argc, const char * argv[])
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(11332);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(&(server_addr.sin_zero), 8);

    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_fd == -1)
    {
		printf("❌ 系统错误 创建 Socket 失败！\n");
		return 1;
    }
	char recv_msg[BUFFER_SIZE];
	char input_msg[BUFFER_SIZE];

    if(connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
    {
		// system("CLEAR");  // 清屏 Linux
		fd_set client_fd_set;
		struct timeval tv;
		compileMethod();  // 初始化用户信息
		while(1)
		{
			tv.tv_sec = 20;
			tv.tv_usec = 0;
			FD_ZERO(&client_fd_set);
			FD_SET(STDIN_FILENO, &client_fd_set);
			FD_SET(server_sock_fd, &client_fd_set);

			select(server_sock_fd + 1, &client_fd_set, NULL, NULL, &tv);
			if(FD_ISSET(STDIN_FILENO, &client_fd_set))
			{
				bzero(input_msg, BUFFER_SIZE);

				char *getKey = (char *) malloc (BUFFER_SIZE);
				fgets(getKey, BUFFER_SIZE, stdin);
				// printf("准备发送消息 : %s\n", getKey);
				if( strncasecmp(getKey, "send", 4) == 0 ){
					sendMsgToOthers( input_msg, server_sock_fd );
				} else if ( strncasecmp(getKey, "list", 4) == 0 ){
					loadAllOnlineUsers( input_msg, server_sock_fd );
				} else if ( strncasecmp(getKey, "login", 5) == 0 ){
					sendMsgToLogin( input_msg, server_sock_fd );
				} else if ( strncasecmp(getKey, "register", 8) == 0 ){
					sendMsgToRegister( input_msg, server_sock_fd );
				} else {
					printf("▼ 请输入正确的指令 : list[列表]  send[发送]  login[登录]  register[注册]\n");
				}
			}
			if(FD_ISSET(server_sock_fd, &client_fd_set))
			{
				bzero(recv_msg, BUFFER_SIZE);
				long byte_num = recv(server_sock_fd, recv_msg, BUFFER_SIZE, 0);
				if(byte_num > 0)
				{
					if(byte_num > BUFFER_SIZE)
					{
						byte_num = BUFFER_SIZE;
					}
					recv_msg[byte_num] = '\0';
					// 接收服务器发来的消息，并处理
					getMsgFromServer(recv_msg);
				}
				else if(byte_num < 0)
				{
					printf("▼ 系统提示 接收消息出错!\n");
				}
				else
				{
					printf("▼ 系统提示 服务器端退出! 端开连接!\n");
					exit(0);
				}
			}
	    }
    } else {
		printf("❌ 系统错误 连接服务器失败！\n");
	}
    return 0;
}

void compileMethod(){
	user = (User *) malloc (sizeof(User));
	user->name = (char *) malloc (USER_SIZE);
	user->pwd = (char *) malloc (USER_SIZE);
	bzero(user->name, USER_SIZE);
	bzero(user->name, USER_SIZE);
	user->login = 0;
	login_id = (char *) malloc (USER_SIZE);
	printf("======================================================================\n");
	printf("\t欢迎使用即时聊天系统\n");
	printf("\t指令说明:\n");
	printf("\t\tlist[好友列表]\t\tsend[发送消息]\n");
	printf("\t\tlogin[用户登录]\t\tregister[用户注册]\n");
	printf("======================================================================\n");
}

/**
 * 处理从服务器接收的信息
 */
void getMsgFromServer(char *msg){
	// cJSON_Minify(msg);
	cJSON *json = cJSON_Parse(msg);
	if (!json) {
		printf("▼ 系统提示 从服务器接收的数据不是标准格式, 已忽略\n");
	} else {
		char *type = cJSON_GetObjectItem(json, "type")->valuestring;
		char *from = cJSON_GetObjectItem(json, "from")->valuestring;
		char *msg = cJSON_GetObjectItem(json, "msg")->valuestring;
		char *time = cJSON_GetObjectItem(json, "time")->valuestring;
		if( strcmp (type, "x") == 0 ){
			printf("● %s 在线列表 > [ %s ]\n", time, msg);
		} else {
			printf("● %s 收到消息 来自 [ %s ] > %s\n", time, from, msg);
		}
		if( strcmp(type, "8") == 0 ){
			strcpy(user->name, login_id);
			strcpy(user->pwd, msg);
			user->login = 1;
		}

	}
}

/**
 * 发送消息给登录的其他用户
 */
void sendMsgToOthers(char *input_msg, int server_sock_fd){
	if(0 == user->login){
		printf("▼ 系统提示 当前终端未登录，不能发送消息\n");
		return ;
	}
	printf("> 选择在线好友: ");
	char *name = (char *) malloc (USER_SIZE);
	bzero(name, USER_SIZE);
	fgets(name, USER_SIZE, stdin);
	delEnter(name);

	char *to = (char *) malloc (USER_SIZE);
	char *token = strtok(name, ",，");
	strcpy(to, "");
	while(token != NULL){
		strcat(to, token);
		token = strtok( NULL, ",， " );
		if( token != NULL){
			strcat(to, "\",\"");
		}
	}

	printf("> 输入消息内容: ");
	char *msg = (char *) malloc (BUFFER_SIZE);
	bzero(msg, BUFFER_SIZE);
	fgets(msg, BUFFER_SIZE, stdin);
	delEnter(msg);

	strcpy(input_msg, "{\"type\":\"1\",\"from\":\"");
	strcat(input_msg, user->name);
	strcat(input_msg, "\", \"to\":[\"");
	strcat(input_msg, to);
	strcat(input_msg, "\"], \"msg\":\"");
	strcat(input_msg, msg);
	strcat(input_msg, "\"}");
	send(server_sock_fd, input_msg, BUFFER_SIZE, 0);
}

/**
 * 发送消息给服务器，登录系统
 */
void sendMsgToLogin(char *input_msg, int server_sock_fd){
	if(1 == user->login){
		printf("▼ 系统提示 当前终端已登录，不能重复操作\n");
		return ;
	}
	printf("> 输入用户帐号: ");
	char *name = (char *) malloc (USER_SIZE);
	bzero(name, USER_SIZE);
	fgets(name, USER_SIZE, stdin);
	delEnter(name);
	strcpy(login_id, name); // 保存临时登录名

	char *pwd;// = (char *) malloc (BUFFER_SIZE);
	pwd = getpass("> 输入用户密码: "); // 使用 getpass 函数，输入密码不回显
	delEnter(pwd);

	strcpy(input_msg, "{\"type\":\"0\",\"from\":\"");
	strcat(input_msg, name);
	strcat(input_msg, "\", \"pwd\":\"");
	strcat(input_msg, pwd);
	strcat(input_msg, "\"}");
	send(server_sock_fd, input_msg, BUFFER_SIZE, 0);
}

/**
 * 发送消息给服务器，注册用户
 */
void sendMsgToRegister(char *input_msg, int server_sock_fd){
	printf("> 输入用户帐号: ");
	char *name = (char *) malloc (USER_SIZE);
	bzero(name, USER_SIZE);
	fgets(name, USER_SIZE, stdin);
	delEnter(name);

	char *pwd;// = (char *) malloc (BUFFER_SIZE);
	pwd = getpass("> 输入用户密码: "); // 使用 getpass 函数，输入密码不回显
	delEnter(pwd);

	strcpy(input_msg, "{\"type\": \"2\",\"from\": \"");
	strcat(input_msg, name);
	strcat(input_msg, "\",\"pwd\": \"");
	strcat(input_msg, pwd);
	strcat(input_msg, "\"}");

	send(server_sock_fd, input_msg, BUFFER_SIZE, 0);
}

/**
 * 发送消息给服务器，加载所有登录用户
 */
void loadAllOnlineUsers(char *input_msg, int server_sock_fd){
	strcpy(input_msg, "{\"type\": \"x\",\"from\": \"USER\"}");
	send(server_sock_fd, input_msg, BUFFER_SIZE, 0);
}

/**
 * 去除字符串末尾的换行符
 */
void delEnter(char *info){
	if('\n' == info[ strlen(info) - 1 ] ){
		info[ strlen(info) - 1 ] = '\0';
	}
}