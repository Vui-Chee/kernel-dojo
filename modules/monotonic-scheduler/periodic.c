// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

int register_job(int pid, unsigned int period, unsigned int p_time)
{
	char string[100];

	sprintf(string, "echo -n \'R,%d,%u,%u\' > /proc/monotonic_sched/status", pid, period, p_time);
	printf("%s\n", string);
	return system(string);
}

int deregister_job(int pid)
{
	char string[50];

	sprintf(string, "echo -n \'D,%d\' > /proc/monotonic_sched/status", pid);
	printf("%s\n", string);
	return system(string);
}

// writes to the proc filesystem, kernel module puts application to sleep
void yield_job(int pid)
{
	char string[50];

	sprintf(string, "echo -n \'Y,%d\' > /proc/monotonic_sched/status", pid);
	printf("%s\n", string);
	return system(string);
}

void read_fs(void) {}

int job(int n)
{
	if (n <= 1)
		return 1;
	return n * __job(n - 1);
}

int main(void)
{
	int pid = getpid();
	// TODO: randomize time
	unsigned int p_time = 3000; // ms
	unsigned int period = 500; // ms

	register_job(pid, period, p_time);

	yield_job(pid);

	job(1000);
	// sleep(3);
	printf("finished work...\n");

	deregister_job(pid);

	printf("finished\n");
	return 0;
}
