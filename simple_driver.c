#include <linux/kernel.h>
#include <linux/module.h>   //  MODULE_AUTHOR, MODULE_DESCRIPTION, etc..
#include <linux/fs.h>       // inode, file, should already include types
#include <linux/kdev_t.h>   // MAJOR, MINOR
#include <linux/types.h>    // ssize_t, dev_t
#include <linux/slab.h>     // kmalloc, kfree
#include <linux/cdev.h>     // cdev
#include <linux/uaccess.h>  // copy_from_user / copy_to_user
#include <linux/device.h>   // device_create / destroy

MODULE_AUTHOR("Keer Mei");
MODULE_DESCRIPTION("A SIMPLE CHAR DRIVER");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.0");

#define BUFFER_SIZE         3
#define CHAR_DEVICE_NAME    "my_device_driver"
#define CHAR_CLASS_NAME     "my_device_class"

// define a special data type
struct my_device_data {
    struct cdev cdev;
    char *data;
    size_t size;
};

// initialize a data structure to hold data
struct my_device_data my_devs;

// setup class and device for user space and device registration
static struct class *my_class;  // for udev
dev_t my_devt;

/* ====================================================================================================================================================== */

/* setup prototypes here, actuallly the prototypes names don't matter*/
static int my_open(struct inode *inode, struct file *file);
static int my_close(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char *user_buffer, size_t size, loff_t *offset);
static ssize_t my_write(struct file *file, const char *user_buffer, size_t size, loff_t *offset);
static loff_t my_llseek(struct file *file, loff_t offset, int whence);

static int my_open(struct inode *inode, struct file *file)
{   
    // initialize a device
    struct my_device_data *my_data;

    // inodes will have a container_of macro wrapper? this is a linux macro
    // that returns a pointer to the cdev member. you can use this to identify the device
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);

    // stores the data into the file
    file->private_data = my_data;

    pr_info("Device opened successfully\n");
    return 0;
};

static int my_close(struct inode *inode, struct file *file)
{
    pr_info("Device closed successfully\n");
    return 0;
};


// __user is a linux macro here
static ssize_t my_read(struct file *file, char *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;
    // accesses the data to read from the file, notice not the inode
    my_data = (struct my_device_data *) file->private_data;

    // this is actually the same thing because we allocated BUFFER_SIZE in kmalloc()
    ssize_t len = min(my_data->size, size);

    if (len == 0) {
        pr_info("Reached the end of the device for reading\n");
    }

    if (copy_to_user(user_buffer, my_data->data, len) != 0)
        return -EFAULT;
    
    pr_info("Device has been read\n");
    pr_info("current offset: %lld\n", *offset);
    *offset += len; // add to the offset
    pr_info("offset has been updated %ld\n", len);
    return len;
};

static ssize_t my_write(struct file *file, const char *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;
    my_data = (struct my_device_data *) file->private_data;
    ssize_t len = min(my_data->size - *offset, size);

    if (len == 0) {
        pr_info("Reached the end of the device for writing\n");
    }

    if (copy_from_user(my_data->data + *offset, user_buffer, len) != 0)
        return -EFAULT;
    
    pr_info("Device has been written\n");
    pr_info("current offset: %lld\n", *offset);
    *offset += len;
    pr_info("offset has been updated %ld\n", len);
    return len;
};

static loff_t my_llseek(struct file *file, loff_t offset, int whence)
{
    pr_info("lseek function begin\n");
    loff_t new_pos = 0;
    switch(whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = BUFFER_SIZE - offset;
        break;
    }

    if (new_pos > BUFFER_SIZE)
        new_pos = BUFFER_SIZE;
    if (new_pos < 0)
        new_pos = 0;
    file->f_pos = new_pos;
    return new_pos;
};

const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_close,
    .llseek = my_llseek,
};
/* ====================================================================================================================================================== */

/*kernel functions already provided, just here for reference:
*    int register_chrdev_region(dev_t first, unsigned int count, char *name);
*    void unregister_chrdev_region(dev_t first, unsigned int count);
*    void cdev_init(struct cdev *cdev, struct file_operations *fops); cdev_init is if memory is already allocated for you, use cdev_alloc if not 
*    int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
*    void cdev_del(struct cdev *dev);
*/ 

int init_fn(void)
{
    int i;

    // register the device major and minor if you know it
    // err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS, "my_device_driver");
    // do it this way if you dont have the actual numbers
    if ((alloc_chrdev_region(&my_devt, 0, 1, CHAR_DEVICE_NAME)) < 0) {
        pr_info("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n", MAJOR(my_devt), MINOR(my_devt));
    
    // create class
    my_class = class_create(THIS_MODULE, CHAR_CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_info("Failed to create class\n");
        goto r_class;
    }

    // register device
    if (IS_ERR(device_create(my_class, NULL, my_devt, NULL, CHAR_DEVICE_NAME))) {
        pr_info("Failed to create device\n");
        goto r_device;
    }
    
    // initialize the devs[MY_MAX_MINORS] array
    cdev_init(&my_devs.cdev, &my_fops);
    
    // register the minor numbers
    if ((cdev_add(&my_devs.cdev, my_devt, 1)) < 0) {
        pr_info("Cannot add %d device to the system\n", i);
        goto r_device;
    }
    
    // allocate memory for data structure
    my_devs.data = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (my_devs.data == 0) {
        pr_info("Cannot allocate memory for devs[%d]\n", i);
    }

    // set the size
    my_devs.size = BUFFER_SIZE;

    pr_info("Kernel Module inserted successfully...\n");
    return 0;

// gotos are fallthrough
r_device:
    class_destroy(my_class);
// this label will get executed anyways
r_class:
    unregister_chrdev_region(my_devt, 1);
    return -1;
};

void cleanup_fn(void)
{   
    kfree(my_devs.data);
    device_destroy(my_class, my_devt);
    class_destroy(my_class);
    // cleanup the devs array
    cdev_del(&my_devs.cdev);
    unregister_chrdev_region(my_devt, 1);
    pr_info("Device driver removed successfully\n");
};

module_init(init_fn);
module_exit(cleanup_fn);
