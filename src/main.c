/*
	Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <json-c/json.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <git2.h>
#include <pwd.h>

#ifndef CONF_PATH
#define CONF_PATH "/etc/git-auto-pull/config.json"
#endif
#ifndef LOG_PATH
#define LOG_PATH "/var/log/git-auto-pull/log.log"
#endif
#ifndef LOG_ERR_PATH
#define LOG_ERR_PATH "/var/log/git-auto-pull/err.log"
#endif
#ifndef PID_PATH
#define PID_PATH "/var/run/git-auto-pull.pid"
#endif
#ifndef VERSION
#define VERSION "unknown"
#endif

#define BACKLOG 100
#define DEFAULT_PORT 8081

// Color text in terminal - see: 
//   http://stackoverflow.com/questions/3585846/color-text-in-terminal-aplications-in-unix
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

int debug_flag;

// Sockets stuff
uint16_t listen_port;
int sockfd, new_fd;
struct sockaddr_in their_addr;
socklen_t sin_size;
struct sigaction sa;

const char * proxy_header;
const char * proxies[100];
int proxy_cnt;

// GitLab Repositories stuff
const char * fetch_branch[1000];
const char * match_ref[1000];
const char * git_url[1000];
const char * repo_name[1000];
const char * repo_path[1000];
const char * git_user[1000];
const char * scr_after_pull[1000];
const char * key_path[1000];
int repo_cnt;
git_cred * my_cred;

void operate_git_pull(int i, int iwhtype, char * user_home);

void sigchld_handler(int s) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void printerr(const char * format, ...) {
	va_list arglist;
	printf("Error: ");
	fprintf(stderr, "Error: ");
	va_start(arglist, format);
	vprintf(format, arglist);
	vfprintf(stderr, format, arglist);
	va_end(arglist);
	printf("\n");
	fprintf(stderr, "\n");
}

int read_conf() {
	char conf_str[10240];
	char * default_key_path = NULL;
	int i ,arr_len;
	FILE * fp = fopen(CONF_PATH, "r");
	if (fp == NULL) {
		return 1;
	}
	fread(conf_str, 8192, 1, fp);
	json_object * jobj = json_tokener_parse(conf_str);
	enum json_type type = json_object_get_type(jobj);
	if (type != json_type_object) {
		return 1;
	}
	const char * default_user = NULL;
	json_object * j_port;
	json_object_object_get_ex(jobj, "port", &j_port);
	if (json_object_get_type(j_port) == json_type_int) {
		listen_port = json_object_get_int(j_port);
	} else if (json_object_get_type(j_port) == json_type_null) {
		listen_port = DEFAULT_PORT;
	} else {
		printerr("error: port can only be json_type_int.");
		return 1;
	}
	json_object * j_default_user;
	json_object_object_get_ex(jobj, "default_user", &j_default_user);
	if (json_object_get_type(j_default_user) == json_type_string) {
		default_user = json_object_get_string(j_default_user);
	}
	json_object * j_default_key;
	json_object_object_get_ex(jobj, "default_key", &j_default_key);
	if (json_object_get_type(j_default_key) == json_type_string) {
		default_key_path = json_object_get_string(j_default_key);
	}
	json_object * j_proxies;
	json_object_object_get_ex(jobj, "proxy", &j_proxies);
	if (json_object_get_type(j_proxies) == json_type_array) {
		arr_len = json_object_array_length(j_proxies);
		proxy_cnt = arr_len;
		for (i = 0; i < arr_len; i++) {
			json_object * j_proxy = json_object_array_get_idx(j_proxies, i);
			if (json_object_get_type(j_proxy) == json_type_string) {
				proxies[i] = json_object_get_string(j_proxy);
			} else {
				printerr("wrong syntax, there should be strings in proxy array");
				return 1;
			}
		}
	} else if (json_object_get_type(j_proxies) == json_type_string) {
		proxy_cnt = 1;
		proxies[0] = json_object_get_string(j_proxies);
	} else if (json_object_get_type(j_proxies) != json_type_null) {
		printerr("proxy type error, should be string or array of strings");
		return 1;
	}
	json_object * j_proxy_header;
	json_object_object_get_ex(jobj, "proxy_header", &j_proxy_header);
	proxy_header = "X-Real-IP";
	if (json_object_get_type(j_proxy_header) == json_type_string) {
		proxy_header = json_object_get_string(j_proxy_header);
	} else if (json_object_get_type(j_proxy_header) == json_type_null) {
		proxy_header = "X-Real-IP";
	} else {
		printerr("proxy_header should be string or null");
		return 1;
	}
	json_object * j_repo;
	json_object_object_get_ex(jobj, "repo", &j_repo);
	if (json_object_get_type(j_repo) != json_type_array) {
		return 1;
	}
	arr_len = json_object_array_length(j_repo);
	repo_cnt = arr_len;
	if (repo_cnt > 1000) {
		printerr("too many repos! I can only handle 1000.");
		return 1;
	}
	for (i = 0; i < arr_len; i++) {
		json_object * j_repoa = json_object_array_get_idx(j_repo, i);
		json_object * j_repo_name, * j_url, * j_branch, * j_path, * j_user, * j_key, * j_after_pull;
		json_object_object_get_ex(j_repoa, "repo_name", &j_repo_name);
		json_object_object_get_ex(j_repoa, "url", &j_url);
		json_object_object_get_ex(j_repoa, "branch", &j_branch);
		json_object_object_get_ex(j_repoa, "path", &j_path);
		json_object_object_get_ex(j_repoa, "user", &j_user);
		json_object_object_get_ex(j_repoa, "key", &j_key);
		json_object_object_get_ex(j_repoa, "after_pull", &j_after_pull);
		if (json_object_get_type(j_branch) != json_type_string) {
			printerr("field branch should be a valid string");
			return 1;
		}
		fetch_branch[i] = json_object_get_string(j_branch);
		if (strlen(fetch_branch[i]) > 255) {
			printerr("field branch: string length exceeded, max length is 255");
			return 1;
		}
		match_ref[i] = (const char *) malloc(1024);
		sprintf((char *) match_ref[i], "refs/heads/%s", fetch_branch[i]);
		if (json_object_get_type(j_url) != json_type_string) {
			printerr("field url should be a valid string");
			return 1;
		}
		git_url[i] = json_object_get_string(j_url);
		if (strlen(git_url[i]) > 255) {
			printerr("field url: string length exceeded, max length is 255");
			return 1;
		}
		if (json_object_get_type(j_repo_name) != json_type_string) {
			printerr("field repo_name should be a valid string");
			return 1;
		}
		repo_name[i] = json_object_get_string(j_repo_name);
		if (strlen(repo_name[i]) > 255) {
			printerr("field repo_name: string length exceeded, max length is 255");
			return 1;
		}
		if (json_object_get_type(j_path) != json_type_string) {
			printerr("field path should be a valid string");
			return 1;
		}
		repo_path[i] = json_object_get_string(j_path);
		if (strlen(repo_path[i]) > 255) {
			printerr("field path: string length exceeded, max length is 255");
			return 1;
		}
		if (json_object_get_type(j_user) != json_type_string) {
			if (default_user != NULL) {
				git_user[i] = default_user;
			} else {
				printerr("field user should be a valid string");
				return 1;
			}
		} else {
			git_user[i] = json_object_get_string(j_user);
		}
		if (strlen(git_user[i]) > 64) {
			printerr("field user: string length exceeded, max length is 64");
			return 1;
		}
		if (json_object_get_type(j_key) != json_type_string) {
			key_path[i] = default_key_path;
		} else {
			key_path[i] = json_object_get_string(j_key);
		}
		if (json_object_get_type(j_after_pull) != json_type_string) {
			// Optional
			scr_after_pull[i] = "";
		} else {
			scr_after_pull[i] = json_object_get_string(j_after_pull);
			if (strlen(scr_after_pull[i]) > 2048) {
				printerr("field after_pull: string length exceeded. max length is 2048");
				return 1;
			}
		}
		printf("Read %d: Url: %s\n", i, git_url[i]);
	}
	return 0;
}

void bind_port() {
	int yes = 1;
	struct sockaddr_in my_addr;

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(listen_port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

void parse_post_obj(char * str, char * realip) {

	json_object * jobj = json_tokener_parse(str);
	enum json_type type = json_object_get_type(jobj);

	struct passwd pwd;
	struct passwd *result;
	char *buf;
	size_t bufsize;
	int s;
	int iwhtype = 1;
	char * whtype = "GitLab";

	if (type == json_type_object) {
		// Correct #1
		json_object * ref;
		json_object_object_get_ex(jobj, "ref", &ref);
		enum json_type ref_type = json_object_get_type(ref);
		json_object * repository;
		json_object_object_get_ex(jobj, "repository", &repository);
		enum json_type repository_type = json_object_get_type(repository);
		json_object * user_name;
		json_object_object_get_ex(jobj, "user_name", &user_name);
		enum json_type user_name_type = json_object_get_type(user_name);
		if (user_name_type != json_type_string) {
			// GitHub Webhook compatible
			whtype = "GitHub";
			iwhtype = 2;
			json_object * jpusher;
			json_object_object_get_ex(jobj, "pusher", &jpusher);
			json_object_object_get_ex(jpusher, "name", &user_name);
			user_name_type = json_object_get_type(user_name);
		}
		if (ref_type != json_type_string || repository_type != json_type_object || user_name_type != json_type_string) return;
		json_object * rep_name;
		json_object_object_get_ex(repository, "name", &rep_name);
		json_object * rep_url;
		json_object_object_get_ex(repository, "url", &rep_url);
		enum json_type rep_name_type = json_object_get_type(rep_name);
		enum json_type rep_url_type = json_object_get_type(rep_url);
		if (strcmp(whtype, "GitHub") == 0 || rep_url_type != json_type_string) {
			// GitHub Webhook compatible
			json_object_object_get_ex(repository, "ssh_url", &rep_url);
			rep_url_type = json_object_get_type(rep_url);
		}
		if (rep_name_type != json_type_string || rep_url_type != json_type_string) return;
		const char * sz_ref = json_object_get_string(ref);
		const char * sz_rep_url = json_object_get_string(rep_url);
		const char * sz_rep_name = json_object_get_string(rep_name);
		const char * sz_user_name = json_object_get_string(user_name);
		printf("[%s] %s (%s) by %s\n", whtype, sz_rep_name, sz_rep_url, sz_user_name);
		int i;
		for (i = 0; i < repo_cnt; i++) {
			if (strcmp(repo_name[i], sz_rep_name) == 0 && strcmp(match_ref[i], sz_ref) == 0) {
				// That's it
				printf("[Auto Pull] IP: %s, ID: %d, Url: %s\n", realip, i, git_url[i]);
				//char cmd[1024];
				//sprintf(cmd, "cd \"%s\"; sudo -u %s -H git --work-tree=\"%s\" --git-dir=\"%s/.git\" pull origin %s --force", repo_path[i], git_user[i], repo_path[i], repo_path[i], fetch_branch[i]);
				//system(cmd);
				bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
				if (bufsize == -1) bufsize = 16384;
				buf = (char *) malloc(bufsize);
				if (buf == NULL) {
					perror("malloc");
					exit(EXIT_FAILURE);
				}
				s = getpwnam_r(git_user[i], &pwd, buf, bufsize, &result);
				if (result == NULL) {
					if (s == 0) {
						printf("ERROR: user %s not found in passwd entry.\n", git_user[i]);
						fprintf(stderr, "ERROR: user %s not found in passwd entry.\n", git_user[i]);
					} else {
						errno = s;
						printf("Find user %s failed\n", git_user[i]);
						perror("getpwnam_r");
					}
				} else {
					printf("Change user: %s(%ld,%ld)\n", git_user[i], (long) pwd.pw_uid, (long) pwd.pw_gid);
					if (setgid(pwd.pw_gid) != 0) {
						printf("Change gid failed! \n");
						fprintf(stderr, "Change user to %s(%ld,%ld) failed for %s \n", git_user[i], pwd.pw_uid, pwd.pw_gid, repo_path[i]);
					}
					if (setuid(pwd.pw_uid) != 0) {
						printf("Change uid failed! \n");
						fprintf(stderr, "Change user to %s(%ld,%ld) failed for %s \n", git_user[i], pwd.pw_uid, pwd.pw_gid, repo_path[i]);
					}
					// Set environment: $HOME
					setenv("HOME", pwd.pw_dir, 1);
					printf("Set user HOME to %s\n", pwd.pw_dir);
				}
				chdir(repo_path[i]);

				operate_git_pull(i, iwhtype, pwd.pw_dir);
				break;
			}
		}
	} else {
		printf("Not a JSON\n");
	}
}

void libgit2_handle_err(int i) {
	const git_error * e = giterr_last();
	printf("ERROR: %s %d %s\n", repo_name[i], e->klass, e->message);
	fprintf(stderr, "ERROR: %s %d %s\n", repo_name[i], e->klass, e->message);
}

int operate_get_cred(git_cred **cred, const char *url, const char *user_from_url, unsigned int allowed_types, void *payload) {
	*cred = my_cred;
	return 0;
}

void operate_git_pull(int i, int iwhtype, char * user_home) {
	char git_dir[1000];
	sprintf(git_dir, "%s/.git", repo_path[i]);
	git_repository * repo;
	int error = git_repository_open(&repo, git_dir);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	char this_key_path[1000], this_pubkey_path[1000];
	if (key_path[i] == NULL) {
		sprintf(this_key_path, "%s/.ssh/id_rsa", user_home);
		sprintf(this_pubkey_path, "%s/.ssh/id_rsa.pub", user_home);
		error = git_cred_ssh_key_new(&my_cred, "git", this_pubkey_path, this_key_path, "");
	} else {
		sprintf(this_pubkey_path, "%s.pub", key_path[i]);
		error = git_cred_ssh_key_new(&my_cred, "git", this_pubkey_path, key_path[i], "");
	}
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	git_remote * remote = NULL;
	error = git_remote_lookup(&remote, repo, "origin");
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	if (iwhtype == 2) {
		// from GitHub
		// Set CA Cert Path to Our /usr/share directory, 
		//   to avoid of certificate errors. 
		git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS, "/usr/share/git-auto-pull/github.com.crt", NULL);
	}
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	callbacks.credentials = operate_get_cred;
	error = git_remote_set_callbacks(remote, &callbacks);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	error = git_remote_fetch(remote, NULL, NULL, NULL);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	git_oid expected_id;
	git_reference * myref;
	error = git_repository_head(&myref, repo);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	const char * my_branch_name;
	error = git_branch_name(&my_branch_name, myref);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	printf("%s currently works on %s\n", repo_name[i], my_branch_name);
	char remote_branch[1000];
	sprintf(remote_branch, "origin/%s", fetch_branch[i]);
	error = git_branch_lookup(&myref, repo, remote_branch, GIT_BRANCH_REMOTE);
	if (error < 0) {
		printf("git_branch_lookup ");
		libgit2_handle_err(i);
		return;
	}
	if (error == GIT_ENOTFOUND) {
		printf("git_branch_lookup not found\n");
	}
	git_oid oid;
	char remote_ref[1000];
	sprintf(remote_ref, "refs/remotes/origin/%s", fetch_branch[i]);
	error = git_reference_name_to_id(&oid, repo, "HEAD");
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	if (error == GIT_ENOTFOUND) {
		printf("git_reference_name_to_id not found\n");
	}
	if (error == GIT_EINVALIDSPEC) {
		printf("git_reference_name_to_id GIT_EINVALIDSPEC\n");
	}
	git_annotated_commit * myhead;
	error = git_annotated_commit_from_fetchhead(&myhead, repo, remote_branch, git_url[i], &oid);
	if (error) {
		libgit2_handle_err(i);
		return;
	}
	const git_annotated_commit * their_heads[2] = {myhead};
	git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
	git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
	merge_opts.file_favor = GIT_MERGE_FILE_FAVOR_OURS;
	
/*
	error = git_merge(repo, their_heads, 1, &merge_opts, &checkout_opts);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
*/
	git_annotated_commit_free(myhead);
	// Dirty hack!!
	char cmd[1000];
	sprintf(cmd, "git merge %s", remote_branch);
	system(cmd);
	system(scr_after_pull[i]);
	printf("Repository %s finished.\n", repo_name[i]);
	sleep(10); // Long time sleep
}

int socket_select(fd) {
	fd_set read_set;
	struct timeval timeout;

	timeout.tv_sec = 20;
	timeout.tv_usec = 0;

	FD_ZERO(&read_set);
	FD_SET(fd, &read_set);

	int ret = select(fd + 1, &read_set, NULL, NULL, &timeout);

	if (ret < 0) {
		printerr("socket_select error: %d", ret);
		exit(1);
	}

	if (ret == 0) {
		return 1;
	}

	if (ret == 1) {
		return 0;
	}
}

// From: http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
void segfault_handler(int sig) {
    void *array[10];
    size_t size;

    size = backgtrace(array, 10);

    fprintf(stderr, "Error: signal %d: \n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(int argc, char * argv[]) {
	char in[25000],  sent[500], code[50], file[200], mime[100], moved[200], length[100], auth[200], auth_dir[500], start[100], end[100];
	char *result=NULL, *hostname, *hostnamef, *lines, *ext=NULL, *extf, *auth_dirf=NULL, *authf=NULL, *rangetmp, *header, *headerval, *headerline, *realip;
	int buffer_length;
	char buffer[25000], headers[25000], charset[30], client_addr[32];
	char post_content[25000], org[34000], *po;
	long filesize, range=0, peername, i, req_content_length, bytes_len;

    signal(SIGSEGV, segfault_handler);

	if (argc >= 2 && (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--debug") == 0)) {
		debug_flag = 1;
	} else {
		debug_flag = 0;
	}

	if (argc >= 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
		printf("git-auto-pull %s\n", VERSION);
		return 0;
	}

	if (argc >= 2 && (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0)) {
		printf("git-auto-pull is testing your config... \n");
		if (read_conf()) {
			printf(" > Test " KRED "ERROR" RESET "!! \n");
			return 1;
		} else {
			printf(" > Test " KGRN "OK" RESET "\n");
			return 0;
		}
	}

	if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		printf("git-auto-pull %s\n", VERSION);
		printf("Usage:	%s [OPTION...]\n\n", argv[0]);
		printf("Help Options:\n");
		printf("  -h, --help                   Show help options\n");
		printf("  -v, --version                Show program version\n");
		printf("\n");
		printf("Developer Options:\n");
		printf("  -d, --debug                  Log debug info into log\n");
		printf("  -t, --test                   Test config syntax and exit\n");
		printf("\n");
		return 0;
	}

	if (geteuid() != 0) {
		fprintf(stderr, "Error: I need root privilege.\n");
		return 1;
	}
	git_libgit2_init();
	if (!debug_flag) {
		// Then set up daemon
		freopen(LOG_PATH, "a", stdout);
		freopen(LOG_ERR_PATH, "a", stderr);
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);
    	daemon(0, 1);
		int pid = getpid();
    	FILE * fp;
    	fp = fopen(PID_PATH, "w");
    	if (fp == NULL) {
    		fprintf(stderr, "Error writing pid file!\n");
    	} else {
    		fprintf(fp, "%d", pid);
    		fclose(fp);
    	}
	}
	printf("%s %s\n", argv[0], VERSION);
	if (read_conf()) {
		printerr("Start error: cannot read config");
		return 1;
	}
	bind_port();
	while (1) {
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));

		strcpy(client_addr, inet_ntoa(their_addr.sin_addr));
		realip = client_addr;
		if (!fork()) {
			close(sockfd);
			int find = 0;
			bytes_len = read(new_fd, in, 8000);
			if (bytes_len == -1) {
				perror("receive");
			} else {
				strcpy(headers, in);
				strcpy(org, in);
				lines = strtok(in, "\n");
				result = strtok(lines, " ");
				result = strtok(NULL, " ");
				req_content_length = 0;
				// Read proxy_header
				char * tokr1, * tokr2;
				headerline = strtok_r(headers, "\n", &tokr1);
				do {
					header = strtok_r(headerline, ":", &tokr2);
					headerval = strtok_r(NULL, ":", &tokr2);
					if (headerval != NULL) {
						if (headerval[strlen(headerval) - 1] == '\r') {
							headerval[strlen(headerval) - 1] = 0;
						}
						if (headerval[0] == ' ') headerval++;
					}

					if (debug_flag && headerval && header) {
						printf("HTTP Header: %s %s\n", header, headerval);
					}

					if (headerval != NULL && strcmp(header, proxy_header) == 0) {
						for (i = 0; i < proxy_cnt; i++) {
							if (strcmp(client_addr, proxies[i]) == 0) {
								realip = strdup(headerval);
								break;
							}
						}
					}
					if (headerval != NULL && strcmp(header, "Content-Length") == 0) {
						req_content_length = atoi(headerval);
						if (debug_flag) {
							printf("req_content_length = %d\n", req_content_length);
						}
					}
					headerline = strtok_r(NULL, "\n", &tokr1);
				} while (headerline != NULL);
				char tmpstr[3000];
				int len;
				strcpy(file, result);
				printf("[Request] %s %s\n", realip, file);
				int try = 3;
				while (!find && --try) {
					if (org[0] == 'G') {
						// GET?
						find = 0;
						break;
					}
					len = strlen(org);
					for (i = 0; i < len - 4 && i < bytes_len; i++) {
						if (org[i] == '\r' && org[i+1] == '\n' && org[i+2] == '\r' && org[i+3] == '\n') {
							find = 4;
							break;
						} else if (org[i] == '\n' && org[i+1] == '\n') {
							find = 2;
							break;
						}
					}
					if (find) {
						int start_fetch_time = time(NULL);
						while (strlen(org) - i + find < req_content_length) {
							if (time(NULL) - start_fetch_time > 120) {
								printerr("Timeout, req_content_length: %d, current: %d", req_content_length, strlen(org));
							}
							in[0] = 0;
							int nbytes = read(new_fd, org + bytes_len, 8000);
							if (nbytes < 0) {
								printerr("1-Read error");
								return 1;
							}
							if (nbytes == 0) continue;
							bytes_len += nbytes;
							if (bytes_len >= 34000) {
								printerr("Entity too large");
								return 1;
							}
							if (debug_flag) {
								printf("1-bytes_len: %d\n", bytes_len);
							}
						}
						break;
					} else {
						in[0] = 0;
						int nbytes = read(new_fd, org + bytes_len, 8000);
						if (nbytes < 0) {
							printerr("2-Read error");
							return 1;
						}
						if (nbytes == 0) continue;
						bytes_len += nbytes;
						if (debug_flag) {
							printf("2-bytes_len: %d\n", bytes_len);
						}
					}
				}
				
				if (debug_flag) {
					printf("HTTP Data: %s\n", org);
				}

				strcpy(code, "200 OK");
				if (!find) {
					printf("Requet body not found.\n");
					sprintf(buffer, "{\"err\":-1,\"server\":\"Senorsen's git-auto-pull\",\"version\":\"%s\",\"hint\":\"Method not supported\"}", VERSION);
				} else {
					printf("Request body found.\n");
					sprintf(buffer, "{\"err\":0,\"server\":\"Senorsen's git-a  uto-pull\",\"version\":\"%s\",\"hint\":\"Processing\"}", VERSION);
					po = (char *) malloc(sizeof(char) * 34000);
					int pos = i + find, j = 0;
					for (i = pos; i <= bytes_len; i++) {
						po[j++] = org[i];
					}
					po[j] = 0;
					if (debug_flag) {
						printf("HTTP Data now: %s\n", org);
						printf("JSON: %s\n", po);
						printf("JSON pos: %d, bytes_len: %d\n", pos, bytes_len);
					}
				}
			}
			buffer_length = strlen(buffer);
			sprintf(length, "%d", buffer_length);
			strcpy(charset, "utf-8");
			strcpy(mime, "application/json");
			strcpy(sent, "HTTP/1.0 ");
			strcat(sent, code);
			sprintf(sent, "%s\nServer: Senorsen's git-auto-pull %s\n", sent, VERSION);
			strcat(sent, "Content-Length: ");
			strcat(sent, length);
			strcat(sent, "\nConnection: close\nContent-Type: ");
			strcat(sent, mime);
			strcat(sent, "; charset=");
			strcat(sent, charset);
			strcat(sent, "\n\n");
			write(new_fd, sent, strlen(sent));
			write(new_fd, buffer, 1024);
			if (find) {
				parse_post_obj(po, realip);
			}
			sleep(1);	// Wait for 1s in case of ECONNRESET
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}

