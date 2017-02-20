#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include "cJSON.h"
#include "util.h"

#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define CONCURRENT_MAX 100   //应用层同时可以处理的连接
#define SERVER_PORT 11332
#define BUFFER_SIZE 1024
#define QUIT_CMD ".quit"

int client_fds[CONCURRENT_MAX];
iUtil *onlineList;
iUtil *allUsers;

void getMsgFromClient(int index,char *msg);
void serverSendToClient(int client_fds[],char *input_msg);

int main(int argc, const char * argv[])
{
    char input_msg[BUFFER_SIZE];
    char recv_msg[BUFFER_SIZE];
    //本地地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(&(server_addr.sin_zero), 8);
    //创建socket

	Log("=========    记录日志   ========");
    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_fd == -1)
    {
    	Log("服务端创建 Socket 失败");
    	return 1;
    }
    //绑定socket
    int bind_result = bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bind_result == -1)
    {
    	Log("服务端绑定端口失败, 端口占用, 请重新启动服务器终端");
    	printf("服务端绑定端口失败, 端口占用, 请重新启动服务器终端\n");
    	return 1;
    }
    //listen
    if(listen(server_sock_fd, BACKLOG) == -1)
    {
    	printf("服务端创建监听失败, 请重新启动服务器终端\n");
    	return 1;
    }
    //fd_set
    fd_set server_fd_set;
    int max_fd = -1;
	int i = 0; //定义循环变量
    struct timeval tv;  //超时时间设置

	onlineList = NULL;
	allUsers = iUtil_Load_Users(); // 
	Log("开始监听客户端发送的消息...");
    while(1)
    {
    	tv.tv_sec = 20;
    	tv.tv_usec = 0;
    	FD_ZERO(&server_fd_set);
    	FD_SET(STDIN_FILENO, &server_fd_set);
    	if(max_fd <STDIN_FILENO)
    	{
    		max_fd = STDIN_FILENO;
    	}
		
		//服务器端socket
        FD_SET(server_sock_fd, &server_fd_set);
		
        if(max_fd < server_sock_fd)
        {
        	max_fd = server_sock_fd;
        }
		//客户端连接
        for(i = 0; i < CONCURRENT_MAX; i++)
        {
        	if(client_fds[i] != 0)
        	{
        		FD_SET(client_fds[i], &server_fd_set);
        		if(max_fd < client_fds[i])
        		{
        			max_fd = client_fds[i];
        		}
        	}
        }
        int ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);
        if(ret < 0)
        {
        	Log("select 与服务器连接出错");
        	continue;
        }
        else if(ret == 0)
        {
        	// printf("select 超时\n");
        	continue;
        }
        else
        {
        	//ret 为未状态发生变化的文件描述符的个数
        	if(FD_ISSET(STDIN_FILENO, &server_fd_set))
        	{	
				printf("> ");
        		bzero(input_msg, BUFFER_SIZE);
        		fgets(input_msg, BUFFER_SIZE, stdin);
        		printf("发送消息：%s\n", input_msg);
        		//输入“.quit"则退出服务器
        		if(strncasecmp(input_msg, QUIT_CMD, 5) == 0)
        		{
        			exit(0);
        		}
				// 发送信息到客户端
        		for(i = 0; i < CONCURRENT_MAX; i++)
        		{
        			if(client_fds[i] != 0)
        			{
        				printf("client_fds[%d]=%d\n", i, client_fds[i]);
        				send(client_fds[i], input_msg, BUFFER_SIZE, 0);
						
						// 设置发送信息
						// serverSendToClient(client_fds, input_msg);

        			}
        		}
        	}
        	if(FD_ISSET(server_sock_fd, &server_fd_set))
        	{
        		//有新的连接请求
        		struct sockaddr_in client_address;
        		socklen_t address_len;
        		int client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_address, &address_len);
        		printf("new connection client_sock_fd = %d\n", client_sock_fd);
        		if(client_sock_fd > 0)
        		{
        			int index = -1;
        			for(i = 0; i < CONCURRENT_MAX; i++)
        			{
        				if(client_fds[i] == 0) {
        					index = i;
        					client_fds[i] = client_sock_fd;
        					break;
        				}
        			}
        			if(index >= 0) {
						Log_D("新客户端 index", index);
						Log_D("===>>   key", client_fds[index]);
        				printf("新客户端(%d)加入成功 %s:%d\n", index, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        			} else {
        				send(client_sock_fd, RECEIVE_OVER_MAX, BUFFER_SIZE, 0);
        				printf("客户端连接数达到最大值，新客户端加入失败 %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        			}
        		}
        	}
        	for(i = 0; i < CONCURRENT_MAX; i++)
        	{
        		if(client_fds[i] !=0)
        		{
        			if(FD_ISSET(client_fds[i], &server_fd_set))
        			{
        				// 处理某个客户端过来的消息
        				bzero(recv_msg, BUFFER_SIZE);
        				long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
        				if (byte_num > 0)
        				{
        					if(byte_num > BUFFER_SIZE)
        					{
        						byte_num = BUFFER_SIZE;
        					}
        					recv_msg[byte_num] = '\0';
							Log_S("收到客户端发送的消息", recv_msg);

							getMsgFromClient(i, recv_msg);
        				}
        				else if(byte_num < 0)
        				{
							Log_D("从客户端接收消息出错", i);
        					printf("从客户端(%d)接收消息出错.\n", i);
        				}
        				else
        				{
        					printf("客户端(%d)退出了\n", i);
							Log_D("客户端退出", client_fds[i]);
							onlineList = iUtil_Delete_Iterm(onlineList, client_fds[i]);
        					FD_CLR(client_fds[i], &server_fd_set);
        					client_fds[i] = 0;
        				}
        			}
        		}
        	}
        }
    }
    return 0;
}

/**
 * 从客户端获取信息并解析
*/
void getMsgFromClient(int index,char *info){
	char *out;
	Log("开始转换接收数据");
	cJSON_Minify(info);
	cJSON *json = cJSON_Parse(info);
	Log_S("转换接收数据完成", info);
	if (!json) {
		// printf("转码接收信息错误: %s\n", cJSON_GetErrorPtr());
		Log("转码接收信息错误");
	} else {
		Log("开始判断消息类型...");
		char *type = cJSON_GetObjectItem(json,"type")->valuestring;
		char *from = cJSON_GetObjectItem(json, "from")->valuestring;
	 	if (strcmp(type, "1") == 0) {
			Log_S("1 用户发送消息", from);
			cJSON *to = cJSON_GetObjectItem(json, "to");
			int len = cJSON_GetArraySize(to);
			char *msg = cJSON_GetObjectItem(json, "msg")->valuestring;

			iUtil *node = onlineList;
			Log_S("准备发送消息", from);
			int flag = 0;
			while(node){
				int i = 0;
				for(; i < len ; i++){
					if( strcmp(cJSON_GetArrayItem(to, i)->valuestring, node->name) == 0 ){
						Log_S("接收端", cJSON_GetArrayItem(to, i)->valuestring);
						Log_S("发送消息", msg);
						flag = 1;
						char *rspMsg = sendMsg(json);
						Log_S("服务器发送", rspMsg);
						send(node->key, rspMsg, BUFFER_SIZE, 0);
					}
				}
				node = node->next;
			}
			if( 0 == flag ){
				Log("目标用户不存在");
				Log_S("服务器发送", RECEIVE_NOLOGIN);
				send(iUtil_GetItermKey(onlineList, from), RECEIVE_NOLOGIN, BUFFER_SIZE, 0);
				return ;
			}
		} else if (strcmp(type, "x") == 0) {
			char *list = iUtil_Load_List(onlineList);
			char *info = (char *) malloc (BUFFER_SIZE);
			strcpy(info, "{\"type\":\"x\", \"from\":\"server\", \"msg\":");
			strcat(info, list);
			strcat(info, ", \"time\":\"");
			strcat(info, getTime());
			strcat(info, "\"}");
			Log_S("服务器发送", info);
			send( client_fds[index], info, BUFFER_SIZE, 0);

		} else if (strcmp(type, "0") == 0) {
			Log_S("0 用户登录系统" , from);
			char *pwd = cJSON_GetObjectItem(json, "pwd")->valuestring;
			// char *msg = cJSON_GetObjectItem(json, "msg")->valuestring;
			iUtil *node = allUsers;
			while(node){
				if( strcmp( from, node->name ) == 0 
					&&  strcmp( pwd, node->pwd ) == 0 ){
					break;
				}
				node = node->next;
			}
			if(node == NULL){
				Log_S( "LOGIN_FAILED", from );
				send( client_fds[index], LOGIN_FAILED, BUFFER_SIZE, 0 );
			} else {
				Log_S( "LOGIN_SUCCESS", from );
				onlineList = iUtil_Join_List( onlineList, iUtil_Create_Iterm(from, pwd, client_fds[index], index ));
				Log_S("服务器发送", LOGIN_SUCCESS);
				send( client_fds[index], LOGIN_SUCCESS, BUFFER_SIZE, 0 );
				Log("添加在线列表成功");
			}
		} else if (strcmp(type, "2") == 0) {
			Log_S("2 用户注册", from);
			char *pwd = cJSON_GetObjectItem(json, "pwd")->valuestring;
			iUtil *list = allUsers;
			while(list){
				if( strcmp(list->name, from) == 0 ){
					Log_S("服务器发送", REGISTER_FAILED);
					send( client_fds[index], REGISTER_FAILED, BUFFER_SIZE, 0 );
					return;
				}
				list = list->next;
			}
			if(iUtil_Do_Register(allUsers, from, pwd) == 0){
				Log_S("服务器发送", REGISTER_SUCCESS);
				send( client_fds[index], REGISTER_SUCCESS, BUFFER_SIZE, 0 );
			}
			return ;
		} else {
			return ;
		}

		return ;
	}
	return ;
}

