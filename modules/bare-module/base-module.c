#include <linux/module.h>
#include <linux/init.h>

static int __init my_init(void)
{
	pr_info("Hello world from mod1\n");

	return 0;
}

static void __exit my_exit(void)
{
	pr_info("Goodbye world from mod1\n");
}

void mod1fun(void)
{
	pr_info(" VOILA! I got into %s\n", __func__);
}
EXPORT_SYMBOL(mod1fun);

int foo = 123;
EXPORT_SYMBOL(foo);

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Vui Chee");
MODULE_DESCRIPTION("base module");
MODULE_LICENSE("GPL v2");
