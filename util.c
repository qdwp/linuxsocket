#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include "cJSON.h"
#include "util.h"


const char *allUsers = "ALLUSERS.JSON";
const char *printLog = "PRINTLOG.LOG";

// 申请一个新的节点空间，并置空 
static iUtil *iUtil_New_Item(){
    iUtil *node = (iUtil *)malloc(sizeof(iUtil));
    if (node) memset(node,0,sizeof(iUtil));
	return node;
}

// 创建一个带参数的节点
iUtil *iUtil_Create_Iterm(char *name, char *pwd, int key, int index){
    iUtil *iterm = iUtil_New_Item();
    if(iterm){
        iterm->name = (char *)malloc(strlen(name) + 1);
        iterm->pwd = (char *)malloc(strlen(pwd) + 1);
        strcpy(iterm->name, name);
        strcpy(iterm->pwd, pwd);
        iterm->key = key;
        iterm->index = index;
    }
    return iterm;
}

// 擦黄建一个空节点
iUtil *iUtil_Create(){
    iUtil *iterm = iUtil_New_Item();
    return iterm;
}

// 释放一个节点空间
void iUtil_Delete(iUtil *iterm){
    if (iterm){
        free(iterm);
    }
}

// 将一个新的节点放进链表内
iUtil *iUtil_Join_List(iUtil *root, iUtil *iterm){
    // printf("新添加节点 %s\n", iterm->name);
    if (root == NULL){
        return iterm;
    }
    iUtil *node = root;
    while(node != NULL && node->next != NULL) {
        if( node->key == iterm->key || strcmp(node->name, iterm->name) == 0 ){
            Log_S("重复登录", node->name);
            return root;
        }
        node = node->next;
    }
    node->next = iterm;
    iUtil_Print_List(root); // 打印列表
    return root;
}

// 打印列表数据
void iUtil_Print_List(iUtil *root){
    iUtil *iterm = root;
    Log("打印列表数据");
    while(iterm){
        Log_S("==>> ", iterm->name);
        iterm = iterm->next;
    }
}

// 在链表内删除一个节点
iUtil *iUtil_Delete_Iterm(iUtil *root, int key){
    if(!root){
        return NULL;
    }
    iUtil *iterm = root;
    iUtil *node;
    if(iterm->key == key){
        root = iterm->next;
        free(iterm);
        return root;
    }
    while(iterm->next){
        if(iterm->next->key == key){
            node = iterm->next;
            iterm->next = iterm->next->next;
            free(node);
            break;
        }
    }
    iUtil_Print_List(root); // 打印列表
    return root;
}

// 启动时加载文件内数据，表示已注册用户
iUtil *iUtil_Load_Users(){
    /* 若要一个byte不漏地读入整个文件，只能采用二进制方式打开 */
    FILE *users = fopen(allUsers, "rb");
    if(users == NULL){
        printf("用户文件打开失败");
        exit(1);
    }
    long lSize;  
    char * buffer;  
    size_t result;
    /* 获取文件大小 */  
    fseek (users , 0 , SEEK_END);  
    lSize = ftell (users);  
    rewind (users);  
  
    /* 分配内存存储整个文件 */   
    buffer = (char*) malloc (sizeof(char)*lSize);  
    if (buffer == NULL)  
    {  
        fputs ("Memory error",stderr);   
        exit (2);  
    }  
  
    /* 将文件拷贝到buffer中 */  
    result = fread (buffer,1,lSize,users);  
    if (result != lSize)  
    {  
        fputs ("Reading error",stderr);  
        exit (3);  
    }  
    // ALLUSERS.json 内容读进内存并转化成 JSON 格式
    cJSON *json = cJSON_Parse(buffer);
    
    fclose(users); // 关闭文件

    if (!json) {
        Log("文件加载失败，不是标准的 Json 文件");
    }
    printf("加载注册用户 %s\n", cJSON_Print(json));

    iUtil *root = iUtil_Create();
    iUtil *node = root;
    int size = cJSON_GetArraySize(json);
    int index = 0;
    for(; index < size ; index++){
        cJSON *c = cJSON_GetArrayItem(json, index);
        node->next = iUtil_Create_Iterm(
            cJSON_GetObjectItem(c,"name")->valuestring,
            cJSON_GetObjectItem(c,"pwd")->valuestring,
            -1,
            -1
            );
        node = node->next;
    }
    printf("用户: %s\n", root->next->name);
    return root->next;
}

// 用户信息写回文件
int iUtil_Write_Info(iUtil *root){
    FILE *users = fopen("ALLUSERS.json", "wb");
    if(users == NULL){
        printf("用户文件打开失败");
        exit(1);
    }
    iUtil *node = root;
    cJSON *info = cJSON_CreateArray();
    while(node){
        cJSON *iterm = cJSON_CreateObject();
        cJSON_AddStringToObject(iterm, "name", node->name);
        cJSON_AddStringToObject(iterm, "pwd", node->pwd);
        cJSON_AddItemToArray(info, iterm);
        node = node->next;
    }
    Log_S("用户信息写回文件", cJSON_Print(info));
    iUtil_Print_List(root);
    fprintf(users, "%s", cJSON_Print(info));
    //fclose(users);
    fflush(users);
    return 0;
}

// 注册用户
int iUtil_Do_Register(iUtil *root, char *name, char *pwd){
	iUtil *iterm = iUtil_Create_Iterm(name, pwd, -1, -1);
	if(!root){
		root = iterm;
        if(iUtil_Write_Info(root) != 0){
            return -1;
        }
		return 0;
	}
	iUtil *node = root;
	while(node->next){
		node = node->next;
	}
	node->next = iterm;
    if(iUtil_Write_Info(root) != 0){
        return -1;
    }
	return 0;
}

// 加载在线用户列表
char *iUtil_Load_List(iUtil *root){
    iUtil *node = root;
    char *list = (char *) malloc ( 1024 );
    strcpy(list, "\"");
    while(node){
        strcat( list, node->name );
        if(node->next){
            strcat( list, ", " );
        }
        node = node->next;
    }
    strcat( list, "\"" );
    return list;
}

// 通过用户名获取登录用户的 Key 值
int iUtil_GetItermKey(iUtil *root, char *name){
    iUtil *iterm = root;
    while(iterm){
        if( strcmp(iterm->name, name) == 0 ){
            return iterm->key;
        }
        iterm = iterm->next;
    }
    return 0;
}

// 打印日志文件 文本信息
void Log(const char *msg){
    FILE *log = fopen(printLog, "ab");
    if(log == NULL){
        printf("用户文件打开失败");
        exit(1);
    }
    fprintf(log, "%s > %s\n", getTime(), msg);
    fclose(log);
}

// 打印日志文件 int 信息
void Log_D(const char *format, int argument){
    FILE *log = fopen(printLog, "ab");
    if(log == NULL){
        printf("用户文件打开失败");
        exit(1);
    }
    char *string = (char *) malloc (strlen(format) + 15);
    strcpy(string, "%s > ");
    strcat(string, format);
    strcat(string, " : %d\n");
    fprintf(log, string, getTime(), argument);
    fclose(log);
}

// 打印日志文件 string 信息
void Log_S(const char *format, char *argument){
    FILE *log = fopen(printLog, "ab");
    if(log == NULL){
        printf("用户文件打开失败");
        exit(1);
    }
    char *string = (char *)malloc(strlen(format) + 15);
    strcpy(string, "%s > ");
    strcat(string, format);
    strcat(string, " : %s\n");
    fprintf(log, string, getTime(), argument);
    fclose(log);
}

// 转出时间格式 2016/12/24 22:15:01
char *getTime(){
    struct tm *local;
    time_t t;
    t = time(NULL);
    local = localtime( &t );
    char *surrentTime = (char *) malloc (30);
    strftime(surrentTime, 30, "%Y/%m/%d %H:%M:%S", local); 
    return surrentTime;
}

// 格式化发送消息
char *sendMsg(cJSON *json){
    cJSON *tm = cJSON_CreateString(getTime());
    cJSON_DeleteItemFromObject(json, "time");
    cJSON_AddItemToObject(json, "time", tm);
    return cJSON_Print(json);
}