//passing_params.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>


int value, arr_value[4];
char *name;
int cb_value = 0;

module_param(value, int, S_IWUSR|S_IRUSR);
module_param(name, charp, S_IRUSR|S_IWUSR);
module_param_array(arr_value,int, NULL, S_IRUSR|S_IWUSR);

// module_param_cb를 위한 setter
int notify_param(const char *val, const struct kernel_param *kp){
        int res = param_set_int(val, kp);
        if(res == 0){
                printk(KERN_INFO "Call back function called...\n");
                printk(KERN_INFO "New value of cb_value = %d\n",cb_value);
                return 0;
        }
        return -1;
}

const struct kernel_param_ops my_param_ops = {
        .set = &notify_param, //위에 정의한 setter
        .get = &param_get_int, // standard getter
};

module_param_cb(cb_value, &my_param_ops, &cb_value, S_IRUGO|S_IWUSR);

static int __init my_module_init(void){
        int i;
        printk(KERN_INFO "===== Print Params =====\n");
        printk(KERN_INFO "value = %d \n", value);
        printk(KERN_INFO "cb_value = %d \n", cb_value);
        printk(KERN_INFO "name = %s\n", name);
        for(i = 0; i < sizeof(arr_value)/sizeof(int); i++){
                printk(KERN_INFO "arr_value[%d] = %d \n", i, arr_value[i]);
        }
        return 0;
}

static void __exit my_module_exit(void){
        printk(KERN_INFO "Kernel Module Removed ...\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Reakwon");
MODULE_DESCRIPTION("A simple parameter test module");
MODULE_VERSION("1:1.0");
