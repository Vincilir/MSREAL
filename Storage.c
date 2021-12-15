#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#define BUFF_SIZE 20

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

char storage[4];
int result;
int carry;
int pos = 0;
int endRead = 0;

int storage_open(struct inode *pinode, struct file *pfile);
int storage_close(struct inode *pinode, struct file *pfile);
ssize_t storage_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t storage_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);


struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = storage_open,
	.read = storage_read,
	.write = storage_write,
	.release = storage_close,
	
};


int storage_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

int storage_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}

ssize_t storage_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len;
	if (endRead){
		endRead = 0;
		pos = 0;
		printk(KERN_INFO "Succesfully read from file\n");
		return 0;
	}
	len = scnprintf(buff,BUFF_SIZE , "%d %d ", result, carry); //ispis vrednosti registra result i carry
	
	//len = scnprintf(buff,BUFF_SIZE , "%d ", storage[pos]); //test
	ret = copy_to_user(buffer, buff, len);
	if(ret)
		return -EFAULT;
	pos++;
	if(pos == 1)
		endRead = 1;
	return len;
}



ssize_t storage_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
	char buff[BUFF_SIZE];
	int value;
	int ret;
	char reg[]="";
	

	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	buff[length-1] = '\0';

	ret = sscanf(buff,"%s %d",reg, &value);

	if(ret==2)
	{
		if(strcmp(reg,"regA=") == 0)				//komparacija dva stringa
		{	
			storage[0] = value; 				//smestanje promenljive u niz koji predstavlja regA(pozicija 0)
			printk(KERN_INFO "Uspesno upisana %d u regA\n", value); 
		}
		else
		{
			printk(KERN_WARNING "Vrednost  nije upisan u regA\n"); 
		}
		if(strcmp(reg,"regB=") == 0)
		{
			storage[1] = value; 
			printk(KERN_INFO "Uspesno upisana %d u regB\n", value); 
		}
		else
		{
			printk(KERN_WARNING "Vrednost  nije upisan u regB\n"); 
		}
		if(strcmp(reg,"regC=") == 0)
		{
			storage[2] = value; 
			printk(KERN_INFO "Uspesno upisana %d u regC\n", value); 
		}
		else
		{
			printk(KERN_WARNING "Vrednost nije upisan u regC\n"); 
		}
		if(strcmp(reg,"regD=") == 0)
		{
			storage[3] = value; 
			printk(KERN_INFO "Uspesno upisana %d u regD\n", value); 
		}
		else
		{
			printk(KERN_WARNING "Vrednost  nije upisan u regD\n"); 
		}
	}
	else
	{
		printk(KERN_WARNING "Wrong command format\nexpected: n,m\n\tn-position\n\tm-value\n");
	}
	
	

	return length;
}

static int __init storage_init(void)
{
   int ret = 0;
	int i=0;

	//inicijalizacija 
	for (i=0; i<4; i++)
		storage[i] = 1;
	result = 0;
	carry = 0;
	

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "storage");//alokacija memorije za registre 
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   
   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "result");//alokacija memorije za promenljivu result
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   
   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "carry");//alokacija memorije za promenljivu carry
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "storage_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "storage");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit storage_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(storage_init);
module_exit(storage_exit);
