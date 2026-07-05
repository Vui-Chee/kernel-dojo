// SPDX-License-Identifier: GPL-2.0
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define PROC_FILE "/proc/vm_profiler/status"

int write_message(char op, int pid)
{
	char buf[50];
	int len = 0;

	switch (op) {
	case 'R':
		len = snprintf(buf, sizeof(buf), "R,%d", pid);
		break;
	case 'U':
		len = snprintf(buf, sizeof(buf), "U,%d", pid);
		break;
	default:
		fprintf(stderr, "Unknown operation: %c\n", op);
	}

	int fd = open(PROC_FILE, O_WRONLY);

	if (fd < 0) {
		fprintf(stderr, "Failed to open file: %s\n", PROC_FILE);
		return -1;
	}

	int ret = write(fd, buf, len);

	close(fd);
	return ret;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <operation>\n", argv[0]);
		return 1;
	}

	char op = argv[1][0];

	pid_t pid = getpid();

	switch (op) {
	case 'r':
	case 'R':
		write_message('R', pid);
		break;
	case 'u':
	case 'U':
		write_message('U', pid);
		break;
	default:
	}

	return 0;
}
