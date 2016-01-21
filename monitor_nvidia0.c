#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/namei.h>
#include<linux/cdev.h>
#include<linux/sched.h>
#include<asm/uaccess.h>
#include<linux/time.h>
#include<linux/jiffies.h>
#include"monitor_nvidia0.h"

static int cdev_major=0;
static int cdev_minor=0;
dev_t cdev_no;
static struct cdev *chandev;
static int find_dev=0;			//0,for find chrdev;1,for didnot find
static struct file_operations chrdev_fops_new;
static const struct file_operations *chrdev_fops_old;
static struct file_operations chandev_fops_own; //fops used by this module
struct cdev *cdevp=NULL;
LIST_HEAD(TASK_STATISTICS_HEAD);

static int chrdev_open_new(struct inode *inode,struct file *filp)
{
	struct task_struct *task = current;
	struct task_statistics *taskp;
#ifdef DEBUG
	printk(KERN_INFO "Open chrdev new...\n");
	printk(KERN_INFO "task->pid=%d,task->tgid=%d.\n",task->pid,task->tgid);
#endif
	taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
#ifdef DEBUG
	printk(KERN_INFO "Back to function:%s.",__FUNCTION__);
#endif
	if(taskp!=NULL)
	{
		spin_lock(&taskp->lock);
		taskp->ntimes++;
		taskp->details->open.times++;
		taskp->details->open.latest_time=CURRENT_TIME;
		taskp->details->state=1;
		spin_unlock(&taskp->lock);
	}
#ifdef DEBUG
	printk(KERN_INFO "Execute chrdev_fops_old->open.\n");
#endif
	if(chrdev_fops_old->open!=NULL)
		return chrdev_fops_old->open(inode,filp);
	else
		printk(KERN_INFO "%s:no open function in old fops.\n",__FUNCTION__);
	return 0;
}

static int chrdev_release_new(struct inode *inode,struct file *filp)
{
	struct task_struct *task = current;
	struct task_statistics *taskp;
#ifdef DEBUG
        printk(KERN_INFO "Release chrdev new...\n");
#endif
	
	taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
	if(taskp!=NULL)
	{
		spin_lock(&taskp->lock);
		taskp->ntimes++;
		taskp->details->release.times++;
		//taskp->details->release.latest_time=ns_to_timespec(jiffies);
		//jiffies_to_timespec(jiffies,&(taskp->details->release.latest_time));
		taskp->details->release.latest_time=CURRENT_TIME;
		taskp->details->state=0;
		spin_unlock(&taskp->lock);
	}
	if(chrdev_fops_old->release!=NULL)
		return chrdev_fops_old->release(inode,filp);
	else
		printk(KERN_INFO "%s:no release function in old fops.\n",__FUNCTION__);
        return 0;
}

static ssize_t chrdev_read_new(struct file *filp,char *buf,size_t count,loff_t* offp)
{
        struct task_struct *task = current;
        struct task_statistics *taskp;
#ifdef DEBUG
        printk(KERN_INFO "Read chrdev new...\n");
#endif

        taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
        if(taskp!=NULL)
        {
                spin_lock(&taskp->lock);
                taskp->ntimes++;
                taskp->details->read.times++;
                //taskp->details->read.latest_time=ns_to_timespec(jiffies);
		//jiffies_to_timespec(jiffies,&(taskp->details->read.latest_time));
		taskp->details->read.latest_time=CURRENT_TIME;
                spin_unlock(&taskp->lock);
        }
	if(chrdev_fops_old->read!=NULL)
        	return chrdev_fops_old->read(filp,buf,count,offp);
	else
		printk(KERN_INFO "%s:no read function in old fops.\n",__FUNCTION__);
        return 1;
}

static ssize_t chrdev_write_new(struct file *filp,const char *buf,size_t count,loff_t* offp)
{
        struct task_struct *task = current;
        struct task_statistics *taskp;
#ifdef DEBUG
        printk(KERN_INFO "Write chrdev new...\n");
#endif

        taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
        if(taskp!=NULL)
        {
                spin_lock(&taskp->lock);
                taskp->ntimes++;
                taskp->details->write.times++;
               // taskp->details->write.latest_time=ns_to_timespec(jiffies);
		//jiffies_to_timespec(jiffies,&(taskp->details->write.latest_time));
		taskp->details->write.latest_time=CURRENT_TIME;
                spin_unlock(&taskp->lock);
        }
	if(chrdev_fops_old->write!=NULL)
        	return chrdev_fops_old->write(filp,buf,count,offp);
	else
		printk(KERN_INFO "%s:no write function in old fops.\n",__FUNCTION__);
        return count;
}

static loff_t chrdev_llseek_new(struct file *filp,loff_t offp,int count)
{
	if(chrdev_fops_old->llseek!=NULL)
		return chrdev_fops_old->llseek(filp,offp,count);
	else
                printk(KERN_EMERG "%s:no llseek function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static ssize_t chrdev_aio_read_new(struct kiocb *kiocba,const struct iovec *iovecb,unsigned long count,loff_t offp)
{
	struct task_struct *task = current;
	struct task_statistics *taskp;
#ifdef DEBUG
	printk(KERN_INFO "Aio_read chrdev new...\n");
#endif
 	taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
	if(taskp!=NULL)
	{
 		spin_lock(&taskp->lock);
 		taskp->ntimes++;
 		taskp->details->read.times++;
		taskp->details->read.latest_time=CURRENT_TIME;
		spin_unlock(&taskp->lock);
	}
	if(chrdev_fops_old->aio_read!=NULL)
		return chrdev_fops_old->aio_read(kiocba,iovecb,count,offp);
	else
                printk(KERN_EMERG "%s:no aio_read function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static ssize_t chrdev_aio_write_new(struct kiocb *kiocba,const struct iovec *iovecb,unsigned long count,loff_t offp)
{
	struct task_struct *task = current;
	struct task_statistics *taskp;
#ifdef DEBUG
	printk(KERN_INFO "Aio_write chrdev new...\n");
#endif

	taskp=get_task_statistics(task->pid,&TASK_STATISTICS_HEAD);
	if(taskp!=NULL)
	{
		spin_lock(&taskp->lock);
		taskp->ntimes++;
		taskp->details->write.times++;
		taskp->details->write.latest_time=CURRENT_TIME;
		spin_unlock(&taskp->lock);
	}
	if(chrdev_fops_old->aio_write!=NULL)
		return chrdev_fops_old->aio_write(kiocba,iovecb,count,offp);
	else
		printk(KERN_EMERG "%s:no aio_write function in old fops.\n",__FUNCTION__);
     
   return EINVAL;
}

static int chrdev_readdir_new(struct file *filp,void *source,filldir_t dir)
{
	if(chrdev_fops_old->readdir!=NULL)
		return chrdev_fops_old->readdir(filp,source,dir);
	else
                printk(KERN_EMERG "%s:no readdir function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static unsigned int chrdev_poll_new(struct file *filp,struct poll_table_struct *table)
{
	if(chrdev_fops_old->poll!=NULL)
		return chrdev_fops_old->poll(filp,table);
	else
		printk(KERN_EMERG "%s:no poll function in old fops.\n",__FUNCTION__);
	return EINVAL;
}

static int chrdev_ioctl_new(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long args)
{
	if(chrdev_fops_old->ioctl!=NULL)
		return chrdev_fops_old->ioctl(inode,filp,cmd,args);
	else
		printk(KERN_EMERG "%s:no ioctl function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static long chrdev_unlocked_ioctl_new(struct file *filp,unsigned int cmd,unsigned long args)
{
	if(chrdev_fops_old->unlocked_ioctl!=NULL)
		return chrdev_fops_old->unlocked_ioctl(filp,cmd,args);
	else
                printk(KERN_EMERG "%s:no unlocked_ioctl function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static long chrdev_compat_ioctl_new(struct file *filp,unsigned int cmd,unsigned long args)
{
	if(chrdev_fops_old->compat_ioctl!=NULL)
		return chrdev_fops_old->compat_ioctl(filp,cmd,args);
	else
                printk(KERN_EMERG "%s:no compat_ioctl function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_mmap_new(struct file *filp,struct vm_area_struct *vm_area)
{
	if(chrdev_fops_old->mmap!=NULL)
		return chrdev_fops_old->mmap(filp,vm_area);
	else
                printk(KERN_EMERG "%s:no mmap function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_flush_new(struct file *filp,fl_owner_t id)
{
	if(chrdev_fops_old->flush!=NULL)
		return chrdev_fops_old->flush(filp,id);
	else
                printk(KERN_EMERG "%s:no flush function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_fsync_new(struct file *filp,struct dentry *dent,int datasync)
{
	if(chrdev_fops_old->fsync!=NULL)
		return chrdev_fops_old->fsync(filp,dent,datasync);
	else
                printk(KERN_EMERG "%s:no fsync function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_aio_fsync_new(struct kiocb *kiocba,int datasync)
{
	if(chrdev_fops_old->aio_fsync!=NULL)
		return chrdev_fops_old->aio_fsync(kiocba,datasync);
	else
                printk(KERN_EMERG "%s:no aio_fsync function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_fasync_new(int inta,struct file *filp,int intb)
{
	if(chrdev_fops_old->fasync!=NULL)
		return chrdev_fops_old->fasync(inta,filp,intb);
	else
                printk(KERN_EMERG "%s:no fasync function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_lock_new(struct file *filp,int inta,struct file_lock *flockp)
{
	if(chrdev_fops_old->lock!=NULL)
		return chrdev_fops_old->lock(filp,inta,flockp);
	else
                printk(KERN_EMERG "%s:no lock function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static ssize_t chrdev_sendpage_new(struct file *filp,struct page *pagep,int inta,size_t size,loff_t *offp,int intb)
{
	if(chrdev_fops_old->sendpage!=NULL)
		return chrdev_fops_old->sendpage(filp,pagep,inta,size,offp,intb);
	else
                printk(KERN_EMERG "%s:no sendpage function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static unsigned long chrdev_get_unmapped_area_new(struct file *filp,unsigned long longa,unsigned long longb,unsigned long longc,unsigned long longd)
{
	if(chrdev_fops_old->get_unmapped_area!=NULL)
		return chrdev_fops_old->get_unmapped_area(filp,longa,longb,longc,longd);
	else
                printk(KERN_EMERG "%s:no get_unmapped_area function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_check_flags_new(int inta)
{
	if(chrdev_fops_old->check_flags!=NULL)
		return chrdev_fops_old->check_flags(inta);
	else
                printk(KERN_EMERG "%s:no check_flags function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_flock_new(struct file *filp,int inta,struct file_lock *flock)
{
	if(chrdev_fops_old->flock!=NULL)
		return chrdev_fops_old->flock(filp,inta,flock);
	else
                printk(KERN_EMERG "%s:no flock function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static ssize_t chrdev_splice_write_new(struct pipe_inode_info *inode_info,struct file *filp,loff_t *offp,size_t size,unsigned int inta)
{
	if(chrdev_fops_old->splice_write!=NULL)
		return chrdev_fops_old->splice_write(inode_info,filp,offp,size,inta);
	else
                printk(KERN_EMERG "%s:no splice_write function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static ssize_t chrdev_splice_read_new(struct file *filp,loff_t *offp,struct pipe_inode_info *inode_info,size_t size,unsigned int inta)
{
	if(chrdev_fops_old->splice_read!=NULL)
		return chrdev_fops_old->splice_read(filp,offp,inode_info,size,inta);
	else
                printk(KERN_EMERG "%s:no splice_read function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static int chrdev_setlease_new(struct file *filp,long longa,struct file_lock **flock)
{
	if(chrdev_fops_old->setlease!=NULL)
		return chrdev_fops_old->setlease(filp,longa,flock);
	else
                printk(KERN_EMERG "%s:no setlease function in old fops.\n",__FUNCTION__);
        return EINVAL;
}

static struct file_operations chrdev_fops_new=
{
	.owner		=THIS_MODULE,
	.open		=chrdev_open_new,
	.release	=chrdev_release_new,
	.read		=chrdev_read_new,
	.write		=chrdev_write_new,
	.llseek		=chrdev_llseek_new,
	.aio_read	=chrdev_aio_read_new,
	.aio_write	=chrdev_aio_write_new,
	.readdir	=chrdev_readdir_new,
	.poll		=chrdev_poll_new,
	.ioctl		=chrdev_ioctl_new,	
	.unlocked_ioctl	=chrdev_unlocked_ioctl_new,
	.compat_ioctl	=chrdev_compat_ioctl_new,
	.mmap		=chrdev_mmap_new,
	//.flush		=chrdev_flush_new,
	.fsync		=chrdev_fsync_new,
	.aio_fsync	=chrdev_aio_fsync_new,
	.fasync		=chrdev_fasync_new,
	.lock		=chrdev_lock_new,
	.sendpage	=chrdev_sendpage_new,
	//.get_unmapped_area=chrdev_get_unmapped_area_new,
	.check_flags	=chrdev_check_flags_new,
	.flock		=chrdev_flock_new,
	.splice_write	=chrdev_splice_write_new,
	.splice_read	=chrdev_splice_read_new,
	.setlease	=chrdev_setlease_new,
};

static int chandev_open_own(struct inode *inode,struct file *filp)
{
#ifdef DEBUG
	printk(KERN_INFO "Open chandev.\n");
#endif
	return 0;
}

static int chandev_release_own(struct inode *inode,struct file *filp)
{
#ifdef DEBUG
	printk(KERN_INFO "Release chandev.\n");
#endif
	return 0;
}

static int test_list(void)
{
	int i,result;
	struct task_statistics *taskp;
	for(i=0;i<5;i++)
	{
		taskp=alloc_task_statistics(&TASK_STATISTICS_HEAD);
	}
	result=print_process_all(&TASK_STATISTICS_HEAD);
	return result;
}

static int test_del_list(void)
{
	int result;
	result=destroy_task_list(&TASK_STATISTICS_HEAD);
#ifdef DEBUG
	printk(KERN_INFO "clear the list.\n");
#endif
	result=print_process_all(&TASK_STATISTICS_HEAD);
	return result;
}

static ssize_t chandev_write_own(struct file *filp,const char __user *buf,size_t count,loff_t *offp)
{
	char data[32],*p;
	int ret=0;
	pid_t pid;

	if(!buf || count<=0)
	{
		printk(KERN_EMERG "Error:Invalid parameter\n");
		return -EINVAL;
	}
	memset(data,0,sizeof(data));
	if(count>sizeof(data))
		count=sizeof(data)-1;
	if(unlikely(ret=copy_from_user(data,buf,count)))
	{
#ifdef DEBUG
		printk(KERN_INFO "copy from user error.\n");
#endif
		return -EFAULT;
	}

	if((p=strstr(data,"test"))!=NULL)
	{
#ifdef DEBUG
		printk(KERN_INFO "get command \"test\".");
#endif
		test_list();
	}
	else if((p=strstr(data,"show"))!=NULL)
	{
		print_process_all(&TASK_STATISTICS_HEAD);
	}
	else if((p=strstr(data,"clear"))!=NULL)
	{
#ifdef DEBUG
		printk(KERN_INFO "get command \"clear\".");
#endif
		test_del_list();
	}
	else if((p=strstr(data,"pid="))!=NULL)
	{
		pid=atoi10(data+4);
		printk(KERN_EMERG "Details of pid=%d\n",pid);
		print_process_details(pid,&TASK_STATISTICS_HEAD);
	}
	else
		printk(KERN_EMERG "Error:command unkown.\n");

	return count;
}

static struct file_operations chandev_fops_own={
	.owner=THIS_MODULE,
	.open=chandev_open_own,
	.release=chandev_release_own,
	.write=chandev_write_own,
};

static int change_cdev(char *file_path)
{
        int error=0;
	struct nameidata nd;
	struct inode *inode;
	
        error=path_lookup(file_path,0,&nd);
	if(error!=0)
	{
#ifdef DEBUG
		printk(KERN_INFO "Unable to find %s.\n",file_path);
#endif
		return error;
	}
	inode=nd.path.dentry->d_inode;
	if(inode==NULL)	return -EINVAL;
	cdevp=inode->i_cdev;	
	printk(KERN_INFO "%s:inode_no:%ld.\n",__FUNCTION__,inode->i_ino);
	printk(KERN_INFO "nameidata->depth=%d,dentry name:%s",nd.depth,nd.path.dentry->d_iname);
	if(inode->i_cdev==NULL)
	{
		printk(KERN_EMERG "Error:unable to change file operations of device.\n");
		return -EINVAL;
	}
	if(cdevp==NULL)
	{
#ifdef DEBUG
		printk(KERN_INFO "Error:%s:Unable to get cdev pointer.\n",__FUNCTION__);
#endif
		return -EINVAL;
	}
	find_dev=1;
#ifdef DEBUG	
	printk(KERN_INFO "get cdev->dev_no:%d,major=%d,minor=%d\n",cdevp->dev,MAJOR(cdevp->dev),MINOR(cdevp->dev));
#endif

	chrdev_fops_old=cdevp->ops;
	cdevp->ops=&chrdev_fops_new;
        return error;
}

static int __init chandev_init(void)
{
	int result=0;	
#ifdef DEBUG
        printk(KERN_INFO "nsccsz_nvidia0 module start.\n");
#endif

	if(cdev_major)
	{
		cdev_no=MKDEV(cdev_major,cdev_minor);
#ifdef DEBUG
		printk(KERN_INFO "cdev-no=%d.\n",cdev_no);
#endif
	}
	else
	{
		result=alloc_chrdev_region(&cdev_no,cdev_minor,1,"nsccsz_nvidia0");
		cdev_major=MAJOR(cdev_no);
#ifdef DEBUG
		printk(KERN_INFO "cdev_major=%d,cdev_minor=%d,cdev_no=%d.\n",
						cdev_major,cdev_minor,cdev_no);
#endif
	}
	chandev=cdev_alloc();
	if(chandev==NULL)
	{
#ifdef DEBUG
		printk(KERN_INFO "Unable to alloc nsccsz_nvidia0.\n");
#endif
		return -EINVAL;
	}
	chandev->owner=THIS_MODULE;
	chandev->ops=&chandev_fops_own;
	cdev_init(chandev,&chandev_fops_own);
	result=cdev_add(chandev,cdev_no,1);
	if(result)
	{
#ifdef DEBUG
		printk(KERN_INFO "Error adding nsccsz_nvidia0.\n");
#endif
		return -EINVAL;
	}
	
	change_cdev("/dev/nvidia0");
        return 0;
}

static void __exit chandev_exit(void)
{
	if(find_dev==1)
	{
		cdevp->ops=chrdev_fops_old;
#ifdef DEBUG
		printk(KERN_INFO "recover fops of nvidia.\n");
#endif
	}
	test_del_list();	//destroy the head list
	cdev_no=MKDEV(cdev_major,cdev_minor);
	unregister_chrdev_region(cdev_no,1);
	cdev_del(chandev);
        printk(KERN_INFO "nsccsz_nvidia0 module exit.\n");
}

module_init(chandev_init);
module_exit(chandev_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Peng Zhenyi");
MODULE_DESCRIPTION("A simple module changes the file_operations of nvidia.");
MODULE_ALIAS("A simple module.");
