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

int run_child(void *arg)
{
	printf("Child with PID %d here !\n", getpid());
	return 0;
}

int main()
{
	printf("Parent with PID %d here!\n", getpid());

	void *pchild_stack = malloc(1024 * 1024);
	if (pchild_stack == NULL) {
		printf("Failed to allocate stack for child\n");
		return -1;
	}

	pid_t pid = clone(
		run_child,
		pchild_stack + 1024 * 1024,
		CLONE_NEWPID | SIGCHLD,
		NULL);
	if (pid == -1) {
		printf("Failed to clone process: %s\n", strerror(errno));
		return -1;
	}

	wait(NULL);
	free(pchild_stack);
	return 0;
}
