#define _GNU_SOURCE

#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>

int run_child(void *arg)
{
	char *filename = (char *)arg;
	char *argv[] = {filename, NULL};

	int exec_ret = execv(filename, argv);
	if (exec_ret == -1) {
		printf("execv failed %s\n", strerror(errno));
		exit(1);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	void *pchild_stack = malloc(1024 * 1024);
	if (pchild_stack == NULL) {
		printf("Failed to allocate stack for child\n");
		return -1;
	}

	if (argc < 2) {
		printf("Usage: %s <command>\n", argv[0]);
		return -1;
	}

	pid_t pid = clone(
		run_child,
		pchild_stack + 1024 * 1024,
		CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
		argv[1]);

	if (pid == -1) {
		printf("Failed to clone process: %s\n", strerror(errno));
		return -1;
	}

	wait(NULL);
	free(pchild_stack);
	return 0;
}
