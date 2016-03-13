#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include<linux/slab.h>
#include<linux/sched.h>


static int processID = -1;
static int processSPolicy = -1;
static int processPrio = -1;

module_param( processID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processID, "processID");
module_param( processSPolicy, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processSPolicy, "processSPolicy");
module_param( processPrio, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC( processPrio, "processPrio");
void get_pol_name(int pol);

int myfileinfo_init(void)
{
	struct task_struct *task, tt;
	struct sched_param param;
//struct list children;
	struct list_head *p;
	int id_found = -1;
	INIT_LIST_HEAD(&tt.children);
	printk(KERN_INFO "SchedInfo is Loading...\n");

	if(processID == -1) {
		printk("Process id is not provided.\n");
		return 0;
	}

	for_each_process(task){
		if (task->pid == processID) {
			id_found = 1;
			break;
		}
	}
	if (id_found == -1) {
		//printk("here\n");
		printk("Process not found with id %d\n", processID);
		return 0;
	}
	printk("Executable Name: %s\n", task->comm);
	printk("Process ID     : %d\n", task->pid);
	printk("Priority       : %d\n", task->prio);
	printk("Static Priority: %d\n", task->static_prio);
	printk("Parent PID     : %d\n", task->parent->pid);
	printk("Time slice     : %d\n", task->rt.time_slice);
	printk("Policy         : %d", task->policy);
	get_pol_name(task->policy);
	printk("User ID        : %d\n", task->cred->uid.val);



	list_for_each(p, &(task->parent->children)){
		tt = *list_entry(p, struct task_struct, sibling);
		if(tt.pid != task->pid){
			printk("Sibling Executable Name: %s\n", tt.comm);
			printk("Sibling Process ID     : %d\n", tt.pid);
		}
	}

	if(processSPolicy == -1 && processPrio == -1)
		return 0;

		if(processSPolicy == -1){
			processSPolicy = task->policy;
		}

		if (processPrio != -1){
			param.sched_priority = processPrio;
		}else{
			param.sched_priority = task->prio;
		}

   
		printk("\nChanging process scheduler info: %d\n", sched_setscheduler(task, processSPolicy, & param));



	printk("Executable Name: %s\n", task->comm);
	printk("Process ID     : %d\n", task->pid);
	printk("Priority       : %d\n", task->prio);
	printk("Static Priority: %d\n", task->static_prio);
	printk("Parent PID     : %d\n", task->parent->pid);
	printk("Time slice     : %d\n", task->rt.time_slice);
	printk("Policy         : %d", task->policy);
	get_pol_name(task->policy);
	printk("User ID        : %d\n", task->cred->uid.val);



	list_for_each(p, &(task->parent->children)){
		tt = *list_entry(p, struct task_struct, sibling);
		if(tt.pid != task->pid){
			printk("Sibling Executable Name: %s\n", tt.comm);
			printk("Sibling Process ID     : %d\n", tt.pid);
		}
	}

	/*printk("%d\n", SCHED_NORMAL); //0
	printk("%d\n", SCHED_BATCH); //3
	printk("%d\n", SCHED_IDLE);//5
	printk("%d\n", SCHED_FIFO); //1
	printk("%d\n", SCHED_RR);//2*/

	return 0;
}

void get_pol_name(int pol){
	if (pol==0) {
		printk(" (SCHED_NORMAL)\n");
	} else if (pol==1) {
		printk(" (SCHED_FIFO)\n");
	} else if (pol==2) {
		printk(" (SCHED_RR)\n");
	} else if (pol==3) {
		printk(" (SCHED_BATCH)\n");
	} else if (pol==5) {
		printk(" (SCHED_IDLE)\n");
	} else {
		printk(" (UNKWN_POL)\n");
	}
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
