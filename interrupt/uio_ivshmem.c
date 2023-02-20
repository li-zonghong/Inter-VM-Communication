/*
 * UIO IVShmem Driver
 *
 * (C) 2009 Cam Macdonell
 * (C) 2017 Henning Schild
 * based on Hilscher CIF card driver (C) 2007 Hans J. Koch <hjk@linutronix.de>
 *
 * Licensed under GPL version 2 only.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include <linux/io.h>
#include "my_queue.h"

#define IntrStatus 0x04
#define IntrMask 0x00

#define INTERRUPT_NUM 1
#define BUFF_SIZE 0x1000

struct ivshmem_info {
	struct uio_info *uio;
	struct pci_dev *dev;
};

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

//----------------------------------/
	struct packet_msg *rev_msg;
	char *rev_info_buff;
	int rev_info_ID;
	int rev_info_size;
//----------------------------------/

static void rev_init(void)
{
	rev_info_ID = readl( ivshmem_dev.regs_base_addr + 8);
	printk("IVSHMEM: my posn is %d\n", rev_info_ID);
	
	rev_info_buff = ivshmem_dev.data_base_addr + rev_info_ID * BUFF_SIZE;
	rev_info_size = BUFF_SIZE/packet_length;
	rev_msg = (struct packet_msg *)rev_info_buff;
}


static irqreturn_t ivshmem_handler(int irq, struct uio_info *dev_info)
{
	printk("uio_irq");
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

	return IRQ_HANDLED;
}

static int ivshmem_pci_probe(struct pci_dev *dev,
					const struct pci_device_id *id)
{
	struct uio_info *info;
	struct ivshmem_info *ivshmem_info;
	info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ivshmem_info = kzalloc(sizeof(struct ivshmem_info), GFP_KERNEL);
	if (!ivshmem_info) {
		kfree(info);
		return -ENOMEM;
	}
	info->priv = ivshmem_info;

	if (pci_enable_device(dev))
		goto out_free;

	if (pci_request_regions(dev, "ivshmem"))
		goto out_disable;

	info->mem[0].addr = pci_resource_start(dev, 0);
	if (!info->mem[0].addr)
		goto out_release;

	info->mem[0].size = (pci_resource_len(dev, 0) + PAGE_SIZE - 1)
		& PAGE_MASK;
	info->mem[0].internal_addr = pci_ioremap_bar(dev, 0);
	if (!info->mem[0].internal_addr)
		goto out_release;

	if (1 > pci_alloc_irq_vectors(dev, 1, 1,
				      PCI_IRQ_LEGACY | PCI_IRQ_MSIX))
		goto out_vector;

	info->mem[0].memtype = UIO_MEM_PHYS;
	info->mem[0].name = "registers";

	info->mem[1].addr = pci_resource_start(dev, 2);
	if (!info->mem[1].addr)
		goto out_unmap;

	info->mem[1].size = pci_resource_len(dev, 2);
	info->mem[1].memtype = UIO_MEM_PHYS;
	info->mem[1].name = "shmem";

	ivshmem_info->uio = info;
	ivshmem_info->dev = dev;

	if (pci_irq_vector(dev, 0)) {
		info->irq = pci_irq_vector(dev, 0);
		info->irq_flags = IRQF_SHARED;
		info->handler = ivshmem_handler;
	} else {
		dev_warn(&dev->dev, "No IRQ assigned to device: "
			 "no support for interrupts?\n");
	}
	pci_set_master(dev);

	info->name = "uio_ivshmem";
	info->version = "0.0.1";

	if (uio_register_device(&dev->dev, info))
		goto out_unmap;

	if (!dev->msix_enabled)
		writel(0xffffffff, info->mem[0].internal_addr + IntrMask);

	pci_set_drvdata(dev, ivshmem_info);
	/* bar2: data mmio region */
	
	ivshmem_dev.data_base_addr = pci_iomap(dev, 2, 0);
	ivshmem_dev.regs_base_addr = pci_iomap(dev, 0, 0x100);
	rev_init();
	return 0;
out_vector:
	pci_free_irq_vectors(dev);
out_unmap:
	iounmap(info->mem[0].internal_addr);
out_release:
	pci_release_regions(dev);
out_disable:
	pci_disable_device(dev);
out_free:
	kfree(ivshmem_info);
	kfree(info);
	return -ENODEV;
}

static void ivshmem_pci_remove(struct pci_dev *dev)
{
	struct ivshmem_info *ivshmem_info = pci_get_drvdata(dev);
	struct uio_info *info = ivshmem_info->uio;

	pci_set_drvdata(dev, NULL);
	uio_unregister_device(info);
	pci_free_irq_vectors(dev);
	iounmap(info->mem[0].internal_addr);
	pci_release_regions(dev);
	pci_disable_device(dev);
	kfree(info);
	kfree(ivshmem_info);
}

static struct pci_device_id ivshmem_pci_ids[] = {
	{
		.vendor =	0x1af4,
		.device =	0x1110,
		.subvendor =	PCI_ANY_ID,
		.subdevice =	PCI_ANY_ID,
	},
	{ 0, }
};

static struct pci_driver ivshmem_pci_driver = {
	.name = "uio_ivshmem",
	.id_table = ivshmem_pci_ids,
	.probe = ivshmem_pci_probe,
	.remove = ivshmem_pci_remove,
};

module_pci_driver(ivshmem_pci_driver);
MODULE_DEVICE_TABLE(pci, ivshmem_pci_ids);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Cam Macdonell");
