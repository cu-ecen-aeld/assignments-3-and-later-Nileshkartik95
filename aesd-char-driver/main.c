/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Nileshkartik Ashokkumar");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_dev;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
	struct aesd_dev *dev;
	ssize_t rd_count = 0;
    ssize_t bytes_rd = 0;
	ssize_t rd_offset = 0;
    struct aesd_buffer_entry *rd_idx = NULL;

    if ((buf == NULL) ||(filp == NULL) || (f_pos == NULL))
    {
        return -EFAULT;
    }
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    dev = (struct aesd_dev *)filp->private_data;
    if (mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG(KERN_ERR "mutex lock failed");
        return -EFAULT;
    }
    rd_idx = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circle_buff), *f_pos, &rd_offset);
    if (rd_idx == NULL)
    {
        mutex_unlock(&(dev->lock));
		return bytes_rd;
    }
    else
    {
        count = (count > (rd_idx->size - rd_offset))? (rd_idx->size - rd_offset): count;
    }
    if((rd_count = copy_to_user(buf,(rd_idx->buffptr + rd_offset), count))==0)
		PDEBUG("read Complete");
    bytes_rd = count - rd_count;
    *f_pos += bytes_rd;
	mutex_unlock(&(dev->lock));
    return bytes_rd;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
    struct aesd_dev *dev;
	ssize_t bytes_wr = -ENOMEM;
    const char *wr_entry = NULL;
    ssize_t wr_count = 0;

    if (count == 0)
        return 0;
    if ((buf == NULL) ||(filp == NULL) || (f_pos == NULL))
        return -EFAULT;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    dev = (struct aesd_dev *)filp->private_data;

    if (mutex_lock_interruptible(&(dev->lock)))
    {
        PDEBUG(KERN_ERR "mutex lock failed");
        return -EFAULT;
    }

    if (dev->circle_buff_entry.size == 0)
    {
        PDEBUG("Allocating buffer");
        dev->circle_buff_entry.buffptr = kmalloc(count * sizeof(char), GFP_KERNEL);
        if (dev->circle_buff_entry.buffptr == NULL)
        {
            PDEBUG("memory alloc failure");
            mutex_unlock(&dev->lock);
			return bytes_wr;
        }
    }
    else
    {
        dev->circle_buff_entry.buffptr = krealloc(dev->circle_buff_entry.buffptr, (dev->circle_buff_entry.size + count), GFP_KERNEL);
        if (dev->circle_buff_entry.buffptr == NULL)
        {
            PDEBUG("memory alloc failure");
            mutex_unlock(&dev->lock);
			return bytes_wr;
        }
    }
    PDEBUG("write from user space buffer to kernel buffer");
    if((wr_count = copy_from_user((void *)(dev->circle_buff_entry.buffptr + dev->circle_buff_entry.size),buf, count))== 0)
		PDEBUG("write Complete");

    bytes_wr = count - wr_count;
    dev->circle_buff_entry.size += bytes_wr;
    if (memchr(dev-> circle_buff_entry.buffptr, '\n', dev->circle_buff_entry.size))
    {
        if((wr_entry = aesd_circular_buffer_add_entry(&dev->circle_buff, &dev->circle_buff_entry)))
        {
            kfree(wr_entry);
        }
        dev-> circle_buff_entry.buffptr = NULL;
        dev->circle_buff_entry.size = 0;
    }
    dev->buf_size += bytes_wr;

    mutex_unlock(&dev->lock);
    return bytes_wr;
}



static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
    struct aesd_dev *dev = filp->private_data;
    int loc_fpos = 0;
    int itr = 0;

    if (mutex_lock_interruptible(&dev->lock))
    {
        /*lock to access the cbuff*/
        PDEBUG(KERN_ERR "mutex lock failed");
        return -ERESTARTSYS;
    }

    if (dev->circle_buff.in_offs == dev->circle_buff.out_offs)
    {
        loc_fpos = dev->circle_buff.full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : 0;
    }
    else if (dev->circle_buff.in_offs < dev->circle_buff.out_offs)
    {
        loc_fpos = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - dev->circle_buff.out_offs + dev->circle_buff.in_offs;
    }
    else
    {
        loc_fpos = dev->circle_buff.in_offs - dev->circle_buff.out_offs;
    }

    if ((write_cmd >= loc_fpos) || ((write_cmd_offset >= dev->circle_buff.entry[write_cmd].size)))
    {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    loc_fpos = 0;
    while(itr++ < write_cmd)
    {
        /*calculated start offset to write command*/
        loc_fpos += dev->circle_buff.entry[itr].size;
    }    
    mutex_unlock(&dev->lock);

    loc_fpos += write_cmd_offset;
    filp->f_pos = loc_fpos;
    return 0;
}

/*reference : https://github.com/starpos/scull/blob/master/scull/main.c*/
loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
	struct aesd_dev *dev = filp->private_data;
	loff_t cur_pos;

	switch(whence) {
	  case SEEK_SET: /* SEEK_SET */
		cur_pos = off;
		break;

	  case SEEK_CUR: /* SEEK_CUR */
		cur_pos = filp->f_pos + off;
		break;

	  case SEEK_END: /* SEEK_END */
		cur_pos = dev->buf_size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (cur_pos < 0) return -EINVAL;
	filp->f_pos = cur_pos;
	return cur_pos;
}



/*reference : https://github.com/starpos/scull/blob/master/scull/main.c*/
long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_seekto seekto;
    int retval = 0;

	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

    switch (cmd)
    {
        case AESDCHAR_IOCSEEKTO:
        {
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0)
            {
                retval = -EFAULT;
            }
            else
            {
                retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
            }
        }
        break; 
        default:
        {
            retval = -ENOTTY;
        }
        break;
    }

    return retval;
}

struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_dev, 0, sizeof(struct aesd_dev));
    mutex_init(&aesd_dev.lock);
    aesd_circular_buffer_init(&aesd_dev.circle_buff);
    result = aesd_setup_cdev(&aesd_dev);
    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    uint8_t idx = 0;
	struct aesd_buffer_entry *entry = NULL;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_dev.cdev);
    kfree(aesd_dev.circle_buff_entry.buffptr);
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_dev.circle_buff, idx)
        if (entry->buffptr != NULL)
            kfree(entry->buffptr);
    unregister_chrdev_region(devno, 1);
	PDEBUG("cleanup complete");
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

