// SPDX-License-Identifier: GPL-2.0
#include "stdio.h"
#include <unistd.h> // Required for getpid()
#include <stdlib.h> // Required for system()
#include <time.h> // Required for time()

int main(void)
{
	int pid = getpid();
	char string[100];

	printf("pid is %d\n", pid);
	sprintf(string, "echo -n \'%d\' > /proc/proc_time/status", pid);
	printf("%s\n", string);
	system(string);

	// Get the starting time
	time_t start = time(NULL);

	// Loop until the current time is X seconds past the start
	// This keeps the CPU at 100% usage for this core
	while (time(NULL) < start + 10)
		;
	return 0;
}
