vector<string> split(string line, char splitter) {
	int i = 0;
	vector<string> ret;
	string s;
	while (i < line.size()) {
		while (i < line.size() && line[i] != splitter) {
			s = s + line[i];
			i++;
		}
		if (s != "") {
			ret.push_back(s);
			s = "";
		}
		i++;
	}
	return ret;
}

string join(vector<string> v, string joiner) {
	string ret = v[0];
	for (int i = 1; i < v.size(); i++) {
		ret = ret + joiner + v[i];
	}
	return ret;
}

void baseline() {
	char path[100];
	getcwd(path, 100);
	vector<string> dirs = split(string(path), '/');
	string curdir;
	if (dirs.size() == 0) {
		curdir = "/";
	} else {
		curdir = dirs[dirs.size() - 1];
	}
	size_t uid = geteuid();
	char is_su = uid ? '>' : '!';
	printf("%s %s %c ", getenv("USER"), curdir.c_str(), is_su);
}

vector<string> input() {
	vector<string> ret;
	string s;
	int i = 0;
	char c = ' ';
	baseline();
	while (c != '\n') {
		scanf("%c", &c);
		while (c != ' ' && c != '\n') {
			s = s + c;
			scanf("%c", &c);
		}
		if (s != "") {
			ret.push_back(s);
			s = "";
		}
	}
	return ret;
}

vector<vector<string> > split_in(vector<string> in) {
	vector<vector<string> > res;
	vector<string> v;
	int i = 0;
	while (i < in.size()) {
		while (i < in.size() && in[i] != "|") {
			v.push_back(in[i]);
			i++;
		}
		res.push_back(v);
		v.clear();
		i++;
	}
	return res;
}

int metacmp(string line, string metaline) {
	if (line == "" && metaline == "") return 1;
	if (line != "" && metaline == "") return 0;
	if (metaline[0] == '?') {
		if (line.size() == 0) { return 0; }
		else { return (metacmp(line.substr(1), metaline.substr(1))); }
	}
	if (metaline[0] == '*') {
		for (int i = 0; i <= line.size(); i++) {
			if (metacmp(line.substr(i), metaline.substr(1))) {
				return 1;
			}
		}
		return 0;
	}
	if (metaline[0] == line[0]) {
		return (metacmp(line.substr(1), metaline.substr(1)));
	} else {
		return 0;
	}
}

int v_metacmp(vector<string> v, vector<string> v_meta) {
	if (v.size() != v_meta.size()) { return 0; }
	for (int i = 0; i < v.size(); i++) {
		if (metacmp(v[i], v_meta[i]) == 0) {
			return 0;
		}
	}
	return 1;
}

vector<vector<char *> > vv_c_str(vector<vector<string> > &vv) {
	vector<vector<char *> > ret(vv.size());
	for (int i = 0; i < vv.size(); i++) {
		for (int j = 0; j < vv[i].size(); j++) {
			ret[i].push_back((char *)(vv[i][j].c_str()));
		}
		ret[i].push_back(nullptr);
	}
	return ret;
}

vector<char *> v_c_str(vector<string> &v) {
	vector<char *> ret;
	for (int i = 0; i < v.size(); i++) {
		ret.push_back((char *)(v[i].c_str())); 
	}
	ret.push_back(nullptr);
	return ret;
}

int is_meta(string line) {
	for (int i = 0; i < line.size(); i++) {
		if (line[i] == '*' || line[i] == '?') {
			return 1;
		}
	}
	return 0;
}

int v_is_meta(vector<string> v) {
	for (int i = 0; i < v.size(); i++) {
		if (is_meta(v[i])) {
			return 1;
		}
	}
	return 0;
}

int v_is_meta_io(vector<string> v) {
	for (int i = 0; i < v.size(); i++) {
		if (v[i] == ">" || v[i] == "<") {
			return 1;
		}
	}
	return 0;
}

void walk_recursive(string const &dirname, vector<string> &ret, int depth) {
	DIR *dir = opendir(dirname.c_str());
	if (dir == nullptr) {
		// perror(dirname.c_str());
		return;
	}
	for (dirent *de = readdir(dir); de != NULL; de = readdir(dir)) {
		if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) continue; // не берём . и ..
		ret.push_back(dirname + "/" + de->d_name); // добавление в вектор
		if (de->d_type == DT_DIR && depth > 1) {
			walk_recursive(dirname + "/" + de->d_name, ret, depth - 1);
		}
	}
	closedir(dir);
}

vector<string> walk(string const &dirname, int depth) {
	vector<string> ret;
	walk_recursive(dirname, ret, depth);
	return ret;
}

