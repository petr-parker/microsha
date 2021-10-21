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

using namespace std;

#include "help.cc"

void walk_recursive(string const &dirname, vector<string> &ret) {
    DIR *dir = opendir(dirname.c_str());
    if (dir == nullptr) {
        perror(dirname.c_str());
        return;
    }
    for (dirent *de = readdir(dir); de != NULL; de = readdir(dir)) {
        if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) continue; // не берём . и ..
        ret.push_back(dirname + "/" + de->d_name); // добавление в вектор
        if (de->d_type == DT_DIR) {
            walk_recursive(dirname + "/" + de->d_name, ret);
        }
    }
    closedir(dir);
}

vector<string> walk(string const &dirname) {
    vector<string> ret;
    walk_recursive(dirname, ret);
    return ret;
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
	write(1, "\n", sizeof("\n"));;
}

void meta(vector<string> commands) {
	int i_meta;
	for (int i = 0; i < commands.size(); i++) {
		if (is_meta(commands[i])) {
			i_meta = i;
			break;
		}
	}
	vector<string> all;
	vector<string> comm = commands;
	if (commands[i_meta][0] == '/') {
		const char * base = "/";
		all = walk(base);
	} else {
		const char * base = ".";
		all = walk(base);
	}
	vector<string> v_meta;
	vector<string> v;
	string replace;
	for (int i = 0; i < all.size(); i++) {
		v_meta = split(commands[i_meta], '/');
		v = split(all[i], '/');
		if  (v[0] == ".") v.erase(v.begin(), v.begin() + 1);
		if  (v_meta[0] == ".") v_meta.erase(v_meta.begin(), v_meta.begin() + 1);
		if (v_metacmp(v, v_meta)) {
			replace = join(v, "/");
			comm[i_meta] = replace; 
			pid_t pid = fork();
			if (pid == 0) {
			    vector<char *> c_comm = v_c_str(comm);
			    execvp(c_comm[0], &c_comm[0]);
			} else {
			    int status;
			    pid_t waitd = wait(&status);
			}
		}
	}	
}

void meta_io(vector<string> commands) {
	int i_in = -1;
	int i_out = -1;
	for (int i = 1; i < commands.size(); i++) {
		if (commands[i] == "<") {
			if (i != -1) {
				perror("Too many <");
				return;
			}
			i_in = i;
		}
		if (commands[i] == ">") {
			if (i != -1) {
    			perror("Too many >");
				return;
			}
			i_out = i;
		}
	}
	int in = 0;
	int out = 1;
	if (i_in != -1) {
		// in = open("foo", O_RDWR | O_CREAT | O_EXCL, 0666);
	}
	if (i_out != 1) {
		// out = open("foo", O_RDWR | O_CREAT | O_EXCL, 0666);
	}

	int fd[2];
	int status;
	pid_t waitd;

    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], in);

	}
	waitd = wait(&status);
	close(fd[1]); // закрыть вывод в предыдущий pipe
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
	    } else {
	        int status;
	        pid_t waitd = wait(&status);
	    }   
	}
}

void single(vector<string> commands) {
	if (commands[0] == "cd") {
    	change_dir(commands);
	} else if (commands[0] == "pwd") {
		pwd(commands);
	} else {
		call(commands);
	}
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

int main() {
	/*
	string a;
	vector<string> v;
	for (int i = 0; i < 5; i++) {
		cin >> a;
		v.push_back(a);

	}
	cout << join(v, "==") << endl;

	vector<string> v_m;
	for (int i = 0; i < 5; i++) {
	    cin >> a;
	    v_m.push_back(a);
	}
	cout << join(v_m, "==") << endl;

	cout << v_metacmp(v, v_m) << endl;
	*/
	
	vector<string> in;
	vector<vector<string> > com;
	do {
		in = input();
		com = split_in(in);
		if (com.size() == 1) {
			single(com[0]);
		} else if (com.size() > 1) {
			conveer(com);
		}
	} while (in.size() > 0);
	
}

