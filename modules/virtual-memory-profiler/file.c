// SPDX-License-Identifier: GPL-2.0
#include "file.h"

const struct proc_ops fileops = {
	.proc_write = on_proc_write,
};

ssize_t on_proc_write(struct file *file, const char __user *ubuf, size_t count,
		      loff_t *ppos)
{
	if (*ppos != 0)
		return -EINVAL;

	if (count < 1 || count > PAGE_SIZE)
		return -EINVAL;

	/* Copy data from user space to kernel space safely (including null-termination). */
	char *kbuf __free(kfree) = memdup_user_nul(ubuf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	pid_t pid;
	char op = kbuf[0];

	switch (op) {
	case 'R': {
		int ret = sscanf(kbuf, "%c,%d", &op, &pid);

		if (ret != 2) {
			pr_err("Register operation accepts only `R <pid>`. Got %d values.\n",
			       ret);
			break;
		}
		pr_debug("Registering process: %c,%d\n", op, pid);
		break;
	}

	case 'U': {
		int ret = sscanf(kbuf, "%c,%d", &op, &pid);

		if (ret != 2) {
			pr_err("De-register operation accepts only `U <pid`. Got %d values.\n",
			       ret);
			break;
		}
		pr_debug("De-registering process: %c,%d\n", op, pid);
		break;
	}

	default:
		pr_err("Unknown operation: %c\n", op);
	}

	return count;
}
