// SPDX-License-Identifier: GPL-2.0
#include "file.h"
#include "process.h"

const struct proc_ops fileops = {
	.proc_write = on_proc_write,
};

ssize_t on_proc_write(struct file *file, const char __user *ubuf, size_t count,
		      loff_t *ppos)
{
	char *kbuf __free(kfree);

	pid_t pid;
	char op;
	int ret, err;

	if (*ppos != 0)
		return -EINVAL;

	if (count < 1 || count > PAGE_SIZE)
		return -EINVAL;

	/* Copy data from user space to kernel space safely (including null-termination). */
	kbuf = memdup_user_nul(ubuf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	op = kbuf[0];

	switch (op) {
	case 'R': {
		ret = kstrtoint(kbuf + 2, 10, &pid);

		if (ret < 0) {
			pr_err("Register operation accepts only `R <pid>`. Got %d values.\n",
			       ret);
			break;
		}

		err = reg_proc(pid);

		if (err != 0)
			pr_err("Failed to register process %d, got err: %d\n",
			       pid, err);
		else
			pr_debug("Registered process: %d\n", pid);
		break;
	}

	case 'U': {
		ret = kstrtoint(kbuf + 2, 10, &pid);

		if (ret < 0) {
			pr_err("De-register operation accepts only `U <pid>`. Got %d values.\n",
			       ret);
			break;
		}

		err = unreg_proc(pid);

		if (err != 0)
			pr_err("Failed to un-register process %d, got err: %d\n",
			       pid, err);
		else
			pr_debug("Unregistered process: %d\n", pid);
		break;
	}

	default:
		pr_err("Unknown operation: %c\n", op);
	}

	return count;
}
