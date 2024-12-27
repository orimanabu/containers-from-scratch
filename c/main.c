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

#define STACK_SIZE 1024 * 1024
#define ROOTFS "/home/ori/ubuntu-rootfs"

struct cmd_arg {
	int argc;
	char **argv;
};

int into_miscns(void *arg)
{
	printf("Child2 (into_miscns): PID = %d, PPID = %d\n", getpid(), getppid());

	printf("* chroot\n");
	int ret_chroot = chroot(ROOTFS);
	if (ret_chroot == -1) {
		printf("chroot failed: %s\n", strerror(errno));
		return 1;
	}

	printf("* chdir /\n");
	int ret_chdir = chdir("/");
	if (ret_chdir == -1) {
		printf("chdir failed: %s\n", strerror(errno));
		return 1;
	}

	printf("* mount /proc\n");
	int ret_mount = mount("proc", "/proc", "proc", 0, NULL);
	if (ret_mount == -1) {
		printf("mount failed: %s\n", strerror(errno));
		return 1;
	}

	printf("* xxx 1\n");
	struct cmd_arg *cmdarg = (struct cmd_arg *)arg;
	printf("* xxx 1.1\n");
	char **p = (char **)malloc(sizeof(char *) * (cmdarg->argc + 1));
	printf("* xxx 1.2\n");
	const char *prog = cmdarg->argv[1];

	/*
	printf("* xxx 2\n");
	int i;
	for (i = 0; i < cmdarg->argc; i++) {
		printf("  %d: [%s]\n", i, cmdarg->argv[i+1]);
		p[i] = cmdarg->argv[i+1];
	}
	printf("* xxx 3\n");
	printf("XXX execv start: [%s]", prog);
	for (i = 0; i < cmdarg->argc-1; i++) {
		printf(" %s", p[i]);
	}
	printf("\n");

	int exec_ret = execv(prog, p);
	if (exec_ret == -1) {
		printf("execv failed: %s\n", strerror(errno));
		return 1;
	}
	printf("XXX execv end\n");
	*/

	execlp("bash", "bash", NULL);
	perror("execlp failed");
	return -1;

	return 0;
}

int into_userns(void *arg)
{
	struct cmd_arg *cmdarg = (struct cmd_arg *)arg;
	int i;

	printf("Child1 (into_userns): PID = %d, PPID = %d\n", getpid(), getppid());

	for (i = 0; i < cmdarg->argc; i++) {
		printf("  %d: [%s]\n", i, cmdarg->argv[i]);
	}

	const char *uid_map = "0 1000 1\n";
	const char *gid_map = "0 1000 1\n";
	FILE *uid_map_file, *gid_map_file;

	if ((uid_map_file = fopen("/proc/self/uid_map", "w")) == NULL) {
		perror("Failed to open /proc/self/uid_map");
		return -1;
	}
	if (fwrite(uid_map, 1, strlen(uid_map), uid_map_file) != strlen(uid_map)) {
		perror("Failed to write to /proc/self/uid_map");
		fclose(uid_map_file);
		return -1;
	}
	fclose(uid_map_file);

	if ((gid_map_file = fopen("/proc/self/gid_map", "w")) == NULL) {
		perror("Failed to open /proc/self/gid_map");
		return -1;
	}
	if (fwrite(gid_map, 1, strlen(gid_map), gid_map_file) != strlen(gid_map)) {
		perror("Failed to write to /proc/self/gid_map");
		fclose(gid_map_file);
		return -1;
	}
	fclose(gid_map_file);

	printf("UID and GID mappings applied\n");
	printf("Now running as unprivileged user\n");

	char *stack2 = malloc(STACK_SIZE);
	if (!stack2) {
	    perror("malloc for stack2");
	    exit(EXIT_FAILURE);
	}

	pid_t pid2 = clone(into_miscns, stack2 + STACK_SIZE, CLONE_NEWUTS|CLONE_NEWPID|CLONE_NEWNS|SIGCHLD, (void *)cmdarg);
	if (pid2 == -1) {
	    perror("clone child2");
	    exit(EXIT_FAILURE);
	}

	//waitpid(pid2, NULL, 0);
	wait(NULL);
	free(stack2);

	printf("Child1: Exiting after Child2.\n");
	return 0;
}

int main(int argc, char *argv[])
{
    struct cmd_arg cmdarg = {argc, argv};

    char *stack1 = malloc(STACK_SIZE);
    if (!stack1) {
	    perror("malloc for stack1");
	    exit(EXIT_FAILURE);
    }

    pid_t pid1 = clone(into_userns, stack1 + STACK_SIZE, CLONE_NEWUSER|SIGCHLD, (void *)&cmdarg);
    if (pid1 == -1) {
	    perror("clone child1");
	    exit(EXIT_FAILURE);
    }

    //waitpid(pid1, NULL, 0);
    wait(NULL);
    free(stack1);

    printf("Parent: children have exited.\n");
    return 0;
}
