#include <stdio.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <stdlib.h>
#include <linux/in.h>
#include <string.h>


#define N 128
#define S 1
#define U 2
#define A 3
#define D 4
#define H 5
#define US 6
#define UU 7

typedef struct{
	int type;
	char name[32];
	char word[128];
}MSG;


char buf[N] = {0};
void menu_frist(int sockfd, MSG *msg);
void menu_admin(int sockfd, MSG *msg);
void menu_user(int sockfd, MSG *msg);

void login_admin(int sockfd, MSG *msg);
void choice_admin(int sockfd, MSG *msg);

void login_user(int sockfd, MSG *msg);
void choice_user(int sockfd, MSG *msg);

int do_select(int sockfd, MSG *msg);
int do_update(int sockfd, MSG *msg);
int do_add(int sockfd, MSG *msg);
int do_del(int sockfd, MSG *msg);
int do_history(int sockfd, MSG *msg);
int do_user_select(int sockfd, MSG *msg);
int do_user_update(int sockfd, MSG *msg);


int main(int argc, const char *argv[])
{
	
	if(argc != 2)
	{
		printf("Usage: %s <PORT>\n", argv[0]);
		exit(-1);
	}

	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket failed");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	socklen_t server_len;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = inet_addr("10.10.10.102");
	server_len = sizeof(server_addr);


	if(connect(sockfd, (struct sockaddr *)&server_addr, server_len) < 0)
	{
		printf("connect error\n");
		exit(-1);
	}
	int ret;
	MSG msg;
	while(1)
	{
		menu_frist(sockfd, &msg);
	}

	return 0;
}


void menu_frist(int sockfd, MSG *msg)
{
	int num = 0;
	printf("*************************************************************\n");
	printf("********  1：管理员模式    2：普通用户模式    3：退出********\n");
	printf("*************************************************************\n");
	printf("请输入你的选择(数字):");
	scanf("%d", &num);

	switch(num){
		case 1:		printf("管理员登录\n");	
					menu_admin(sockfd, msg);
					break;

		case 2:		printf("普通用户登录\n");	
					menu_user(sockfd, msg);
					break;

		case 3:		exit(-1);			
					break;

		default:	printf("input error\n"); 
	};

}

void menu_admin(int sockfd, MSG *msg)
{
	login_admin(sockfd, msg);
	choice_admin(sockfd, msg);
}


void login_admin(int sockfd, MSG *msg)
{
	int ret;
	msg->type = 10;
	printf("Input name:");
	scanf("%s", msg->name);
	getchar();
	printf("Input passwd:");
	scanf("%s", msg->word);
	getchar();
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret < 0){
		printf("send failed\n");
		exit(-1);
	}
	else{
		printf("send ok\n");
	}

	ret = recv(sockfd, msg, sizeof(MSG), 0);
	if(ret < 0){
		printf("recv failed\n");
		exit(-1);
	}
	else{
		printf("recv ok\n");
	}
	if(strncmp(msg->word, "OK", 3) == 0){
		printf("LOGIN OK\n");
	}
	else{
		printf("user/passwd error\n");
		exit(-1);
	}
}

void choice_admin(int sockfd, MSG *msg)
{
	int num;
	printf("亲爱的管理员，欢迎您登陆员工管理系统!\n");
	printf("*************************************************************\n");
	printf("* 1：查询  2：修改 3：添加用户  4：删除用户  5：查询历史记录*\n");
	printf("* * 6：返回上一层目录						                *\n");
	printf("* ***********************************************************\n");
	printf("请输入你的选择(数字):");
	scanf("%d", &num);
	getchar();
	switch(num){
					//查询
		case 1:		do_select(sockfd, msg);
					break;
	
		case 2:		do_update(sockfd, msg);
					break;
		
		case 3:		do_add(sockfd, msg);
					break;
		
		case 4:		do_del(sockfd, msg);
					break;

		case 5:		do_history(sockfd, msg);
					break;
		
		case 6:		printf("返回上一层目录\n");
					msg->type = 6;
					send(sockfd, msg, sizeof(MSG), 0);
					printf("send ok\n");

					menu_frist(sockfd, msg);
					break;
		
		default:	menu_frist(sockfd, msg);
					printf("input error\n"); 
	};
}


int do_select(int sockfd, MSG *msg)
{
	int num;
	MSG msg1;
	msg->type = S;
	printf("*************************************************************\n");
	printf("* 1:按照人名查询  2:查询所有  					            *\n");
	printf("* ***********************************************************\n");
	printf("请输入你的选择(数字):");
	scanf("%d", &num);
	getchar();
	if(num == 1)
	{
		strcpy(msg->word, "S1");
		printf("请输入你要查询的名字:");
		scanf("%s", msg->name);
		getchar();

		send(sockfd, msg, sizeof(MSG), 0);
		printf("send ok\n");
		
		recv(sockfd, &msg1, sizeof(MSG), 0);
		printf("工号 姓名 密码 年龄 电话 地址 工作类型 时间 等级 工资\n");
		printf("%s\n", msg1.word);

		choice_admin(sockfd, msg);
	}
	else if(num == 2)
	{
		strcpy(msg->word, "S2");
		send(sockfd, msg, sizeof(MSG), 0);
		
		printf("工号 姓名 密码 年龄 电话 地址 工作类型 时间 等级 工资\n");
		while(1)
		{
			recv(sockfd, msg, sizeof(MSG), 0);

			if(strncmp(msg->word, "\0",1) == 0)
				break;
			
			printf("%s\n", msg->word);
		}

		choice_admin(sockfd, msg);
	}
	else{
		printf("input number error\n");
		do_select(sockfd, msg);
	}

	return 0;
}


int do_update(int sockfd, MSG *msg)
{
	int ret;
	msg->type = U;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send U ok\n");
	
	printf("输入想要修改的职工号:");
	scanf("%s", msg->name);
	getchar();

	printf("*******************请输入要修改的选项********************\n");
	printf("******	1：姓名	  2：年龄	3：家庭住址   4：电话  ******\n");
	printf("******	5：职位	  6：工资   7：入职年月   8：评级  ******\n");
	printf("******	9：密码	  10：退出				  		  *******\n");
	printf("*********************************************************\n");
	printf("输入想要修改的选项(数字):");
	scanf("%d", &msg->type);
	getchar();

	printf("输入修改之后的值:");
	scanf("%s", msg->word);
	getchar();

	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send msg ok\n");

	recv(sockfd, msg, sizeof(MSG), 0);
	if(strncmp(msg->word, "\0",1) == 0)
		printf("修改成功\n");
	else
		printf("修改失败\n");

	choice_admin(sockfd, msg);

	return 0;
}


int do_add(int sockfd, MSG *msg)
{
	char *info[10] = {0};
	char temp;
	int i = 0, ret = 0;
	msg->type = A;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send A ok\n");
	
	for(i = 0; i < 10; i++)
	{
		info[i] = (char *)malloc(sizeof(char)*32);
	}
	printf("***************热烈欢迎新员工***************\n");
	printf("请输入工号：");
	scanf("%s", info[0]);
	getchar();
	
	
	printf("工号一旦输入将无法修改!是否继续(Y/N):");
	scanf("%c", &temp);
	getchar();
	if(!(temp == 'y' || temp == 'Y'))
	{
		msg->word[0] = '\0';
		ret = send(sockfd, msg, sizeof(MSG), 0);
		if(ret > 0)
			printf("取消添加\n");
		choice_admin(sockfd, msg);
	}
	else
	{
		msg->word[0] = '*';
		ret = send(sockfd, msg, sizeof(MSG), 0);
		if(ret > 0)
			printf("继续添加\n");
	
	}

	printf("请输入用户名：");
	scanf("%s", info[1]);
	getchar();
	printf("请输入用户密码：");
	scanf("%s", info[2]);
	getchar();
	printf("请输入年龄：");
	scanf("%s", info[3]);
	getchar();
	printf("请输入电话：");
	scanf("%s", info[4]);
	getchar();
	printf("请输入家庭住址：");
	scanf("%s", info[5]);
	getchar();
	printf("请输入职位：");
	scanf("%s", info[6]);
	getchar();
	printf("请收入入职日期(格式：0000.00.00)：");
	scanf("%s", info[7]);
	getchar();
	printf("请输入评级(1~5,5为最高，新员工为 1)：");
	scanf("%s", info[8]);
	getchar();
	printf("请输入工资：");
	scanf("%s", info[9]);
	getchar();
	

	for(i = 0; i < 10; i++)
	{
		printf("%s\n", info[i]);
		ret = send(sockfd, info[i], 32, 0);
		if(ret < 0)
			printf("send info[%d] failed\n", i);
	}

	recv(sockfd, msg, sizeof(MSG), 0);
	if(strncmp(msg->word, "\0",1) == 0)
		printf("添加成功\n");
	else
		printf("添加失败\n");
	
	for(i = 0; i < 10; i++)
	{
		free(info[i]);
	}
	
	choice_admin(sockfd, msg);
	
	return 0;
}


int do_del(int sockfd, MSG *msg)
{
	int ret;
	msg->type = D;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send D ok\n");
	
	
	
	printf("请输入工号：");
	scanf("%s", msg->name);
	getchar();
	printf("请输入用户名：");
	scanf("%s", msg->word);
	getchar();
	
	
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send msg ok\n");
	
	
	recv(sockfd, msg, sizeof(MSG), 0);
	if(strncmp(msg->word, "\0",1) == 0)
		printf("删除成功\n");
	else
		printf("删除失败\n");
	
	choice_admin(sockfd, msg);
	return 0;
}


int do_history(int sockfd, MSG *msg)
{
	int ret;
	msg->type = H;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send H ok\n");

	while(1)
	{
		recv(sockfd, msg, sizeof(MSG), 0);

		if(strncmp(msg->word, "\0",1) == 0)
		{
			printf("查询成功\n");
			break;
		}
		printf("%s\n", msg->word);
	}

	printf("查询结束\n");
	
	choice_admin(sockfd, msg);
	
	return 0;
}


void menu_user(int sockfd, MSG *msg)
{
	login_user(sockfd, msg);
	choice_user(sockfd, msg);
}


void login_user(int sockfd, MSG *msg)
{
	int ret;
	char passwd[32] = {0};
	msg->type = 20;
	printf("Input ur name:");
	scanf("%s", msg->name);
	getchar();
	printf("Input ur passwd:");
	scanf("%s", passwd);
	getchar();
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret < 0){
		printf("send failed\n");
		exit(-1);
	}
	else{
		printf("send ok\n");
	}

	ret = recv(sockfd, msg, sizeof(MSG), 0);
	if(ret < 0){
		printf("recv failed\n");
		exit(-1);
	}
	else{
		printf("recv ok\n");
	}
	if(strncmp(msg->word, passwd, msg->type) == 0){
		printf("LOGIN OK\n");
	}
	else{
		printf("user/passwd error\n");
		exit(-1);
	}
}
void choice_user(int sockfd, MSG *msg)
{
	int num = 0;
	printf("*************************************************************\n");
	printf("*************  1：查询  	2：修改		3：退出	 ************\n");
	printf("*************************************************************\n");
	printf("请输入您的选择（数字）>>");
	scanf("%d", &num);
	switch(num){
		case 1:		printf("查询\n");
					do_user_select(sockfd, msg);
					break;
	
		case 2:		printf("修改\n");	
					do_user_update(sockfd, msg);
					break;

		default:	printf("input eror\n");
					choice_user(sockfd, msg);
	};	
}


int do_user_select(int sockfd, MSG *msg)
{
	int ret;
	msg->type = US;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send US ok\n");
	
	printf("请输入工号：");
	scanf("%s", msg->name);
	getchar();
	
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send 工号 ok\n");

	recv(sockfd, msg, sizeof(MSG), 0);
	printf("%s\n", msg->word);

	recv(sockfd, msg, sizeof(MSG), 0);
	if(strncmp(msg->word, "\0",1) == 0)
		printf("user 查询成功\n");
	else
		printf("user查询失败\n");
	
	choice_user(sockfd, msg);

	return 0;
}


int do_user_update(int sockfd, MSG *msg)
{
	int ret, num;
	msg->type = UU;
	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send UU ok\n");

	printf("***********请输入要修改的选项(其他信息亲请联系管理员)********\n");
	printf("***********	1：家庭住址   2：电话  3：密码  4：退出**********\n");
	printf("*************************************************************\n");
	
	printf("请输入要修改的选项:");
	scanf("%d", &(msg->type));
	getchar();
	if(msg->type == 4)
		exit(-1);

	printf("请输入工号:");
	scanf("%s", msg->name);
	getchar();
	
	printf("输入修改之后的值:");
	scanf("%s", msg->word);
	getchar();

	ret = send(sockfd, msg, sizeof(MSG), 0);
	if(ret > 0)
		printf("send msg ok\n");

	recv(sockfd, msg, sizeof(MSG), 0);
	if(strncmp(msg->word, "\0",1) == 0)
		printf("修改成功\n");
	else
		printf("修改失败\n");

	choice_user(sockfd, msg);

	return 0;
}














