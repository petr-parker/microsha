#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>
#include <sys/resource.h>
#include <sys/times.h>
#include <signal.h>
#include <errno.h>

using namespace std;

#include "help.cc"

void meta(vector<string> commands) {
	int i_meta;
	for (int i = 0; i < commands.size(); i++) {
		if (is_meta(commands[i])) {
			i_meta = i;
			break;
		}
	}

	vector<string> v_meta = split(commands[i_meta], '/');
	int depth = v_meta.size();

	vector<string> all;
	if (commands[i_meta][0] == '/') {
	    const char * base = "/";
	    all = walk(base, depth);
	} else {
	    const char * base = ".";
	    all = walk(base, depth);
	}
	sort(all.begin(), all.end());

	vector<string> v;
	string replace;
	vector<string> comm = commands;
	for (int i = 0; i < all.size(); i++) {
		v = split(all[i], '/');
		if  (v[0] == ".") v.erase(v.begin());
		if  (v_meta[0] == ".") v_meta.erase(v_meta.begin());
		if (v_metacmp(v, v_meta)) {
			replace = join(v, "/");
			if (commands[i_meta][0] == '/') replace = "/" + replace;
			comm[i_meta] = replace;
			pid_t pid = fork();
			if (pid == 0) {
				vector<char *> c_comm = v_c_str(comm);
				execvp(c_comm[0], &c_comm[0]);
				perror(sys_errlist[errno]);
				exit(errno);
			}
			int status;
			pid_t waitd = wait(&status);
		}
	}	
}

void meta_io(vector<string> commands) {
	int i_in = -1;
	int i_out = -1;
	for (int i = 0; i < commands.size(); i++) {
		if (commands[i] == "<") {
			if (i_in != -1) {
				printf("Too many <\n");
				return;
			}
			i_in = i;
		}
		if (commands[i] == ">") {
			if (i_out != -1) {
				printf("Too many >\n");
				return;
			}
			i_out = i;
		}
	}
	int in = 0;
	int out = 1;
	if (i_in != -1 && commands.size() >= i_in) {
		in = open(commands[i_in + 1].c_str(), O_RDWR | O_CREAT, 0666);
	}
	if (i_out != -1 && commands.size() >= i_out) {
		out = open(commands[i_out + 1].c_str(), O_RDWR | O_CREAT, 0666);
	}
	if (in != 0) {
		vector<string>::iterator it_in = find(commands.begin(), commands.end(), "<");
		commands.erase(it_in, it_in + 2);
	}
	if (out != 1) {
		vector<string>::iterator it_out = find(commands.begin(), commands.end(), ">");
		commands.erase(it_out, it_out + 2);
	}
	pid_t pid = fork();
	if (pid == 0) {
		dup2(in, 0);
		dup2(out, 1);
		
		vector<char *> c_commands = v_c_str(commands);
		execvp(c_commands[0], &c_commands[0]);
		perror(sys_errlist[errno]);
		exit(errno);
	}
	int status;
	pid_t waitd = wait(&status);
}

void call(vector<string> commands) {
	if (v_is_meta(commands)) {
	 	meta(commands);
	} else if (v_is_meta_io(commands)) {
		meta_io(commands);
	} else {
	    pid_t pid = fork();
	    if (pid == 0) {
	        vector<char *> c_commands = v_c_str(commands);
	        execvp(c_commands[0], &c_commands[0]);
			perror(sys_errlist[errno]);
			exit(errno);
	    }
		int status;
	    pid_t waitd = wait(&status);   
	}
}

void my_time(vector<string> commands) {
    commands.erase(commands.begin()); // удалить time
	
	struct timeval stop, start;
	struct rusage ru;

    gettimeofday(&start, NULL);

	pid_t pid = fork();    
    if (pid == 0) {
        vector<char *> c_commands = v_c_str(commands);
        execvp(c_commands[0], &c_commands[0]);
        perror(sys_errlist[errno]);
        exit(errno);
    }

	int status;
	pid_t waitd = wait(&status);

	getrusage(RUSAGE_CHILDREN, &ru);
	gettimeofday(&stop, NULL);

	long int seconds = stop.tv_sec - start.tv_sec;
	int microseconds = stop.tv_usec - start.tv_usec;
	if (microseconds < 0) {
		seconds-=1;
		microseconds+=1000000;
	}

	printf("real: %ld.%06d s\n", seconds, microseconds);
	printf("user: %ld.%06d s\n", ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
	printf("system: %ld.%06d s\n", ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
}

void change_dir(vector<string> commands) {
	if (commands.size() == 1) {
		string user_name = string(getenv("USER"));
		string cpp_path = "/Users/" + user_name;
		char * path = (char *) cpp_path.c_str();
		if (chdir(path) == -1) {
			perror("chdir");
		}
	} else {
		char * path = (char *) commands[1].c_str();
		if (chdir(path) == -1) {
			perror("chdir");
		}
	}
}

void pwd(vector<string> commands) {
	char path[100] = {0};
	getcwd(path, 100);
	int len = write(1, path, sizeof(path));
	write(1, "\n", sizeof("\n"));
}

void single(vector<string> commands) {
	if (commands[0] == "time")		{ my_time(commands); }
	else if (commands[0] == "cd")	{ change_dir(commands); }
	else if (commands[0] == "pwd")	{ pwd(commands); }
	else							{ call(commands); }
}

void conveer_call(vector<string> commands) {
	call(commands);
	exit(0);
}

int conveer(vector<vector<string> > &commands) {
	int fd[2][2];
	bool current = false;
	int status;
	pid_t waitd;

	pipe(fd[0]);
	pid_t pid = fork();
	if (pid == 0) {
		dup2(fd[current][1], 1);
		conveer_call(commands[0]);
	}
	for (int i = 1; i < commands.size() - 1; i++) {
		waitd = wait(&status);
		close(fd[current][1]); // закрыть вывод в предыдущий pipe
		pipe(fd[!current]); // новый pipe между данным и следующим процессами
		pid = fork();
		if (pid == 0) {
			dup2(fd[current][0], 0); // ввод из предыдущего
			dup2(fd[!current][1], 1); // вывод в следующий
			conveer_call(commands[i]);
		}
		close(fd[current][0]); // закрыть ввод из предыдущего pipe
		current = !current;
	}
	waitd = wait(&status);
	close(fd[current][1]); // закрыть вывод в предыдущий pipe
	pid = fork();
	if (pid == 0) {
		int len = commands.size();
		dup2(fd[current][0], 0); // ввод из предыдущего, вывод стандартный
		conveer_call(commands[len - 1]); 
	}
	waitd = wait(&status);
	close(fd[current][0]); // закрыть ввод из предыдущего pipe
	return status;
}

//void control_c(int val) { }

int main() {
	//signal(2, control_c);
	char * s = (char *)malloc(123);
	vector<string> in;
	vector<vector<string> > com;
	do {
		in = input();
		while (in.size() == 0) in = input();
		if (in[0] == "ENDOFWORK") return 0;
		com = split_in(in);
		if (com.size() == 1) {
			single(com[0]);
		} else if (com.size() > 1) {
			conveer(com);
		}
	} while (true);
}

