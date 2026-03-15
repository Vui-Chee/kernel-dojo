// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

int __register(int pid, unsigned int period, unsigned int p_time)
{
	char string[100];
	sprintf(string, "echo -n \'R,%d,%u,%u\' > /proc/monotonic_sched/status", pid, period, p_time);
	printf("%s\n", string);
	return system(string);
}

int __deregister(int pid) {
	char string[50];
	sprintf(string, "echo -n \'D,%d\' > /proc/monotonic_sched/status", pid);
	printf("%s\n", string);
	return system(string);
}

// writes to the proc filesystem, kernel module puts application to sleep
void __yield(void) {}

void __read_fs(void) {}

int __job(int n)
{
	if (n <= 1)
		return 1;
	return n * __job(n - 1);
}

int main(void)
{
	int pid = getpid();
	unsigned int p_time = 3000; // ms
	unsigned int period = 100; // ms

	__register(pid, period, p_time);

	// TODO: read() from procfile to verify process is added

	__job(100);
	printf("sleeping...\n");
	sleep(30);

	__deregister(pid);

	printf("finished\n");
	return 0;
}

/*
 * REGISTER(pid, period, processing_time);
 * // Read ProcFS: Verify the process was admitted
 * list = READ(ProcFS);
 * if (!process in the list) exit(1);
 * // setup everything needed for RT loop
 * t0 = clock_gettime();
 * // Proc filesystem
 * YIELD(PID);
 * // this is the real-time loop
 * while (exist jobs)
 * {
 *	wakeup_time = clock_gettime() - t0;
 *	// factorial computation
 *	do_job();
 *	process_time = clock_gettime() - wakeup_time;
 *	YIELD(PID);
 * }
 * // Interact with ProcFS
 * DEREGISTER(PID);
 */
