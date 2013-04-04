#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "kresolv.h"

unsigned int flexdrop_min_mb = 0; /* minimum amount of MB that should always be free */
module_param_named(setuid, frexdrop_min_mb, uint, 0644);

/* This module does the equivalent of a "echo 3 > /proc/sys/vm/drop_caches" until enough
 *  memory is available (in the output of "free -m").
 * It was needed because of ZFS which was not not able to fully meets its configured memory cap
 */

static struct task_struct *gl_flexdrop_task;
static DECLARE_WAIT_QUEUE_HEAD(flexdrop_waitqueue);
static atomic_t flexdrop_wakeup;

static void flexdrop_check_memory(void)
{
  struct sysinfo meminfo;

  si_meminfo(&meminfo);
  printk(KERN_INFO "flexdrop freeram %lu\n", meminfo.freeram * meminfo.mem_unit);

  /* from drop_slab() in linux/fs/drop_cache.c */
  struct shrink_control shrink = { .gfp_mask = GFP_KERNEL, };
  int nr_objects;
  do {
    nr_objects = f_shrink_slab(&shrink, 1000, 1000);
    printk(KERN_INFO " dropped %d nr_objects\n", nr_objects);
    break;
  } while (nr_objects > 0);
}

static int flexdrop_thread(void *data)
{
  unsigned long jif; /* jiffies to wait */

  printk(KERN_INFO "flexdrop thread started\n");
  jif = msecs_to_jiffies(1000);
  while (1)
    {
      /* msleep(1000); */
      wait_event_timeout(flexdrop_waitqueue, atomic_read(&flexdrop_wakeup), jif);
      printk(KERN_INFO "flexdrop thread loop!\n");
      if (atomic_read(&flexdrop_wakeup))
	{
	  atomic_dec(&flexdrop_wakeup);
	  break;
	}
      flexdrop_check_memory();
    }
  printk(KERN_INFO "flexdrop thread stopping...\n");
  /* do_exit(); */
  return 0;
}

static int __init drop_start(void)
{
  struct task_struct *task;

  printk(KERN_INFO "#########################################################################################\n");
  printk(KERN_INFO "###############################           COUCOU          ###############################\n");
  printk(KERN_INFO "#########################################################################################\n");
  printk(KERN_INFO "Loading flexdrop module... All your memory are belong to us.\n\n");
  printk(KERN_INFO "Hello world. I will happily free your memory when I have the feeling to do so.\n\n");
  kresolv_init();

  task = kthread_create(flexdrop_thread, NULL, "flexdrop");
  wake_up_process(task);
  gl_flexdrop_task = task;
  return 0;
}


static void __exit drop_end(void)
{
  if (gl_flexdrop_task)
  {
    printk(KERN_INFO "flexdrop stop()...\n");
    atomic_inc(&flexdrop_wakeup);
    wake_up(&flexdrop_waitqueue);
    kthread_stop(gl_flexdrop_task);
    gl_flexdrop_task = NULL;
    printk(KERN_INFO "flexdrop stopped!\n");
  }
  printk(KERN_INFO "Goodbye Mr. FlexDrop\n\n");
}

module_init(drop_start);
module_exit(drop_end);

#define VERSION "0.1"

MODULE_AUTHOR("Eric Gouyer <folays@folays.net>");
MODULE_DESCRIPTION("Flexible Memory Drop Cache ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
/* MODULE_LICENSE("Proprietary"); */
