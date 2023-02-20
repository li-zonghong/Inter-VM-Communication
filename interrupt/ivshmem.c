#define pr_fmt(fmt) "%s:%d:: " fmt, __func__, __LINE__
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/ratelimit.h>
#include <linux/msi.h>
#include "my_queue.h"
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#define IVSHMEM_DEV_NAME "ivshmem"
#define INTERRUPT_NUM 1
#define BUFF_SIZE 0x1000
/* ------------------------------------------------------------
 *                         PCI SPECIFIC 
 * ------------------------------------------------------------ */
#include <linux/pci.h>
static struct semaphore sema;
static struct ivshmem_dev{
	/* (mmio) control registers i.e. the "register memory region" */
	void __iomem *regs_base_addr;
	resource_size_t regs_start;
	resource_size_t regs_len;
	/* msi region */
	void __iomem *msi_base_addr;
	resource_size_t msi_start;
	resource_size_t msi_len;
	/* data mmio region */
	void __iomem *data_base_addr;
	resource_size_t data_mmio_start;
	resource_size_t data_mmio_len;
	/* irq handling */
	unsigned int irq;
	/**/
	struct pci_dev *pdev;
	char (*msix_names)[256];
	struct msix_entry *msix_entries;
	int nvectors;
	bool		 enabled;
} ivshmem_dev;

/*static struct rev_info{
	char *buff;
	int ID;
	int size;
} rev_info;*/

//----------------------------------/
	struct packet_msg *rev_msg;
	char *rev_info_buff;
	int rev_info_ID;
	int rev_info_size;
//----------------------------------/
int count ; 
struct timer_list mytimer; 
//----------------------------------/

static struct msix_tacle{
	int addressl;
	int addressh;
	int data;
	int vc;
} *mst;
static struct pci_device_id ivshmem_id_table[] = {
	{0x1af4, 0x1110, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0},
};
MODULE_DEVICE_TABLE(pci, ivshmem_id_table);

#define IVSHMEM_IRQ_ID "ivshmem_irq_id"

/*  relevant control register offsets */
enum {
	IntrMask = 0x00,	/* Interrupt Mask */
	IntrStatus = 0x04,	/* Interrupt Status */
	IvPosition = 0x08,
	Doorbell = 0x0c, 
};
struct tasklet_struct my_tasklet;
static irqreturn_t ivshmem_interrupt(int irq, void *dev_id)
{
	//printk("int1");
	tasklet_schedule(&my_tasklet);
	return IRQ_HANDLED;
}
void my_tasklet_func(unsigned long data)
{
//printk("int");
	int i;
    	for(i=0;i<rev_info_size;i++)
    	{
    		if(rev_msg[i].value%2!=0)
    		{
    			//printk("rev :%s\n", rev_msg[i].p);
    			rev_msg[i].value = 0;
    			break;
    		}
    	}
}

/*
static int send_test(int id)
{
	while(1)
	{
		
	}
}*/
void time_func(struct timer_list * arg)
{
	count++;
	 printk("%d",count);
 }
static void rev_init(void)
{
	rev_info_ID = readl( ivshmem_dev.regs_base_addr + IvPosition);
	printk("IVSHMEM: my posn is %d\n", rev_info_ID);
	
	rev_info_buff = ivshmem_dev.data_base_addr + rev_info_ID * BUFF_SIZE;
	rev_info_size = BUFF_SIZE/packet_length;
	rev_msg = (struct packet_msg *)rev_info_buff;
}
void init_time(void)
{
    init_timer_key(&mytimer,time_func,0,0,1000);     //初始化定时器
    mytimer.expires = jiffies+100;//设定超时时间，100代表1秒
    add_timer(&mytimer); //添加定时器，定时器开始生效
}

static int request_msix_vectors(void)
{
	int i, total_vecs , num_vectors ;
	struct pci_dev *pdev = ivshmem_dev.pdev ;
	struct device *dev = &pdev->dev;
	
	//初始化msix_entries
	num_vectors =pci_alloc_irq_vectors(pdev, 1, 1,
				      PCI_IRQ_MSIX);
	ivshmem_dev.msix_entries = kzalloc((sizeof(struct msix_entry) * INTERRUPT_NUM), 	GFP_KERNEL);
	for (i=0; i<INTERRUPT_NUM; i++) {
   	 ivshmem_dev.msix_entries[i].entry = i;
	}
	ivshmem_dev.msix_entries[0].vector = pci_irq_vector(pdev, 0);
	// 获取PCIE 设备支持的中断向量总数，pdev 为probe传入的struct pci_dev 
	//num_vectors =  pci_enable_msix_range(pdev,ivshmem_dev.msix_entries,INTERRUPT_NUM,INTERRUPT_NUM);
	ivshmem_dev.nvectors = INTERRUPT_NUM;
	/* by default initialize semaphore to 0 */
	//sema_init(&sema, 0);
	
	if (request_irq(ivshmem_dev.msix_entries[0].vector, ivshmem_interrupt,0,
			IVSHMEM_DEV_NAME, NULL)<0)
		dev_err(dev, "request_irq %d error\n", ivshmem_dev.msix_entries[0].vector);
	return 0;
}
static int ivshmem_probe(struct pci_dev *pdev,
			 const struct pci_device_id *pdev_id)
{

	int err,i;
	struct device *dev = &pdev->dev;
	ivshmem_dev.pdev = pdev;
	if ((err = pci_enable_device(ivshmem_dev.pdev))) {
		dev_err(dev, "pci_enable_device probe error %d for device %s\n",
			err, pci_name(pdev));
		return err;
	}

	if ((err = pci_request_regions(pdev, IVSHMEM_DEV_NAME)) < 0) {
		dev_err(dev, "pci_request_regions error %d\n", err);
		goto pci_disable;
	}

	/* bar2: data mmio region */
	ivshmem_dev.data_mmio_start = pci_resource_start(pdev, 2);
	ivshmem_dev.data_mmio_len = pci_resource_len(pdev, 2);
	ivshmem_dev.data_base_addr = pci_iomap(pdev, 2, 0);
	if (!ivshmem_dev.data_base_addr) {
		dev_err(dev, "cannot iomap region of size %lu\n",
			(unsigned long)ivshmem_dev.data_mmio_len);
		goto pci_release;
	}
	dev_info(dev, "data_mmio iomap base = 0x%lx \n",
		 (unsigned long)ivshmem_dev.data_base_addr);
	dev_info(dev, "data_mmio_start = 0x%lx data_mmio_len = %lu\n",
		 (unsigned long)ivshmem_dev.data_mmio_start,
		 (unsigned long)ivshmem_dev.data_mmio_len);
	
	/* bar1: msi region  */
	ivshmem_dev.msi_start = pci_resource_start(pdev, 1);
	ivshmem_dev.msi_len = pci_resource_len(pdev, 1);
	ivshmem_dev.msi_base_addr = pci_iomap(pdev, 1, 0);
	if (!ivshmem_dev.msi_base_addr) {
		dev_err(dev, "cannot ioremap registers of size %lu\n",
			(unsigned long)ivshmem_dev.regs_len);
		goto reg_release;
	}
	dev_info(dev, "data_msi iomap base = 0x%lx \n",
		 (unsigned long)ivshmem_dev.msi_base_addr);
	dev_info(dev, "data_msi_start = 0x%lx data_msi_len = %lu\n",
		 (unsigned long)ivshmem_dev.msi_start,
		 (unsigned long)ivshmem_dev.msi_len);
	
	/* bar0: control registers */
	ivshmem_dev.regs_start = pci_resource_start(pdev, 0);
	ivshmem_dev.regs_len = pci_resource_len(pdev, 0);
	ivshmem_dev.regs_base_addr = pci_iomap(pdev, 0, 0x100);
	if (!ivshmem_dev.regs_base_addr) {
		dev_err(dev, "cannot ioremap registers of size %lu\n",
			(unsigned long)ivshmem_dev.regs_len);
		goto reg_release;
	}
	dev_info(dev, "regs iomap base = 0x%lx, irq = %u\n",
		 (unsigned long)ivshmem_dev.regs_base_addr, pdev->irq);
	dev_info(dev, "regs_addr_start = 0x%lx regs_len = %lu\n",
		 (unsigned long)ivshmem_dev.regs_start,
		 (unsigned long)ivshmem_dev.regs_len);
	
	/* interrupts: set all masks */
	writel(0xffffffff, ivshmem_dev.regs_base_addr + IntrMask);
	request_msix_vectors();
	if(pdev->msix_enabled)printk("IVSHMEM MSIX ENABLED");
	rev_init();
	//init_time();
	 tasklet_init(&my_tasklet,my_tasklet_func,0);
	return 0;

reg_release:
	pci_iounmap(pdev, ivshmem_dev.data_base_addr);
pci_release:
	pci_release_regions(pdev);
pci_disable:
	pci_disable_device(pdev);
	return -EBUSY;
}

static void ivshmem_remove(struct pci_dev *pdev)
{

	pci_free_irq_vectors(pdev);
	pci_iounmap(pdev, ivshmem_dev.regs_base_addr);
	pci_iounmap(pdev, ivshmem_dev.data_base_addr);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

}

static struct pci_driver ivshmem_pci_driver = {
	.name = IVSHMEM_DEV_NAME,
	.id_table = ivshmem_id_table,
	.probe = ivshmem_probe,
	.remove = ivshmem_remove,
};

/* -------------------------------------------------------------
 *                    THE FILE OPS
 * ------------------------------------------------------------*/
static int ivshmem_major;
#define IVSHMEM_MINOR 0

static long ivshmem_ioctl( struct file * filp,
			unsigned int cmd, unsigned long arg)
{
	uint32_t msg=0;
	//printk("KVM_IVSHMEM: args is %ld\n", arg);
#if 1
	switch (cmd) {
		case 1://read my position
			msg = readl( ivshmem_dev.regs_base_addr + IvPosition);
			printk("KVM_IVSHMEM: my posn is %d\n", msg);
			//return msg;
			break;
		case 2://Trigger interrupt through doorbell
			msg = ((arg & 0xff) << 16) + (0 & 0xff);
			printk("KVM_IVSHMEM: ringing sema doorbell\n");
			writel(msg, ivshmem_dev.regs_base_addr + Doorbell);
			break;
		default:
			printk("KVM_IVSHMEM: bad ioctl (\n");
	}
#endif

	return 0;
}
static int ivshmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	if ((offset + (vma->vm_end - vma->vm_start)) >
	    ivshmem_dev.data_mmio_len)
		return -EINVAL;

	offset += (unsigned long)ivshmem_dev.data_mmio_start;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start,
			       offset >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int ivshmem_open(struct inode *inode, struct file *filp)
{
	if (MINOR(inode->i_rdev) != IVSHMEM_MINOR) {
		pr_info("minor: %d\n", IVSHMEM_MINOR);
		return -ENODEV;
	}
	return 0;
}

static int ivshmem_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static const struct file_operations ivshmem_ops = {
	.owner = THIS_MODULE,
	.open = ivshmem_open,
	.mmap = ivshmem_mmap,
	.unlocked_ioctl = ivshmem_ioctl,
	.release = ivshmem_release,
};

/* -----------------------------------------------------------
 *                  MODULE INIT/EXIT
 * ----------------------------------------------------------- */
#define IVSHMEM_DEVS_NUM 1	/* number of devices */
static struct cdev cdev;	/* char device abstraction */
static struct class *ivshmem_class;	/* linux device model */

static int __init ivshmem_init(void)
{
	int err = -ENOMEM;

	/* obtain major */
	dev_t mjr = MKDEV(ivshmem_major, 0);
	if ((err =
	     alloc_chrdev_region(&mjr, 0, IVSHMEM_DEVS_NUM,
				 IVSHMEM_DEV_NAME)) < 0) {
		pr_err("alloc_chrdev_region error\n");
		return err;
	}
	ivshmem_major = MAJOR(mjr);

	/* connect fops with the cdev */
	cdev_init(&cdev, &ivshmem_ops);
	cdev.owner = THIS_MODULE;

	/* connect major/minor to the cdev */
	{
		dev_t devt;
		devt = MKDEV(ivshmem_major, IVSHMEM_MINOR);

		if ((err = cdev_add(&cdev, devt, 1))) {
			pr_err("cdev_add error\n");
			goto unregister_dev;
		}
	}

	/* populate sysfs entries */
	if (!(ivshmem_class = class_create(THIS_MODULE, IVSHMEM_DEV_NAME))) {
		pr_err("class_create error\n");
		goto del_cdev;
	}

	/* create udev '/dev' node */
	{
		dev_t devt = MKDEV(ivshmem_major, IVSHMEM_MINOR);
		if (!
		    (device_create
		     (ivshmem_class, NULL, devt, NULL, IVSHMEM_DEV_NAME "%d",
		      IVSHMEM_MINOR))) {
			pr_err("device_create error\n");
			goto destroy_class;
		}
	}

	/* register pci device driver */
	if ((err = pci_register_driver(&ivshmem_pci_driver)) < 0) {
		pr_err("pci_register_driver error\n");
		goto exit;
	}

	return 0;

exit:
	device_destroy(ivshmem_class, MKDEV(ivshmem_major, IVSHMEM_MINOR));
destroy_class:
	class_destroy(ivshmem_class);
del_cdev:
	cdev_del(&cdev);
unregister_dev:
	unregister_chrdev_region(MKDEV(ivshmem_major, IVSHMEM_MINOR),
				 IVSHMEM_DEVS_NUM);

	return err;
}

static void __exit ivshmem_fini(void)
{
	pci_unregister_driver(&ivshmem_pci_driver);
	device_destroy(ivshmem_class, MKDEV(ivshmem_major, IVSHMEM_MINOR));
	class_destroy(ivshmem_class);
	cdev_del(&cdev);
	unregister_chrdev_region(MKDEV(ivshmem_major, IVSHMEM_MINOR),
				 IVSHMEM_DEVS_NUM);
}

module_init(ivshmem_init);
module_exit(ivshmem_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Demo module for QEMU ivshmem virtual pci device");
