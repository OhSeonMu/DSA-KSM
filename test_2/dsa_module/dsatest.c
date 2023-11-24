#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <uapi/linux/idxd.h>
#include <linux/idxd.h>
#include <linux/smp.h>
#include <asm-generic/io.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/random.h>

#define IDXD_IOCAL_GET_WQ 0
static char* dsa_cdev_name = "/dev/dma_dsa/wq2.0";
static struct idxd_wq *wq;

#define EXAMPLES		3
#define XFER_SIZE	 4096
#define BATCH_SIZE		16
#define BUF_SIZE		16

#define UMWAIT_STATE_C0_2 0
#define UMWAIT_STATE_C0_1 1
#define MAX_COMP_RETRY 1000
#define UMWAIT_DELAY 100000

static inline unsigned char
_umwait(unsigned int state, unsigned long long timeout) {
  uint8_t r;
  uint32_t timeout_low = (uint32_t)timeout;
  uint32_t timeout_high = (uint32_t)(timeout >> 32);
  asm volatile(".byte 0xf2, 0x48, 0x0f, 0xae, 0xf1\t\n"
    "setc %0\t\n" :
    "=r"(r) :
    "c"(state), "a"(timeout_low), "d"(timeout_high));
  return r;
}

static inline void
_umonitor(void *addr) {
  asm volatile(".byte 0xf3, 0x48, 0x0f, 0xae, 0xf0" : : "a"(addr));
}
static inline
void _clflushopt(volatile void *__p) {
  asm volatile("clflushopt %0" : "+m" (*(volatile char  *)__p));
}

static inline void
cflush(char *buf, uint64_t len) {
  char *b = buf;
  char *e = buf + len;
  for (; b < e; b += 64)
    _clflushopt(b);
}

int async(dma_addr_t data_dma_buf[][BUF_SIZE], struct dsa_hw_desc *desc_buf,
		struct dsa_completion_record **comp_buf, dma_addr_t *comp_dma_buf, 
		void *wq_portal, struct idxd_device *idxd)
{
	int retry, status;
	uint64_t start;
	uint64_t prep = 0;
	uint64_t submit = 0;
	uint64_t wait = 0;
	uint64_t delay;
	int i;

	pr_info("[info  ] running async offload\n");

	start = get_cycles();
	for (i = 0; i < BUF_SIZE; i++) {
		comp_buf[i]->status = 0;
		desc_buf[i].priv = 1;
		if (device_pasid_enabled(idxd))
			desc_buf[i].pasid = idxd->pasid;
		else
			desc_buf[i].pasid = 0;
		desc_buf[i].opcode = DSA_OPCODE_COMPARE;
		desc_buf[i].flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV;
		// desc_buf[i].flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_CC;
		desc_buf[i].xfer_size = XFER_SIZE;
		desc_buf[i].src_addr = data_dma_buf[0][i];
		desc_buf[i].dst_addr = data_dma_buf[1][i];
		desc_buf[i].completion_addr = comp_dma_buf[i];

	}
	prep += get_cycles() - start;


	start = get_cycles();
	for (i = 0; i < BUF_SIZE; i++) {
		wmb();
		iosubmit_cmds512(wq_portal, &desc_buf[i], 1); // dedicated
		// TODO: shared
	}
	submit += get_cycles() - start;

	retry = 0;
	start = get_cycles();
	for (i = 0; i < BUF_SIZE; i++) {
		while (comp_buf[i]->status == 0 && retry++ < MAX_COMP_RETRY) {
			_umonitor(comp_buf[i]);
			if (comp_buf[i]->status == 0) {
				delay = get_cycles() + UMWAIT_DELAY;
				_umwait(UMWAIT_STATE_C0_1, delay);
			}
		}
	}
	wait += get_cycles() - start;

	pr_info("[time  ] preparation: %llu\n", prep);
	pr_info("[time  ] submission: %llu\n", submit);
	pr_info("[time  ] wait: %llu\n", wait);
	pr_info("[time  ] full offload: %llu\n", prep + submit + wait);

	status = 1;
	for (i = 0; i < BUF_SIZE; i++) {
		if (comp_buf[i]->status != 1) {
			status = comp_buf[i]->status;
			break;
		}
	}

	return status;
}

int batch(dma_addr_t data_dma_buf[][BUF_SIZE], struct dsa_hw_desc *desc_buf,
		struct dsa_completion_record **comp_buf, dma_addr_t *comp_dma_buf, 
		void *wq_portal, struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	dma_addr_t batch_comp_dma, desc_buf_dma;
	struct dsa_hw_desc batch_desc = {};
	struct dsa_completion_record *batch_comp = dma_alloc_coherent(dev, 
			idxd->data->compl_size, &batch_comp_dma, GFP_KERNEL);
	int retry, status;
	uint64_t start;
	uint64_t prep = 0;
	uint64_t submit = 0;
	uint64_t wait = 0;
	uint64_t delay;
	int i;

	pr_info("[info  ] running batch offload\n");

	desc_buf_dma = dma_map_page(dev, virt_to_page(desc_buf),
								(size_t)desc_buf % PAGE_SIZE, BUF_SIZE * sizeof(*desc_buf), DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dev, desc_buf_dma))
		pr_err("desc_buf_dma error!\n");

	start = get_cycles();

	for (i = 0; i < BATCH_SIZE; i++) {
		comp_buf[i]->status = 0;
		desc_buf[i].opcode = DSA_OPCODE_COMPARE;
		desc_buf[i].flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV;
		desc_buf[i].xfer_size = XFER_SIZE;
		desc_buf[i].src_addr = data_dma_buf[0][i];
		desc_buf[i].dst_addr = data_dma_buf[1][i];
		desc_buf[i].completion_addr = comp_dma_buf[i];
	}

	cflush((char*)desc_buf, BUF_SIZE * sizeof(*desc_buf));

	batch_comp->status = 0;
	batch_desc.priv = 1;
	if (device_pasid_enabled(idxd))
		batch_desc.pasid = idxd->pasid;
	else
		batch_desc.pasid = 0;
	batch_desc.opcode = DSA_OPCODE_BATCH;
	batch_desc.flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV;
	batch_desc.desc_count = BATCH_SIZE;
	batch_desc.desc_list_addr = desc_buf_dma;
	batch_desc.completion_addr = batch_comp_dma;

	prep = get_cycles() - start;

	start = get_cycles();
	wmb();
	iosubmit_cmds512(wq_portal, &batch_desc, 1); // dedicated
	// TODO: shared

	submit += get_cycles() - start;

	retry = 0;
	start = get_cycles();

	while (batch_comp->status == 0 && retry++ < MAX_COMP_RETRY) {
		_umonitor(batch_comp);
		if (batch_comp->status == 0) {
			delay = get_cycles() + UMWAIT_DELAY;
			_umwait(UMWAIT_STATE_C0_1, delay);
		}
	}

	wait += get_cycles() - start;

	pr_info("[time  ] preparation: %llu\n", prep);
	pr_info("[time  ] submission: %llu\n", submit);
	pr_info("[time  ] wait: %llu\n", wait);
	pr_info("[time  ] full offload: %llu\n", prep + submit + wait);

	status = batch_comp->status;

	dma_unmap_page(dev, desc_buf_dma, BUF_SIZE * sizeof(*desc_buf), DMA_BIDIRECTIONAL);
	dma_free_coherent(dev, idxd->data->compl_size, batch_comp, batch_comp_dma);

	return status;
}

int single(dma_addr_t data_dma_buf[][BUF_SIZE], struct dsa_hw_desc *desc_buf,
		struct dsa_completion_record **comp_buf, dma_addr_t *comp_dma_buf, 
		void *wq_portal, struct idxd_device *idxd)
{
	int retry, status;
	uint64_t start;
	uint64_t prep = 0;
	uint64_t submit = 0;
	uint64_t wait = 0;
	uint64_t delay;
	int i;

	pr_info("[info  ] running single offload\n");

	for (i = 0; i < BUF_SIZE; i++) {
		
		start = get_cycles();

		comp_buf[i]->status = 0;
		desc_buf[i].priv = 1;
		if (device_pasid_enabled(idxd))
			desc_buf[i].pasid = idxd->pasid;
		else
			desc_buf[i].pasid = 0;
		desc_buf[i].opcode = DSA_OPCODE_COMPARE;
		desc_buf[i].flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV;
		desc_buf[i].xfer_size = XFER_SIZE;
		desc_buf[i].src_addr = data_dma_buf[0][i];
		desc_buf[i].dst_addr = data_dma_buf[1][i];
		desc_buf[i].completion_addr = comp_dma_buf[i];

		prep += get_cycles() - start;

		start = get_cycles();

		wmb();
		iosubmit_cmds512(wq_portal, &desc_buf[i], 1); // dedicated
		// TODO: shared
	
		submit += get_cycles() - start;

		retry = 0;
		start = get_cycles();

		while (comp_buf[i]->status == 0 && retry++ < MAX_COMP_RETRY) {
			_umonitor(comp_buf[i]);
			if (comp_buf[i]->status == 0) {
				delay = get_cycles() + UMWAIT_DELAY;
				_umwait(UMWAIT_STATE_C0_1, delay);
			}
		}

		wait += get_cycles() - start;
	}

	pr_info("[time  ] preparation: %llu\n", prep);
	pr_info("[time  ] submission: %llu\n", submit);
	pr_info("[time  ] wait: %llu\n", wait);
	pr_info("[time  ] full offload: %llu\n", prep + submit + wait);

	status = 1;
	for (i = 0; i < BUF_SIZE; i++) {
		if (comp_buf[i]->status != 1) {
			status = comp_buf[i]->status;
			break;
		}
	}

	return status;
}

static void dsatest_main(void)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	void __iomem *wq_portal = idxd_wq_portal_addr(wq);
	struct dsa_hw_desc *desc_buf;
	struct dsa_completion_record *comp_buf[BUF_SIZE];
	uint64_t *data_buf[2][BUF_SIZE];
	dma_addr_t data_dma_buf[2][BUF_SIZE], comp_dma_buf[BUF_SIZE];
	int status;
	int example, i, j;

	desc_buf = kzalloc_node(BUF_SIZE * sizeof(*desc_buf), GFP_KERNEL, dev_to_node(dev));
	for (j = 0; j < BUF_SIZE; j++)
		comp_buf[j] = dma_alloc_coherent(dev, idxd->data->compl_size, &comp_dma_buf[j], GFP_KERNEL);
	for (i = 0; i < 2; i++) {
		for (j = 0; j < BUF_SIZE; j++) {
			data_buf[i][j] = (uint64_t *)kmalloc(XFER_SIZE, GFP_KERNEL);
			data_dma_buf[i][j] = dma_map_page(dev, virt_to_page(data_buf[i][j]),
									(size_t)data_buf[i][j] % PAGE_SIZE, XFER_SIZE, DMA_BIDIRECTIONAL);
		}
	}

	for (example = 0; example < EXAMPLES; example++) {
		if (example != 0)
			pr_info("\n\n\n");
		pr_info("			Example-%02d\n", example);
		pr_info("======================================\n");

		for (j = 0; j < BUF_SIZE; j++) {
			for (i = 0; i < 2; i++)
				get_random_bytes(&data_buf[i][j][0], sizeof(uint64_t));
			pr_info("[data  ] before-%02d: src=%04x, dst=%04x\n", 
					j, (int)data_buf[0][j][0], (int)data_buf[1][j][0]);
			cflush((char*)data_buf[0][j], XFER_SIZE);
			cflush((char*)data_buf[1][j], XFER_SIZE);
		}
		pr_info("--------------------------------------\n");

		status = single(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
		//status = batch(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
		//status = async(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);

		pr_info("--------------------------------------\n");
		if (status != 1)
			pr_info("[verify] failed: %x\n", status);
		else
			pr_info("[verify] passed\n");
		
		for (j = 0; j < BUF_SIZE; j++) {
			if (comp_buf[j]->result)
				pr_info("[data  ] after-%02d: src=%04x, dst=%04x, result: diff(%d)\n",
						j, (int)data_buf[0][j][0], (int)data_buf[1][j][0], comp_buf[j]->bytes_completed);
			else
				pr_info("[data  ] after-%02d: src=%04x, dst=%04x, result: same\n",
						j, (int)data_buf[0][j][0], (int)data_buf[1][j][0]);
		}
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < BUF_SIZE; j++) {
			dma_unmap_page(dev, data_dma_buf[i][j], XFER_SIZE, DMA_BIDIRECTIONAL);
			kfree(data_buf[i][j]);
		}
	}
	for (j = 0; j < BUF_SIZE; j++)
		dma_free_coherent(dev, idxd->data->compl_size, comp_buf[j], comp_dma_buf[j]);
	kfree(desc_buf);
}

static int __init dsatest_init(void)
{
	struct file *filp;

	filp = filp_open(dsa_cdev_name, O_RDWR, 0);
	if (IS_ERR(filp)) {
		pr_err("Failed to open DSA device\n");
		return PTR_ERR(filp);
	}

	if (filp->f_op->unlocked_ioctl)
		wq = (struct idxd_wq*) filp->f_op->unlocked_ioctl(filp, IDXD_IOCAL_GET_WQ, 0);
	else {
		pr_info("No ioctl for dsa\n");
		return 0;
	}

	if (!wq) {
		pr_err("Failed to get wq\n");
		return -ENOMEM;
	}

	dsatest_main();

	filp_close(filp, NULL);
	return 0;
}
module_init(dsatest_init);

static void __exit dsatest_exit(void)
{
	pr_info("%s\n", __func__);
}
module_exit(dsatest_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Minho Kim");
