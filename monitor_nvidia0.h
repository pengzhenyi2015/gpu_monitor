#include<linux/list.h>
#include<linux/time.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/ctype.h>
#include<linux/rtc.h>
#define DEBUG		//when debugging
#define TIME_OFFSET ((31*24+8)*3600)
struct operation_details{
	unsigned int 		times;
	struct timespec		latest_time;
};

struct task_details{
	struct operation_details open;
	struct operation_details release;
	struct operation_details read;
	struct operation_details write;
	int state;		//1,means process is using module
};

struct task_statistics{
	pid_t 			pid;
	char 			pname[20];
	struct timespec 	stime;
	unsigned int		ntimes;
	struct task_details 	*details;
	spinlock_t 		lock;
	struct list_head 	list;
};

static int atoi10(const char *s) 
{
	int i = 0;
	while(' ' == *s) s++;
	while (isdigit(*s))
	{
		i = i*10 + *(s++) - '0';
	}
	return i;
}

/*
	@Print the informations of the given process
	@in:pid
	@out:0 for succeed, others for failed
*/
int print_process_details(pid_t pid,struct list_head *head)
{
	struct task_statistics *taskp;
        struct rtc_time tm;
	
	list_for_each_entry(taskp,head,list)
	{
		if(taskp->pid==pid)
		{
			printk(KERN_EMERG "\n[processid]:%d\t[process name]:%s\n",taskp->pid,taskp->pname);
			if(taskp->details->state==1)
				printk(KERN_EMERG "State:USING.\n");
			else
				printk(KERN_EMERG "State: NOT USING.\n");
			printk(KERN_EMERG "[operations]\t[number of times]\t[latest time]\n");
			if(taskp->details->open.latest_time.tv_sec!=0)
			{
			rtc_time_to_tm(taskp->details->open.latest_time.tv_sec+TIME_OFFSET,&tm);
			printk(KERN_EMERG "Open        \t%-17d\t%04d-%02d-%02d %02d:%02d:%02d\n",taskp->details->open.times,tm.tm_year+1900,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
			}
			if(taskp->details->release.latest_time.tv_sec!=0)
			{
			rtc_time_to_tm(taskp->details->release.latest_time.tv_sec+TIME_OFFSET,&tm);
			printk(KERN_EMERG "Release     \t%-17d\t%04d-%02d-%02d %02d:%02d:%02d\n",taskp->details->release.times,tm.tm_year+1900,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
			}
			if(taskp->details->read.latest_time.tv_sec!=0)
			{
			rtc_time_to_tm(taskp->details->read.latest_time.tv_sec+TIME_OFFSET,&tm);
			printk(KERN_EMERG "Read        \t%-17d\t%04d-%02d-%02d %02d:%02d:%02d\n",taskp->details->read.times,tm.tm_year+1900,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
			}
			if(taskp->details->write.latest_time.tv_sec!=0)
                        {
			rtc_time_to_tm(taskp->details->write.latest_time.tv_sec+TIME_OFFSET,&tm);
			printk(KERN_EMERG "Write       \t%-17d\t%04d-%02d-%02d %02d:%02d:%02d\n",taskp->details->write.times,tm.tm_year+1900,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
			}
			return 0;
		}
	}
	printk(KERN_EMERG "Error:No such process %d\n",pid);
	return -EINVAL;
}

/*
        @Alloc a new task_statistics and add to the list_head
        @in:list head
        @out:the newly allocated task_statistics pointer
*/
struct task_statistics *alloc_task_statistics(struct list_head *head)
{
	struct task_struct *p=current;
	struct task_statistics *newtaskp=(struct task_statistics *)kmalloc(sizeof(struct task_statistics),GFP_KERNEL);
	
	/*alloc a new task_statistics*/
	if(newtaskp==NULL)
	{
		printk(KERN_EMERG "Error:unable to alloc task_statistics.\n");
		return NULL;
	}
	newtaskp->pid=p->pid;
#ifdef DEBUG
	printk(KERN_INFO "%s:pid=%d.\n",__FUNCTION__,newtaskp->pid);
#endif
	strcpy(newtaskp->pname,current->comm);
	newtaskp->stime.tv_sec=p->start_time.tv_sec;
	newtaskp->stime.tv_nsec=p->start_time.tv_nsec;
	newtaskp->ntimes=0;
	spin_lock_init(&newtaskp->lock);
	newtaskp->details=(struct task_details *)kmalloc(sizeof(struct task_details),GFP_KERNEL);
	if(newtaskp->details==NULL)
	{
#ifdef DEBUG
		printk(KERN_INFO "Error:unable to alloc task_details.\n");
#endif
		return NULL;
	}
	memset(newtaskp->details,0,sizeof(struct task_details));
	newtaskp->details->state=1;			
#ifdef DEBUG
	printk(KERN_INFO "%s:Add a task_statistics to list_head.pid=%d.\n",__FUNCTION__,newtaskp->pid);
#endif
	list_add(&newtaskp->list,head);

	return newtaskp;

	/*add the task_statistics to list_head*/
}
/*
        @Print all the process informations
        @in:head
        @out:0 for succeed, others for failed
*/

int print_process_all(struct list_head *head)
{
	struct task_statistics *taskp;
	//struct tm tm;

	printk(KERN_EMERG "\n[process id]\t[process name]\t[start time] \t[times]\n");
	list_for_each_entry(taskp,head,list)
	{
		//time_to_tm(taskp->stime.tv_sec,0,&tm);
		printk(KERN_EMERG "%-12d\t%-14s\t%ld.%09ld\t%-7d\n",
			taskp->pid,taskp->pname,taskp->stime.tv_sec,taskp->stime.tv_nsec,taskp->ntimes);
	}
	return 0;
}

/*
        @Destroy the task_statistics list when the module exit
        @in:list head
        @out:0 for succeed,others for failed
*/
int destroy_task_list(struct list_head *head)
{
	struct list_head *pos,*n;
	struct task_statistics *taskp;

	list_for_each_safe(pos,n,head)
	{
		taskp=list_entry(pos,struct task_statistics,list);
		list_del(pos);
		kfree(taskp->details);
		kfree(taskp);
	}
	return 0;
}


/*
        @Get the pid,and return the corresponding task_statistics.if no task_statistics exsit,alloc one
        @in:pid,list head
        @out:the corresponding task_statistics
*/
struct task_statistics *get_task_statistics(pid_t pid,struct list_head *head)
{
	struct task_statistics *taskp;

	list_for_each_entry(taskp,head,list)
	{
		if(taskp->pid==pid)
			return taskp;
	}

	taskp=alloc_task_statistics(head);
	return taskp;
}
