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

//Modification By OSM : Compare only one
//#define EXAMPLES		3
#define EXAMPLES		1

#define XFER_SIZE	 4096

//Mofification By OSM : Workload Size 4MB
//#define BATCH_SIZE		16
//#define BUF_SIZE		16
#define BATCH_SIZE		512
#define BUF_SIZE		512

#define UMWAIT_STATE_C0_2 0
#define UMWAIT_STATE_C0_1 1
#define MAX_COMP_RETRY 1000
#define UMWAIT_DELAY 100000

// Modification By OSM : Resolove Size error
struct dsa_completion_record *comp_buf[BUF_SIZE];
static uint64_t *data_buf[2][BUF_SIZE];
dma_addr_t data_dma_buf[2][BUF_SIZE], comp_dma_buf[BUF_SIZE];

// Modification By OSM : Module Parameter
int dsatest_run = 0;
int dsatest_dsa_on;

module_param(dsatest_dsa_on, int, S_IRUSR | S_IWUSR );

// Modification By OSM : kernel thread
static struct task_struct *kdsatestd_1;
static struct task_struct *kdsatestd_2;
static struct task_struct *kdsatestd_3;
static struct task_struct *kdsatestd_4;

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

// Modification By OSM : DSA mem_compare
int dsa_memcmp(dma_addr_t data_dma_buf[][BUF_SIZE], struct dsa_hw_desc *desc_buf,
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
		

	pr_info("[info  ] running dsa offload\n");
	pr_info("Run DSA MEMCMP\n");

	for (i = 0; i < BUF_SIZE; i++) {
		
		start = get_cycles();

		comp_buf[i]->status = 0;
		desc_buf[i].priv = 1;
		if (device_pasid_enabled(idxd))
			desc_buf[i].pasid = idxd->pasid;
		else
			desc_buf[i].pasid = 0;
		desc_buf[i].opcode = DSA_OPCODE_COMPARE;
		desc_buf[i].flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_CC ;
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

	status = 1;
	for (i = 0; i < BUF_SIZE; i++) {
		if (comp_buf[i]->status != 1) {
			status = comp_buf[i]->status;
			break;
		}
	}
	
	if( status != 1)
		pr_err("cache error\n");
	return status;
}

//Modification By OSM : software mem_compare
int sw_memcmp(uint64_t *data_buf[2][BUF_SIZE])
{
	int status = 0;
	int check = 1;
	pr_info("Run SW_MEMCMP\n");
	for(int i = 0; i < BUF_SIZE; i++)
	{
		status = (*data_buf[0][i] == *data_buf[1][i]);
		if(status != 1)
			check = status;
	}
	return check;
}

/*
static void dsatest_main(void)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	void __iomem *wq_portal = idxd_wq_portal_addr(wq);
	struct dsa_hw_desc *desc_buf;
	
	// Modification By OSM : Resolve Size Error
	//struct dsa_completion_record *comp_buf[BUF_SIZE];
	//uint64_t *data_buf[2][BUF_SIZE];
	//dma_addr_t data_dma_buf[2][BUF_SIZE], comp_dma_buf[BUF_SIZE];
	
	int status;
	int example, i, j;

	// Modificaiton By OSM : Check Cache Flush
	int cache_check;
	
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
		// Prepare Pages
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

		status = 0;
		
		// Modificaiton By OSM : Check Cache Flush
		cache_check = 1;

		while(run) {
			// Modification By OSM : Distribute dsa
			if(dsa_on){
				// Modification By OSM : Cache Flush
				if(!cache_check){
					for (j = 0; j < BUF_SIZE; j++) {
						cflush((char*)data_buf[0][j], XFER_SIZE);
						cflush((char*)data_buf[1][j], XFER_SIZE);
					}
				}
				
				// Modification By OSM : mem_compare on dsa
				status = dsa_memcmp(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
				//status = single(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
				//status = batch(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
				//status = async(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);

				pr_info("--------------------------------------\n");
		
				// Find Error
				if (status != 1)
					pr_info("[verify] failed: %x\n", status);
				else
					pr_info("[verify] passed\n");
		
				// Get Result
				for (j = 0; j < BUF_SIZE; j++) {
					if (comp_buf[j]->result)
						pr_info("[data  ] after-%02d: src=%04x, dst=%04x, result: diff(%d)\n",
							j, (int)data_buf[0][j][0], (int)data_buf[1][j][0], comp_buf[j]->bytes_completed);
					else
						pr_info("[data  ] after-%02d: src=%04x, dst=%04x, result: same\n",
							j, (int)data_buf[0][j][0], (int)data_buf[1][j][0]);
				}
			}
			else{
				// Modification By OSM : mem_compare on software
				status = sw_memcmp(data_buf);
				cache_check = 0;
				pr_err("sw memcmp passed \n");
			}
		}
	}

	// Free Pages
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
*/

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

	pr_err("Run memcmp \n");
	//Modificatio By OSM
	//dsatest_main();
	
	filp_close(filp, NULL);
	pr_err("End memcmp\n");
	return 0;
}
module_init(dsatest_init);

static void __exit dsatest_exit(void)
{
	pr_info("%s\n", __func__);
}
module_exit(dsatest_exit);

/*
 * Modification By OSM for using kthread
 */

static int set_workload(void)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	
	int i, j;

	pr_info("Make 4KB Workload");
	for (j = 0; j < BUF_SIZE; j++)
		comp_buf[j] = dma_alloc_coherent(dev, idxd->data->compl_size, &comp_dma_buf[j], GFP_KERNEL);
	
	for (i = 0; i < 2; i++) {
		for (j = 0; j < BUF_SIZE; j++) {
			data_buf[i][j] = (uint64_t *)kmalloc(XFER_SIZE, GFP_KERNEL);
			data_dma_buf[i][j] = dma_map_page(dev, virt_to_page(data_buf[i][j]),
									(size_t)data_buf[i][j] % PAGE_SIZE, XFER_SIZE, DMA_BIDIRECTIONAL);
		}
	}

	for (j = 0; j < BUF_SIZE; j++) {
		for (i = 0; i < 2; i++)
			get_random_bytes(&data_buf[i][j][0], sizeof(uint64_t));
		cflush((char*)data_buf[0][j], XFER_SIZE);
		cflush((char*)data_buf[1][j], XFER_SIZE);
	}
	pr_info("End 4KB Workload");
	return 0;
}

static int kdsatestd_create(void *data)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	void __iomem *wq_portal = idxd_wq_portal_addr(wq);
	struct dsa_hw_desc *desc_buf;
	int status;

	pr_info("Create kdsatestd");
	desc_buf = kzalloc_node(BUF_SIZE * sizeof(*desc_buf), GFP_KERNEL, dev_to_node(dev));
	status = 0;
	
	ssleep(1);

	while(!kthread_should_stop()) {
		if(dsatest_dsa_on)
			status = dsa_memcmp(data_dma_buf, desc_buf, comp_buf, comp_dma_buf, wq_portal, idxd);
		else
			status = sw_memcmp(data_buf);
		ssleep(0.01);
	}
	
	pr_info("Delete kdsatestd");
	return status;
}

int run_set(const char *val, const struct kernel_param *kp)
{
	int res;

	res = param_set_int(val, kp);
	pr_info("Change dsatest_run[%d]", dsatest_run);

	if(res == 0){	
		if(dsatest_run == 1){
			set_workload();
			pr_info("Success set_workload");

			kdsatestd_1 = kthread_create(kdsatestd_create, NULL, "kdsatestd_1");
			kdsatestd_2 = kthread_create(kdsatestd_create, NULL, "kdsatestd_2");
			kdsatestd_3 = kthread_create(kdsatestd_create, NULL, "kdsatestd_3");
			kdsatestd_4 = kthread_create(kdsatestd_create, NULL, "kdsatestd_4");
			
			pr_info("Success kthred_create");

			kthread_bind(kdsatestd_1, 0);
			kthread_bind(kdsatestd_2, 1);
			kthread_bind(kdsatestd_3, 2);
			kthread_bind(kdsatestd_4, 3);
			pr_info("Success kthred_bind");

			wake_up_process(kdsatestd_1);
			wake_up_process(kdsatestd_2);
			wake_up_process(kdsatestd_3);
			wake_up_process(kdsatestd_4);
			pr_info("Success kthred_wake_up");
		}
		else{
			kthread_stop(kdsatestd_1);
			kthread_stop(kdsatestd_2);
			kthread_stop(kdsatestd_3);
			kthread_stop(kdsatestd_4);
			
			pr_info("Success kthred_stop");
		}
	}
	return res;
}

const struct kernel_param_ops param_ops_run = {
	.set = &run_set,
	.get = &param_get_int,
};

module_param_cb(dsatest_run, &param_ops_run, &dsatest_run, S_IRUSR | S_IWUSR );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Minho Kim");
