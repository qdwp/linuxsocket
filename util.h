/**
 * 定义快捷使用结构和方法
 * @param msg 接收消息 
 * @author 齐敦伟
 * @create 2016/12/24
*/

#ifndef UTIL_H
#define UTIL_H

#include "cJSON.h"

#define LOGIN_FAILED "{\"type\": \"9\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"用户名或密码错误\", \"time\":\"系统通知\"}"
#define LOGIN_SUCCESS "{\"type\": \"8\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"用户登陆成功\", \"time\":\"系统通知\"}"
#define LOGIN_REQUEST "{\"type\": \"7\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"用户成功登录，请不要重复登录\", \"time\":\"系统通知\"}"
#define REGISTER_FAILED "{\"type\": \"6\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"用户注册错误, 账号已存在\", \"time\":\"系统通知\"}"
#define REGISTER_SUCCESS "{\"type\": \"5\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"用户注册成功\", \"time\":\"系统通知\"}"
#define RECEIVE_NOLOGIN "{\"type\": \"4\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"指定接收消息的用户不存在或未登录\", \"time\":\"系统通知\"}"
#define RECEIVE_OVER_MAX "{\"type\": \"3\",\"from\": \"server\",\"pwd\": \"***\",\"to\": [], \"msg\": \"超出最大连接数，连接服务器失败\", \"time\":\"系统通知\"}"

// type 2 注册
// type 1 发送
// type 0 登录

typedef struct iUtil{
    struct iUtil *next;
    int key;
    int index;
    char *name;
    char *pwd;
} iUtil;


extern void iUtil_Delete(iUtil * iterm);    // 释放一个节点空间

extern iUtil *iUtil_Create();   /* 创建一个新的 iUtil 节点 */
extern iUtil *iUtil_Create_Iterm(char *name, char *pwd, int key, int index);
extern iUtil *iUtil_Join_List(iUtil *root, iUtil *iterm);   // 将一个新的节点放进链表内
extern iUtil *iUtil_Delete_Iterm(iUtil *root, int key); // 在链表内删除一个节点
extern iUtil *iUtil_Load_Users();   // 启动时加载文件内数据，表示已注册用户
extern char *iUtil_Load_List(iUtil *root);   // 加载登录用户列表

extern int iUtil_GetItermKey(iUtil *root, char *name); // 查询一个节点的 key

extern void iUtil_Print_List(iUtil *root); // 打印列表信息

extern void Log(const char *string);
extern void Log_D(const char *format, int argument);
extern void Log_S(const char *format, char *argument);

extern char *getTime();
extern char *sendMsg(cJSON *json);

int writeInfo(iUtil *root);
int iUtil_Do_Register(iUtil *root, char *name, char *pwd);

#endif //UTIL_H