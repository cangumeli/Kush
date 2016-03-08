#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include<linux/slab.h>
#include<linux/sched.h>


struct myfileinfo {

};

/**
 * The following defines and initializes a list_head object named files_list
 */

static LIST_HEAD(files_list);
static int processID = -1;
static int processSPolicy = -1;
static int processPrio = -1;

module_param( processID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processID, "processID");
module_param( processSPolicy, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processSPolicy, "processSPolicy");
module_param( processPrio, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processPrio, "processPrio");

int myfileinfo_init(void)
{
	struct task_struct *task, tt;
//struct list children;
	struct list_head *p;
	INIT_LIST_HEAD(&tt.children);
	printk(KERN_INFO "SchedInfo is Loading...\n");

	if(processID == -1)
		return 0;

	for_each_process(task){
		if (task->pid == processID) {
			break;
		}
	}
	printk("Executable Name: %s\n", task->comm);
	printk("Process ID     : %d\n", task->pid);
	printk("Priority       : %d\n", task->prio);
	printk("Static Priority: %d\n", task->static_prio);
	printk("Parent PID     : %d\n", task->parent->pid);
	printk("Time slice     : %d\n", task->rt.time_slice);
	printk("Policy         : %d\n", task->policy);
	printk("User ID        : %d\n", task->cred->uid);



	list_for_each(p, &(task->parent->children)){
		tt = *list_entry(p, struct task_struct, sibling);
		if(tt.pid != task->pid){
			printk("Sibling Executable Name: %s\n", tt.comm);
			printk("Sibling Process ID     : %d\n", tt.pid);
		}
	}

	if(processSPolicy == -1 || processPrio == -1)
		return 0;

		

	return 0;
}

void myfileinfo_exit(void)
 {
	printk(KERN_INFO "Removing Module\n");

	/*list_for_each_entry_safe( ptr, next, &files_list, list) {
		list_del(&ptr->list);
		kfree(ptr);
	}*/
}

module_init( myfileinfo_init);
module_exit( myfileinfo_exit);

MODULE_LICENSE( "GPL");
MODULE_DESCRIPTION( "COMP304 Assignment1 Problem4");
MODULE_AUTHOR("Team GG");
