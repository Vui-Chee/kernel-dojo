// SPDX-License-Identifier: GPL-2.0
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

int register_job(int pid, unsigned int period, unsigned int p_time)
{
	char buf[100];
	int len = snprintf(buf, sizeof(buf), "R,%d,%u,%u", pid, period, p_time);
	int fd = open("/proc/monotonic_sched/status", O_WRONLY);

	if (fd < 0)
		return -1;
	int ret = write(fd, buf, len);

	close(fd);
	return ret;
}

int deregister_job(int pid)
{
	char buf[50];
	int len = snprintf(buf, sizeof(buf), "D,%d", pid);
	int fd = open("/proc/monotonic_sched/status", O_WRONLY);

	if (fd < 0)
		return -1;
	int ret = write(fd, buf, len);

	close(fd);
	return ret;
}

/* Kernel will put application to sleep */
int yield_job(int pid)
{
	char buf[50];
	int len = snprintf(buf, sizeof(buf), "Y,%d", pid);
	int fd = open("/proc/monotonic_sched/status", O_WRONLY);

	if (fd < 0)
		return -1;
	int ret = write(fd, buf, len);

	close(fd);
	return ret;
}

bool in_fs(pid_t pid)
{
	FILE *fp;
	char line[256];
	char procfile[30] = "/proc/monotonic_sched/status";

	fp = fopen(procfile, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error opening file %s\n", procfile);
		return false;
	}

	/* Read line by line until End of File (EOF) */
	while (fgets(line, sizeof(line), fp) != NULL) {
		pid_t p_pid;
		unsigned int period, processing_time;
		int got = sscanf(line, "%d: %u, %u", &p_pid, &period, &processing_time);

		if (got == 3) {
			if (pid == p_pid)
				return true;
		} else {
			fprintf(stderr, "sscanf error, expected 3 values, got %d\n", got);
			break;
		}
	}

	return false;
}

void job(unsigned int processing_time) /* ms */
{
	struct timespec start, now;

	clock_gettime(CLOCK_MONOTONIC, &start);

	while (1) {
		// dummy CPU work
		for (int i = 0; i < 1000; i++)
			;

		clock_gettime(CLOCK_MONOTONIC, &now);

		long elapsed = (now.tv_sec - start.tv_sec) * 1000 +
			(now.tv_nsec - start.tv_nsec) / 1000000;

		if (elapsed >= processing_time)
			break;
	}
}

uint64_t in_ns(struct timespec *ts)
{
	return (uint64_t)ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

int main(void)
{
	srand(time(NULL));

	int iterations = (int)rand() % 10001 + 10000;

	pid_t pid = getpid();
	/* 100 to 200 (ms) */
	unsigned int period = (unsigned int)rand() % 101 + 100;
	/* 10 to 20 (ms per job) */
	unsigned int p_time = (unsigned int)rand() % 11 + 10;

	printf("INFO: %d, %u, %u\n", pid, period, p_time);

	register_job(pid, period, p_time);

	if (!in_fs(pid)) {
		fprintf(stderr, "Process %d not found in list.\n", pid);
		return EXIT_FAILURE;
	}

	struct timespec ts;

	/* Kernel puts process to sleep. */
	yield_job(pid);

	while (iterations > 0) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		uint64_t wakeup_ns = in_ns(&ts);

		job(p_time);

		clock_gettime(CLOCK_MONOTONIC, &ts);
		uint64_t processing_ns = in_ns(&ts);
		uint64_t time_taken = processing_ns - wakeup_ns;

		if (iterations % 1000000000)
			printf("%d: time taken(ms) = %lu\n", iterations, time_taken / 1000000);
		iterations--;

		/* Go back to sleep. */
		yield_job(pid);
	}

	deregister_job(pid);
	printf("PID %d: finished job\n", pid);
	return EXIT_SUCCESS;
}
