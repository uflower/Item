#include <stdio.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <stdlib.h>
#include <linux/in.h>
#include <sqlite3.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#define N 128
#define S 1
#define U 2
#define A 3
#define D 4
#define H 5
#define US 6
#define UU 7

#define DATABASE "info.db"

typedef struct{
	int type;
	char name[32];
	char word[128];
}MSG;

fd_set readfds;

void menu(int acceptfd, MSG *msg, sqlite3 *db);
int login_admin(int acceptfd, MSG *msg, sqlite3 *db);
int choice_admin(int acceptfd, MSG *msg, sqlite3 *db);
int do_record(char *person, char *info, sqlite3 *db);

int login_user(int acceptfd, MSG *msg, sqlite3 *db);
int choice_user(int acceptfd, MSG *msg, sqlite3 *db);


int get_date(char *date);
int select_callback(void* arg,int f_num,char** f_value,char** f_name);
int select_user_callback(void* arg,int f_num,char** f_value,char** f_name);
int history_callback(void* arg,int f_num,char** f_value,char** f_name);
int user_login_callback(void* arg,int f_num,char** f_value,char** f_name);
int menu_admin(int acceptfd, MSG *msg, sqlite3 *db);
int menu_user(int acceptfd, MSG *msg, sqlite3 *db);
int do_select(int acceptfd, MSG *msg, sqlite3 *db);
int do_update(int acceptfd, MSG *msg, sqlite3 *db);
int do_add(int acceptfd, MSG *msg, sqlite3 *db);
int do_del(int acceptfd, MSG *msg, sqlite3 *db);
int do_history(int acceptfd, MSG *msg, sqlite3 *db);
int do_user_select(int acceptfd, MSG *msg, sqlite3 *db);
int do_user_update(int acceptfd, MSG *msg, sqlite3 *db);


int main(int argc, const char *argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s <PORT>\n", argv[0]);
		exit(-1);
	}

	int sockfd, acceptfd, maxfd;
	fd_set  tempfds;
	FD_ZERO(&readfds);	
	FD_ZERO(&tempfds);	

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket failed");
		exit(-1);
	}
	printf("socket seccess\n");
	FD_SET(sockfd, &readfds);
	maxfd = sockfd;


	struct sockaddr_in server_addr, client_addr;
	socklen_t server_len, client_len;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_len = sizeof(server_addr);
	client_len = sizeof(client_addr);
	if(bind(sockfd, (struct sockaddr *)&server_addr, server_len) < 0)
	{
		perror("bind failed");
		exit(-1);
	}
	printf("bind ok\n");

	if(listen(sockfd, 8) < 0)
	{
		perror("listen failed");
	}
	printf("listen ok\n");

	//打开数据库	
	sqlite3 *db;
	if(sqlite3_open(DATABASE, &db) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		exit(-1);
	}
	else
	{
		printf("database open success!\n");
	}


	int ret, i = 0;
	MSG msg;
	while(1)
	{
		tempfds = readfds;
		printf("waitting select:\n");
		ret = select(maxfd + 1, &tempfds, NULL, NULL, NULL);
		if(ret < 0)
		{
			printf("select failed\n");
			exit(-1);
		}
		else
		{
			for(i = 0; i < (maxfd + 1); i++)
			{
				if(FD_ISSET(i, &tempfds))
				{
					if(i == 3)
					{
						acceptfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
						printf("FD:%d IP:%s PORT:%d\n", acceptfd, (char *)inet_ntoa(client_addr.sin_addr.s_addr), client_addr.sin_port);
						FD_SET(acceptfd, &readfds);
						maxfd = acceptfd > maxfd ? acceptfd : maxfd;
					}
					else
					{
						menu(i, &msg, db);
					}
				}
			}
		}
	}

	return 0;
}


void menu(int acceptfd, MSG *msg, sqlite3 *db)
{
	int ret = 0;
	printf("waitting recv:\n");
	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		exit(-1);
	}
	else if(ret == 0)
	{
		FD_CLR(acceptfd, &readfds);
		printf("client exit\n");
		close(acceptfd);
	}
	else
	{
		printf("type:%d  name:%s word:%s\n", msg->type, msg->name, msg->word);
		switch(msg->type)
		{
			case 10: menu_admin(acceptfd, msg, db);	break;
			case 20: menu_user(acceptfd, msg, db);	break;
			default:
				 	printf("input error\n");
		};
	}
}



int menu_admin(int acceptfd, MSG *msg, sqlite3 *db)
{
	printf("this is menu_admin\n");
	login_admin(acceptfd, msg, db);
	choice_admin(acceptfd, msg, db);

	return 0;
}


int login_admin(int acceptfd, MSG *msg, sqlite3 *db)
{
	printf("this is login_admin\n");
	printf("type:%d  name:%s word:%s\n", msg->type, msg->name, msg->word);
	if((strncmp(msg->name, "admin", 6) == 0) && (strncmp(msg->word, "admin", 6) == 0))
	{
		printf("admin login\n");
		strcpy(msg->word, "OK");
		send(acceptfd, msg, sizeof(MSG), 0);
		//插入到历史记录表
		char temp[64];
		sprintf(temp, "login");

		do_record("admin", temp, db);

	}
	else
	{
		printf("useer/passed error\n");
		strcpy(msg->word, "error");
		send(acceptfd, msg, sizeof(MSG), 0);
		exit(-1);
	}

	return 0;
}


int choice_admin(int acceptfd, MSG *msg, sqlite3 *db)
{
	printf("this is choice_admin\n");
	int ret;
	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		return -1;
	}

	switch(msg->type)
	{
	case S: do_select(acceptfd, msg, db);	break;
	case U: do_update(acceptfd, msg, db);	break;
	case A: do_add(acceptfd, msg, db);		break;
	case D: do_del(acceptfd, msg, db);		break;
	case H: do_history(acceptfd, msg, db);	break;
	case 6: menu(acceptfd, msg, db);		break;
	default:
			 printf("input error\n");
	};
	return 0;
}



int select_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	int acceptfd;
	MSG msg;
	acceptfd = *((int *)arg);

	sprintf(msg.word, "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s", f_value[0], f_value[1], f_value[2], f_value[3], f_value[4], f_value[5], f_value[6], f_value[7], f_value[8], f_value[9]);

	send(acceptfd, &msg, sizeof(MSG), 0);

	return 0;
}


int do_select(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;

	if(strncmp(msg->word, "S1", 2) == 0)
	{
		printf("指定查询\n");
		memset(sql, sizeof(sql), 0);
		sprintf(sql, "select * from staff where name = '%s'", msg->name);
		if(sqlite3_exec(db, sql, select_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
		}
		else
		{
			printf("select one done.\n");
		}

		//插入到历史记录表
		char temp[64];
		sprintf(temp, "select name='%s'", msg->name);

		do_record("admin", temp, db);

	}

	if(strncmp(msg->word, "S2", 2) == 0)
	{
		printf("查询所有\n");
		memset(sql, sizeof(sql), 0);
		strcpy(sql, "select * from staff");
		if(sqlite3_exec(db, sql, select_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
		}
		else
		{
			printf("select all done.\n");
		}

		msg->word[0] = '\0';
		send(acceptfd, msg, sizeof(MSG), 0);
		
		//插入到历史记录表
		char temp[64];
		sprintf(temp, "select all", msg->name);

		do_record("admin", temp, db);
	}

	choice_admin(acceptfd, msg, db);
	return 0;
}


int do_update(int acceptfd, MSG *msg, sqlite3 *db)
{
	printf("修改信息\n");
	char col[16] = {0};
	int ret;
	char sql[128] = {0};
	char *errmsg;

	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv failed");
		return -1;
	}
	else{
		printf("recv num ok\n");
	}

	switch(msg->type){
		case 1:	strcpy(col, "name");	break;
		case 2:	strcpy(col, "age");		break;
		case 3:	strcpy(col, "addr");	break;
		case 4:	strcpy(col, "phone");	break;
		case 5:	strcpy(col, "work");	break;
		case 6:	strcpy(col, "salary");	break;
		case 7:	strcpy(col, "date");	break;
		case 8:	strcpy(col, "level");	break;
		case 9:	strcpy(col, "passed");	break;
		case 10:	exit(-1);			break;
		default:
				printf("input error\n");
	
	};
	memset(sql, sizeof(sql), 0);
	sprintf(sql, "update staff set '%s'='%s' where no = '%s'", col, msg->word, msg->name);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
	{
		printf("update ok!\n");
	}

	//插入到历史记录表
	char temp[64];
	sprintf(temp, "update no='%s' values='%s'", msg->name, msg->word);

	do_record("admin", temp, db);

	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);
	
	choice_admin(acceptfd, msg, db);
	return 0;
}



int do_add(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char info[10][32] = {0};
	char *errmsg;
	int ret = 0, i = 0;

	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv msg failed");
		exit(-1);
	}

	if(strncmp(msg->word, "*", 1) != 0)
	{
		printf("取消添加\n");
		choice_admin(acceptfd, msg, db);
	}

	for(i = 0; i < 10; i++)
	{
		ret = recv(acceptfd, &info[i][0], 32, 0);
		if(ret < 0)
		{
			perror("recv msg failed");
			return -1;
		}
	}

	memset(sql, sizeof(sql), 0);
	sprintf(sql, "insert into staff values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')", &info[0][0],&info[1][0],&info[2][0],&info[3][0],&info[4][0],&info[5][0],&info[6][0],&info[7][0],&info[8][0],&info[9][0]);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
	{
		printf("update ok!\n");
	}

	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);

	//插入到历史记录表
	char temp[64];
	sprintf(temp, "add no='%s' and name='%s'", &info[1][0], &info[2][0]);

	do_record("admin", temp, db);

	choice_admin(acceptfd, msg, db);

	return 0;
}


int do_del(int acceptfd, MSG *msg, sqlite3 *db)
{
	int ret;
	char sql[128] = {0};
	char *errmsg;
	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		return -1;
	}
	
	memset(sql, sizeof(sql), 0);
	sprintf(sql, "delete from staff where no='%s' and name='%s'", msg->name, msg->word);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
		msg->word[0] = '*';
		send(acceptfd, msg, sizeof(MSG), 0);
	}
	else
	{
		printf("delete ok!\n");
	}
	

	//插入到历史记录表
	char temp[64];
	sprintf(temp, "delete no='%s' and name='%s'", msg->name, msg->word);

	do_record("admin", temp, db);
	
	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);
	
	choice_admin(acceptfd, msg, db);

	return 0;
}


int history_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	int acceptfd;
	MSG msg;

	acceptfd = *((int *)arg);

	sprintf(msg.word, "%s, %s, %s", f_value[0], f_value[1], f_value[2]);

	send(acceptfd, &msg, sizeof(MSG), 0);

	return 0;
}


int do_history(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;


	printf("查询历史记录\n");
	memset(sql, sizeof(sql), 0);
	sprintf(sql, "select * from history");
	if(sqlite3_exec(db, sql, history_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
		msg->word[0] = '*';
		send(acceptfd, msg, sizeof(MSG), 0);
		return 0;
	}
	else
	{
		printf("history_callback done.\n");
	}


	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);

	//插入到历史记录表
	do_record("admin", "select history", db);


	choice_admin(acceptfd, msg, db);

	return 0;
}


int do_record(char *person, char *info, sqlite3 *db)
{
	//插入到历史记录表
	char date[64] = {0};
	char sql[128] = {0};
	char *errmsg;
	printf("this is who:%s\n",person);
	printf("this is info:%s\n", info);

	get_date(date);
	memset(sql, sizeof(sql), 0);
	sprintf(sql, "insert into history values(\"'%s'\", \"'%s'\", \"'%s'\")", date, person, info);

	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	else
	{
		printf("record done.\n");
	}
	return 0;
}


int user_login_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	int acceptfd;
	MSG msg;

	acceptfd = *((int *)arg);

	msg.type = strlen(f_value[2]);
	sprintf(msg.word, "%s", f_value[2]);

	send(acceptfd, &msg, sizeof(MSG), 0);

	return 0;
}



int menu_user(int acceptfd, MSG *msg, sqlite3 *db)
{
	login_user(acceptfd, msg, db);
	choice_user(acceptfd, msg, db);
	
	return 0;
}



int login_user(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;
	printf("this is menu_user\n");
	printf("type:%d  name:%s word:%s\n", msg->type, msg->name, msg->word);

	memset(sql, sizeof(sql), 0);
	sprintf(sql, "select * from staff where name = '%s'", msg->name);
	if(sqlite3_exec(db, sql, user_login_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return 0;
	}
	//插入到历史记录表
	char temp[64];
	sprintf(temp, "login");

	do_record("user", temp, db);

	return 0;
}



int choice_user(int acceptfd, MSG *msg, sqlite3 *db)
{

	int ret;
	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		return -1;
	}

	switch(msg->type)
	{
	case US: do_user_select(acceptfd, msg, db);	break;
	case UU: do_user_update(acceptfd, msg, db);	break;
	default:
			 printf("input error\n");
			 choice_user(acceptfd, msg, db);
	};


	return 0;
}


int select_user_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	int acceptfd;
	MSG msg;

	acceptfd = *((int *)arg);

	sprintf(msg.word, "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s", f_value[0], f_value[1], f_value[2], f_value[3], f_value[4], f_value[5], f_value[6], f_value[7], f_value[8], f_value[9]);

	send(acceptfd, &msg, sizeof(MSG), 0);
	return 0;
}
int do_user_select(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;
	int ret;

	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		return -1;
	}

	memset(sql, sizeof(sql), 0);
	sprintf(sql, "select * from staff where no = '%s'", msg->name);
	if(sqlite3_exec(db, sql, select_user_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
	{
		printf("select_user_callback done.\n");
	}


	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);
	
	choice_user(acceptfd, msg, db);
	
	return 0;
}


int do_user_update(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;
	int ret;
	char col[32] ={};
	ret = recv(acceptfd, msg, sizeof(MSG), 0);
	if(ret < 0)
	{
		perror("recv");
		return -1;
	}

	switch(msg->type){
		case 1:	strcpy(col, "addr");	break;
		case 2:	strcpy(col, "phone");	break;
		case 3:	strcpy(col, "passed");	break;
		default:
				printf("input error\n");
	
	};

	memset(sql, sizeof(sql), 0);
	sprintf(sql, "update staff set '%s'='%s' where no='%s'", col, msg->word, msg->name);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
	{
		printf("user update done.\n");
	}

	msg->word[0] = '\0';
	send(acceptfd, msg, sizeof(MSG), 0);

	choice_user(acceptfd, msg, db);

	return 0;
}


int get_date(char *date)
{
	time_t t;
	struct tm *tp;

	time(&t);

	//进行时间格式转换
	tp = localtime(&t);

	sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year +1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min , tp->tm_sec);
	//printf("get date:%s\n", date);
	
	return 0;
}



