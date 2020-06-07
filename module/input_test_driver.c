#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <asm/irq.h>
#include <asm/io.h>

#define TEST_IRQ    0x1337
#define PWN         0x1337
#define NWP         0x7331
#define X_AXIS_MIN	0
#define X_AXIS_MAX	255
#define Y_AXIS_MAX	X_AXIS_MAX
#define Y_AXIS_MIN	X_AXIS_MIN

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("V4bel");

static int input_test_driver_open(struct inode *inode, struct file *file);
static int input_test_driver_release(struct inode *inode, struct file *file);
static ssize_t input_test_driver_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long input_test_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


struct file_operations input_test_driver_fops = {
    .open       = input_test_driver_open,
    .release    = input_test_driver_release,
    .write      = input_test_driver_write,
    .unlocked_ioctl = input_test_driver_ioctl,
};

static struct fp_struct {
    char dummy[392];
    asmlinkage int (*gift)(const char *, ...);
    int (*fp_report_ps)(char *, int);
    int (*fp_report_rl)(void);
};

static struct miscdevice input_test_driver_driver = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "input_test_driver",
    .fops = &input_test_driver_fops,
};


static struct input_dev *test_dev;
struct fp_struct *_fp = 0;
char *ptr = 0;
struct mutex test_mutex;


static int input_test_driver_open(struct inode *inode, struct file *file) {
    printk("input_test_driver open");

    mutex_init(&test_mutex);

    return 0;
}

static int input_test_driver_release(struct inode *inode, struct file *file) {
    printk("input_test_driver release");

    mutex_lock(&test_mutex);

    if(ptr) {
        kfree(ptr);
        ptr = 0;
    }

    if(_fp) {
        kfree(_fp);
        _fp = 0;
    }

    mutex_unlock(&test_mutex);

    return 0;
}

static ssize_t input_test_driver_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    char *result;
    size_t len;

    mutex_lock(&test_mutex);

    if(ptr) {
        kfree(ptr);
    }

    if(count<256) {
        len = count;
        count = 256;
    } else if(count>0x40000) {
        len = 416;
    } else {
        len = count;
    }

    if(result = (char *)kmalloc(count, GFP_ATOMIC))
        ptr = result;

    if (copy_from_user(ptr, buf, len) != 0) {
        mutex_unlock(&test_mutex);
        return -EFAULT;
    }

    mutex_unlock(&test_mutex);

    return 0;
}

static int report_touch_press(char *start, int len) {
    int i;

    if(!len) {
        printk("len error");
        return -1;
    }

    for(i=0; i<=len; i++) {
        input_report_key(test_dev, BTN_TOUCH, 1);
        input_report_abs(test_dev, ABS_X, *(char *)(start+i));
        input_report_abs(test_dev, ABS_Y, *(char *)(start+i));
        input_sync(test_dev);
    }

    return 0;
}

static int report_touch_release(void) {
    input_report_key(test_dev, BTN_TOUCH, 0);
    input_sync(test_dev);

    return 0;
}
    

static long input_test_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

    switch(cmd) {
        case 0x1337:
            printk("report_touch_press call");
            /*
            input_report_key(test_dev, BTN_TOUCH, 1);
            input_report_abs(test_dev, ABS_X, value_x);
            input_report_abs(test_dev, ABS_Y, value_y);
            input_sync(test_dev);
            */

            mutex_lock(&test_mutex);

            if(!_fp) {
                _fp = kzalloc(sizeof(struct fp_struct), GFP_ATOMIC);
                _fp->fp_report_ps = report_touch_press;
                _fp->fp_report_rl = report_touch_release;
                _fp->gift = printk;
                _fp->gift("_fp class allocate");
            }

            _fp->fp_report_ps(ptr, strlen(ptr));

            mutex_unlock(&test_mutex);

            break;
        case 0x7331:
            printk("report_touch_release call");
            /*
            input_report_key(test_dev, BTN_TOUCH, 0);
            input_sync(test_dev);
            */

            mutex_lock(&test_mutex);

            if(!_fp) {
                _fp = kzalloc(sizeof(struct fp_struct), GFP_ATOMIC);
                _fp->fp_report_ps = report_touch_press;
                _fp->fp_report_rl = report_touch_release;
                _fp->gift = printk;
                _fp->gift("_fp class allocate");
            }

            _fp->fp_report_rl();
            
            mutex_unlock(&test_mutex);

            break;
        default:

            break;
    }

    return 0;
}

/*
 * It's a simple MacGuffin that does nothing.
 */
static void pseudo_report_xy(void)
{
	int value_x = 0, value_y = 0;
	int touch_chk = 0;
	u8 cast;

	if (__raw_readb(PWN) & 0x07) {
		cast = __raw_readb(NWP);
		__raw_writeb(cast, NWP);
		value_y = inb(0x7331);

		cast = __raw_readb(NWP);
		__raw_writeb(cast, NWP);
		value_x = inb(0x1337);

		cast = __raw_readb(NWP);
		__raw_writeb(cast, NWP);

		touch_chk = __raw_readb(PWN) & 0x08;
	}

	if (touch_chk) {
		input_report_key(test_dev, BTN_TOUCH, 1);
		input_report_abs(test_dev, ABS_X, value_x);
		input_report_abs(test_dev, ABS_Y, value_y);
	} else {
		input_report_key(test_dev, BTN_TOUCH, 0);
	}

	input_sync(test_dev);
}

static irqreturn_t test_interrupt(int irq, void *dummy) {
    pseudo_report_xy();

    return IRQ_HANDLED;
}

static int input_test_driver_init(void) {
    int result;
    int err;

    result = misc_register(&input_test_driver_driver);
    if(result) {
        printk("misc_register(): Misc device register failed\n");
        return result;
    }

    if(!request_irq(TEST_IRQ, test_interrupt, 0, "test", NULL)) {
        printk("request_irq(): can't allocate irq\n");
        return -EBUSY;
    }

    test_dev = input_allocate_device();
    if (!test_dev) {
        printk(KERN_ERR "input_allocate_device(): Not enough memory\n");
        err = -ENOMEM;
        goto err_free_irq;
    }

    test_dev->name = "V4bel Touchscreen Test";
    test_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    test_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    input_set_abs_params(test_dev, ABS_X, X_AXIS_MIN, X_AXIS_MAX, 0, 0);
    input_set_abs_params(test_dev, ABS_Y, Y_AXIS_MIN, Y_AXIS_MAX, 0, 0);

    err = input_register_device(test_dev);
    if (err) {
        printk(KERN_ERR "input_register_device(): Failed to register device\n");
        goto err_free_dev;
    }

    return 0;

err_free_dev:
    input_free_device(test_dev);
err_free_irq:
    free_irq(TEST_IRQ, test_interrupt);
    return err;
}

static void input_test_driver_exit(void) {
    misc_deregister(&input_test_driver_driver);
    input_unregister_device(test_dev);
    free_irq(TEST_IRQ, test_interrupt);
}

module_init(input_test_driver_init);
module_exit(input_test_driver_exit);
