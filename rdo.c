/*
 * Copyright (c) 2012 Austin Seipp
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

#include "config.h"

/* Initialization/teardown, and module descriptions/frontmatter */

static int __init
init_randomdotorg(void)
{
  printk("hello!\n");
  return 0;
}

static void __exit
exit_randomdotorg(void)
{
  printk("goodbye!\n");
}

MODULE_AUTHOR("Austin Seipp <mad.one@gmail.com>");
/* Actually MIT, but I don't want the kernel to bitch... */
MODULE_LICENSE("Dual MIT/GPL"); 
MODULE_DESCRIPTION("Provides a /dev/randomdotorg device "
		   "for atmospheric random data");
MODULE_VERSION(DRIVER_VERSION);
module_init(init_randomdotorg);
module_exit(exit_randomdotorg);
