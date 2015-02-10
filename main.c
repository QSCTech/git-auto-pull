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

#define BACKLOG 10

// Sockets stuff
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
int repo_cnt;
git_cred * my_cred;

void operate_git_pull(int i);

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
	json_object * j_default_user;
	json_object_object_get_ex(jobj, "default_user", &j_default_user);
	if (json_object_get_type(j_default_user) == json_type_string) {
		default_user = json_object_get_string(j_default_user);
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
		json_object * j_repo_name, * j_url, * j_branch, * j_path, * j_user, * j_after_pull;
		json_object_object_get_ex(j_repoa, "repo_name", &j_repo_name);
		json_object_object_get_ex(j_repoa, "url", &j_url);
		json_object_object_get_ex(j_repoa, "branch", &j_branch);
		json_object_object_get_ex(j_repoa, "path", &j_path);
		json_object_object_get_ex(j_repoa, "user", &j_user);
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
	my_addr.sin_port = htons(8081);
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
		if (rep_url_type != json_type_string) {
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
			if (strcmp(repo_name[i], sz_rep_name) == 0 && strcmp(match_ref[i], sz_ref) == 0 && strcmp(git_url[i], sz_rep_url) == 0) {
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
					if (setegid(pwd.pw_uid) != 0 && setgid(pwd.pw_gid) != 0) {
						printf("Change gid failed! \n");
						fprintf(stderr, "Change user to %s(%ld,%ld) failed for %s \n", git_user[i], pwd.pw_uid, pwd.pw_gid, repo_path[i]);
					}
					if (seteuid(pwd.pw_uid) != 0 && setuid(pwd.pw_uid) != 0) {
						printf("Change uid failed! \n");
						fprintf(stderr, "Change user to %s(%ld,%ld) failed for %s \n", git_user[i], pwd.pw_uid, pwd.pw_gid, repo_path[i]);
					}
					// Set environment: $HOME
					setenv("HOME", pwd.pw_dir, 1);
					printf("Set user HOME to %s\n", pwd.pw_dir);
				}
				chdir(repo_path[i]);

				operate_git_pull(i);
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

void operate_git_pull(int i) {
	char git_dir[1000];
	sprintf(git_dir, "%s/.git", repo_path[i]);
	git_repository * repo;
	int error = git_repository_open(&repo, git_dir);
	if (error < 0) {
		libgit2_handle_err(i);
		return;
	}
	char key_path[1000], pubkey_path[1000];
	sprintf(key_path, "/home/%s/.ssh/id_rsa", git_user[i]);
	sprintf(pubkey_path, "/home/%s/.ssh/id_rsa.pub", git_user[i]);
	error = git_cred_ssh_key_new(&my_cred, "git", pubkey_path, key_path, "");
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
	// 我也是别无他法 我也是醉了……
	char cmd[1000];
	sprintf(cmd, "git merge %s", remote_branch);
	system(cmd);
	system(scr_after_pull[i]);
	printf("Repository %s finished.\n", repo_name[i]);
	sleep(10); // Long time sleep
}

int main(int argc, char * argv[]) {
	char in[25000],  sent[500], code[50], file[200], mime[100], moved[200], length[100], auth[200], auth_dir[500], start[100], end[100];
	char *result=NULL, *hostname, *hostnamef, *lines, *ext=NULL, *extf, *auth_dirf=NULL, *authf=NULL, *rangetmp, *header, *headerval, *headerline, *realip;
	int buffer_length;
	char buffer[25000], headers[25000], charset[30], client_addr[32];
	char post_content[25000], org[25000], *po;
	long filesize, range=0, peername, i;

	if (geteuid() != 0) {
		fprintf(stderr, "Error: I need root privilege.\n");
		return 1;
	}
	git_libgit2_init();
	// Then set up daemon
	freopen(LOG_PATH, "a", stdout);
	freopen(LOG_ERR_PATH, "a", stderr);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	printf("%s: Version %s\n", argv[0], VERSION);
	if (read_conf()) {
		printf("Start error: cannot read config\n");
		fprintf(stderr, "Error: cannot read config\n");
		return 1;
	}
	daemon(0, 1);
	bind_port();
	int pid = getpid();
	FILE * fp;
	fp = fopen(PID_PATH, "w");
	if (fp == NULL) {
		fprintf(stderr, "Error writing pid file!\n");
	} else {
		fprintf(fp, "%d", pid);
		fclose(fp);
	}
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
			if (read(new_fd, in, 8000) == -1) {
				perror("receive");
			} else {
				strcpy(headers, in);
				strcpy(org, in);
				lines = strtok(in, "\n");
				result = strtok(lines, " ");
				result = strtok(NULL, " ");
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
					if (headerval != NULL && strcmp(header, proxy_header) == 0) {
						for (i = 0; i < proxy_cnt; i++) {
							if (strcmp(client_addr, proxies[i]) == 0) {
								realip = strdup(headerval);
								break;
							}
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
					for (i = 0; i < len - 4; i++) {
						if (org[i] == '\r' && org[i+1] == '\n' && org[i+2] == '\r' && org[i+3] == '\n') {
							find = 4;
							break;
						} else if (org[i] == '\n' && org[i+1] == '\n') {
							find = 2;
							break;
						}
					}
					if (find) {
						break;
					} else {
						read(new_fd, in, 8000);
						strcat(org, in);
					}
				}
				strcpy(code, "200 OK");
				if (!find) {
					printf("Requet body not found.\n");
					sprintf(buffer, "{\"err\":-1,\"server\":\"Senorsen's git-auto-pull\",\"version\":\"%s\",\"hint\":\"Method not supported\"}", VERSION);
				} else {
					printf("Request body found.\n");
					sprintf(buffer, "{\"err\":0,\"server\":\"Senorsen's git-a  uto-pull\",\"version\":\"%s\",\"hint\":\"Processing\"}", VERSION);
					po = (char *) malloc(sizeof(char) * 8000);
					int pos = i, j = 0;
					for (i = pos + find; i <= len; i++) {
						po[j++] = org[i];
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

