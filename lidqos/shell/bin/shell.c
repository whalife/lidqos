/*
 * shell.c
 *
 *  Created on: Nov 26, 2014
 *      Author: lidq
 */

#include <shell/shell.h>

s_session *session = NULL;
char *cmd_file = NULL;
char *cmd_param = NULL;
char *full_path = NULL;

int main(int argc, char **args)
{
	//设置计算机名（以后要从/etc/hostname里读入）
	char *computer_name = COMPUTER_NAME;

	init_shell();

	//加入默认环境变量
	shell_init_path("/usr/bin/");

	while (1)
	{
		login();

		char *cmd = malloc(SHELL_CMD_LEN);

		while (session->status == SESSION_STATUS_LOGIN)
		{
			get_current_folder_name(session->current_folder_name);
			printf("[%s@%s %s]: ", session->username, computer_name, session->current_folder_name);
//			gets(cmd);
			if (str_compare(SHELL_CMD_EXIT, cmd) == 0)
			{
				break;
			}
//			execute_cmd(cmd);
			printf("%d\n", random(0, 100));
			execute_cmd("/usr/bin/example_fpu");
//			execute_cmd("/home/lidq/welcome");
		}
		free(cmd);

		logout();
	}

	return 0;
}

void init_shell()
{
	session = malloc(sizeof(s_session));
	session->username = malloc(USER_U_LEN);
	session->home = malloc(USER_H_LEN);
	session->current_path = malloc(SHELL_CMD_LEN);
	session->current_folder_name = malloc(SHELL_CMD_LEN);
	session->PATH = malloc(SHELL_CMD_LEN);

	session->username[0] = '\0';
	session->home[0] = '\0';
	session->current_path[0] = '\0';
	session->current_folder_name[0] = '\0';
	session->PATH[0] = '\0';
	session->status = SESSION_STATUS_NOLOGIN;

	cmd_file = malloc(SHELL_CMD_LEN);
	cmd_param = malloc(SHELL_CMD_LEN);
	full_path = malloc(SHELL_CMD_LEN);
}

void shell_init_path(char *path)
{
	str_copy(path, session->PATH);
}

void login()
{
	char *username = malloc(USER_U_LEN);
	char *passwd = malloc(USER_P_LEN);
	while (session->status == SESSION_STATUS_NOLOGIN)
	{
		printf("LidqOS login: ");
		gets(username);
		char *p = passwd;
		puts("Password: ");
		while ((*p = getchar()) != '\n')
		{
			//putchar('*');
			p++;
		}
		putchar('\n');
		*p = '\0';

		check_passwd(username, passwd);
	}
	free(username);
	free(passwd);
}

void logout()
{
	session->username[0] = '\0';
	session->home[0] = '\0';
	session->status = SESSION_STATUS_NOLOGIN;
}

int check_passwd(char *username, char *passwd)
{
	str_copy("lidq", session->username);
	str_copy("/home/lidq/", session->home);
	session->uid = 1;
	session->gid = 1;
	session->status = SESSION_STATUS_LOGIN;
	return 1;

	//临时校验root
	if (str_compare(username, "root") == 0 && str_compare(passwd, "root") == 0)
	{
		str_copy(username, session->username);
		str_copy("/root/", session->home);
		str_copy(session->home, session->current_path);
		session->uid = 0;
		session->gid = 0;
		session->status = SESSION_STATUS_LOGIN;

		printf("Login successful.\n");

		return 1;
	}
	else if (str_compare(username, "lidq") == 0 && str_compare(passwd, "lidq") == 0)
	{
		str_copy(username, session->username);
		str_copy("/home/lidq/", session->home);
		str_copy(session->home, session->current_path);
		session->uid = 1;
		session->gid = 1;
		session->status = SESSION_STATUS_LOGIN;

		printf("Login successful.\n");

		return 1;
	}
	else
	{
		printf("Login incorrect.\n");
	}
	return 0;
}

void execute_cmd(char *cmd)
{
	if (str_len(cmd) <= 0)
	{
		return;
	}

	if (cmd[0] == '/')
	{
		str_copy(cmd, full_path);
	}
	else
	{
		str_append(session->PATH, cmd, full_path);
	}

//	FILE *fp = fopen(full_path, FS_MODE_READ);
//	if (fp == NULL)
//	{
//		printf("-bash: \"%s\": command not found.\n", cmd);
//		fclose(fp);
//		return;
//	}
//	fclose(fp);
	install_program(full_path, cmd);
}

void install_program(char *path, char *args)
{
	u32 sem_addr = 0;
	int params[0x5];
	params[0] = 0;
	params[1] = (int) path;
	params[2] = (int) args;
	params[3] = (int) session;
	params[4] = (int) &sem_addr;
	__asm__ volatile("int $0x80" :: "a"(params));
	if (sem_addr != 0)
	{
		sem_wait_shell(sem_addr);
		sem_post_shell(sem_addr + sizeof(s_sem));
	}
	else
	{
		printf("-bash: \"%s\": error.\n", cmd_file);
	}
}

void get_current_folder_name(char *folder_name)
{
	int last_ind = 0;
	//最后一个'/'不算
	for (int i = str_len(session->current_path) - 2; i >= 0; i--)
	{
		if (session->current_path[i] == '/')
		{
			last_ind = i;
			break;
		}
	}
	str_sub(session->current_path, last_ind + 1, str_len(session->current_path) - (last_ind + 1), folder_name);
	if (folder_name[0] == '\0')
	{
		folder_name[0] = '/';
		folder_name[1] = '\0';
	}
}

