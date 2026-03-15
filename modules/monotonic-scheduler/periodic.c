// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

void __register(int pid, int period, int p_time) {}

void __deregister(int pid) {}

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
	int p_time = 3000; // ms
	int period = 100; // ms
	int pid = getpid();

	__register(pid, period, p_time);

	// TODO: read() from procfile to verify process is added

	__job(100);
	printf("sleeping...\n");
	sleep(10);

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
