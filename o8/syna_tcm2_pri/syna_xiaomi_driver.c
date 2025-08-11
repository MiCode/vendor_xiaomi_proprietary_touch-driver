#include <linux/soc/qcom/panel_event_notifier.h>
#include <linux/spi/spi.h>
#include "syna_tcm2.h"
#include "syna_xiaomi_driver.h"
#include "synaptics_touchcom_func_base.h"
#include "synaptics_touchcom_func_base_flash.h"
#include "synaptics_touchcom_core_dev.h"
#include "synaptics_touchcom_func_reflash.h"
#include "../../xiaomi/xiaomi_touch.h"

#define SYNAPTICS_DRIVER_VERSION "synaptics:2023.07.28-001-v2"
#define RETRY_CNT 10

bool coord_log_flag = false;

static xiaomi_bdata_t xiaomi_bdata;
static xiaomi_driver_data_t xiaomi_driver_data;
static hardware_operation_t hardware_operation;
static hardware_param_t hardware_param;
static struct syna_tcm *tcm = NULL;
extern struct qcom_smem_state *state;

static int syna_ic_data_collect(char *buf, int *length)
{
	int ret = 0, cnt1 = 0, cnt2 = 0;
	if (!tcm){
		LOGE("get tcm failed\n");
		goto err_out;
	}

	if (tcm->pwr_state != PWR_ON){
		LOGE("pwr_state !=  PWR_ON\n");
		goto err_out;
	}

  	if (!tcm->is_connected) {
  		LOGE("Not connected\n");
  		goto err_out;
  	}

	cnt1 = syna_xiaomi_report_data(REPORT_DELTA, buf); /* REPORT_DELTA = 0x12 */
	LOGD("cnt1 = %d\n", cnt1);
	if (cnt1 <= 0) {
		LOGE( "REPORT_DELTA failed, cnt1=%d\n", cnt1);
		ret = cnt1;
		goto err_out;
	}

	cnt2 = syna_xiaomi_report_data(REPORT_RID161, buf + cnt1); /* REPORT_RID161 = 0xA1 */
	LOGD("cnt2 = %d\n", cnt2);
	if (cnt2 <= 0){
		LOGE("REPORT_RID161 failed, cnt2=%d\n", cnt2);
		ret = cnt2;
		goto err_out;
	}
	*length = cnt1 + cnt2;
	LOGI("length = %d\n", *length);

err_out:
	return ret;
}

static int syna_ic_self_test(char *type, int *result)
{
	int retval = 0;
	char *self_test_data = NULL;
	struct syna_tcm *tcm_hcd = tcm;

	if (!tcm_hcd)
		return 0;

	if (tcm_hcd->pwr_state != PWR_ON) {
		LOGI("in suspend mode,can not to do\n");
		goto out;
	}

	if (!strncmp("short", type, 5) || !strncmp("open", type, 4)) {
		if (!tcm_hcd && !tcm_hcd->testing_xiaomi_self_test) {
			LOGE("NULL Pointer!\n");
			retval = 1;
			goto out;
		}

		self_test_data = vmalloc(PAGE_SIZE * 3);
		if (self_test_data == NULL) {
			retval = 1;
			goto out;
		}

		retval = tcm_hcd->testing_xiaomi_self_test(self_test_data);
		if (!retval) {
			LOGE("[DIS-TF-TOUCH] self test failed, retval = %d\n", retval);
			retval = 1;
			goto out;
		}
		retval = 2;
	} else if (!strncmp("i2c", type, 3)) {
		if (!tcm_hcd->testing_xiaomi_chip_id_read) {
			retval = 1;
			goto out;
		}

		retval = tcm_hcd->testing_xiaomi_chip_id_read(tcm_hcd);
		/* (retval == 2) passed  (retval == 1) failed */
	}
out:
	*result = retval;
	if (self_test_data) {
		vfree(self_test_data);
		self_test_data = NULL;
	}

	return 0;
}

static int syna_tcm_lockdown_info(void)
{
	int retval = 0;
	int i = 0;
	unsigned char *p = NULL;
	struct tcm_buffer rd_data;
	LOGI("Get syna_tcm_lockdown_info\n");
	if (!tcm) {
		LOGE("tcm is NULL, please check it\n");
		return -1;
	}

	if (xiaomi_driver_data.lockdown_info_ready) {
		LOGI("lockdown info is ready, not read again from flash\n");
		goto exit;
	}

	for (i = 0; i < RETRY_CNT; ++i) {
		retval = syna_tcm_read_flash_area(tcm->tcm_dev, AREA_BOOT_CONFIG, &rd_data, DEFAULT_FLASH_READ_DELAY);
		if (retval < 0) {
			LOGE("Failed to read oem_info data\n");
			goto exit;
		}

		for (i = (BOOT_CONFIG_SLOTS - 1); i >= 0; i--) {
			if (rd_data.buf[i * BOOT_CONFIG_SIZE + 0] != 0) {
				p = &rd_data.buf[i * BOOT_CONFIG_SIZE + 0];
				break;
			}
		}

		if (!p) {
			LOG_ERROR("p pointer is NULL, get rd_data buf is error!!\n");
			continue;
		}

		for (i = 0; i < 8; i++) {
			xiaomi_driver_data.lockdown_info[i] = p[i];
			LOGI("p[%d] = 0x%02x, PAGE_SIZE = %lu\n", i , xiaomi_driver_data.lockdown_info[i], PAGE_SIZE);
		}

		xiaomi_driver_data.lockdown_info_ready = true;
		retval = 0;
		break;
	}

exit:
	return retval;
}

static int syna_tcm_lockdown_info_read(u8 lockdown_info[8])
{
	int ret = syna_tcm_lockdown_info();
	if (ret < 0)
		return ret;
	lockdown_info[0] = xiaomi_driver_data.lockdown_info[0];
	lockdown_info[1] = xiaomi_driver_data.lockdown_info[1];
	lockdown_info[2] = xiaomi_driver_data.lockdown_info[2];
	lockdown_info[3] = xiaomi_driver_data.lockdown_info[3];
	lockdown_info[4] = xiaomi_driver_data.lockdown_info[4];
	lockdown_info[5] = xiaomi_driver_data.lockdown_info[5];
	lockdown_info[6] = xiaomi_driver_data.lockdown_info[6];
	lockdown_info[7] = xiaomi_driver_data.lockdown_info[7];
	return 0;
}

static int syna_tcm_fw_version_read(char firmware_version[64])
{
	int cnt = 0;
	int length = 64;
	struct syna_tcm *tcm_hcd = tcm;

	if (!tcm_hcd) {
		LOG_ERROR("tcm_hcd is null!!,return\n");
		return -EINVAL;
	}

	if (tcm_hcd->pwr_state != PWR_ON) {
		LOGE("is power off\n");
		return 0;
	}

	cnt = snprintf(firmware_version, length, "Firmware: %d Cfg: %02x %02x",
			tcm_hcd->tcm_dev->packrat_number,
			tcm_hcd->tcm_dev->app_info.customer_config_id[6] - 48,
			tcm_hcd->tcm_dev->app_info.customer_config_id[7] - 48);
	return cnt;
}

static u8 syna_tcm_panel_vendor_read(void)
{
	if (syna_tcm_lockdown_info() < 0)
		return 0;
	return xiaomi_driver_data.lockdown_info[0];
}

static u8 syna_tcm_panel_color_read(void)
{
	if (syna_tcm_lockdown_info() < 0)
		return 0;
	return xiaomi_driver_data.lockdown_info[2];
}

static u8 syna_tcm_panel_display_read(void)
{
	if (syna_tcm_lockdown_info() < 0)
		return 0;
	return xiaomi_driver_data.lockdown_info[1];
}

static char syna_tcm_touch_vendor_read(void)
{
	return '5';
}

int syna_tcm_set_gesture_type(struct syna_tcm *tcm, u8 val)
{
	int retval = 0;

	/*set gesture type*/
	if (tcm->hw_if->ops_enable_irq)
		tcm->hw_if->ops_enable_irq(tcm->hw_if, true);

	if ((val & GESTURE_SINGLETAP_EVENT))
		tcm->gesture_type |= (0x0001 << 13);
	else
		tcm->gesture_type &= ~(0x0001 << 13);
	if ((val & GESTURE_DOUBLETAP_EVENT))
		tcm->gesture_type |= 0x0001;
	else
		tcm->gesture_type &= ~0x0001;

	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_GESTURE_TYPE_ENABLE, tcm->gesture_type, RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Failed to enable gesture type\n");
		return retval;
	} else {
		LOGI("set gesture type:%x\n", tcm->gesture_type);
	}
	return retval;
}

static void syna_tcm_set_mode(u8 gesture_type)
{
	int retval;
	struct syna_hw_interface *hw_if;

	LOGI("enter\n");
	
	hw_if = tcm->hw_if;
	if (gesture_type == 0) {
		retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
				DC_ENABLE_WAKEUP_GESTURE_MODE, 3, RESP_IN_ATTN);
		if (retval < 0) {
			LOGE("Failed to disable wakeup gesture mode\n");
			return;
		} else {
			LOGI("clear DC_ENABLE_WAKEUP_GESTURE_MODE\n");
		}
	} else {
		retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
				DC_ENABLE_WAKEUP_GESTURE_MODE, 1, RESP_IN_ATTN);
		if (retval < 0) {
			LOGE("Failed to enable wakeup gesture mode\n");
			return;
		} else {
			LOGI("enter DC_ENABLE_WAKEUP_GESTURE_MODE\n");
			tcm->pwr_state = LOW_PWR;
		}
	}
}

static void syna_tcm_switch_mode(u8 gesture_type)
{
	int retval, i = 0, retry = 3;
	LOGI("enter\n");
#ifdef CONFIG_TOUCH_FACTORY_BUILD
	LOGI("factory version,skip set gesture mode\n");
	return;
#endif
	if (tcm == NULL) {
		LOGE("tcm is null\n");
		return;
	}
	if (gesture_type < 0) {
		LOGE("get geture type error\n");
		return;
	}

	for (i = 0; i < retry; i++) {
		retval = syna_tcm_set_gesture_type(tcm, gesture_type);
		if (retval < 0) {
			LOGE("set gesture type error, retry\n");
			msleep(10);
		} else {
			LOGI("set gesture type ok\n");
			break;
		}
	}

	if (tcm->pwr_state == PWR_ON) {
		LOGI("in resume mode,don't need to set gesture mode\n");
		return;
	}
	
	syna_tcm_set_mode(gesture_type);
}

static void syna_tcm_set_charge_state(int state)
{
	int retval = 0;
	unsigned short val = 0;

	if (tcm == NULL) {
		LOGE("tcm is null\n");
		return;
	}
	if (tcm->pwr_state != PWR_ON) {
		LOGI("in suspend mode,don't need to set charge state\n");
		return;
	}
	if (state) {
		val = (state == 3) ? 0x01 : 0x02;//0x01 for Wired Charging, 0x02 for Wireless Charging
	} else {
		val = 0x00;
	}
	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_ENABLE_CHARGER_CONNECTED, val, RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Failed to set charge mode\n");
	} else {
		LOGI("set charger_connected, value = 0x%02x\n", val);
	}
}

static int syna_tcm_touch_log_level_control(bool flag)
{
	coord_log_flag = flag;
	return 0;
}

int syna_set_thermal_temp(int temp, bool force)
{
	int retval = 0;
	int temp0 = 0;
	unsigned char payload[2]={0,0};

	if (tcm->pwr_state != PWR_ON) {
		LOGI("%s pwr_state is not PWR_ON\n", __func__);
		return -1;
	}

	if (force) {
		temp0 = get_bms_temp_common();
		if (abs(temp0) >= INVAILD_TEMPERATURE)
			return -1;

		temp = (temp0 + 5) / 10; // Rounding, in degrees Celsius
	}
	LOGI("temp: %d", temp);
	payload[0] = 0x5e; //thermal command
	payload[1] = temp;

	retval = tcm->tcm_dev->write_message_without_response(tcm->tcm_dev,0xc8,payload,sizeof(payload));
	if (retval < 0) {
		LOGE("Fail to set ic thermal\n");
		goto exit;
	}
exit:
	return retval;
}

static int syna_ic_resume_suspend(bool is_resume, u8 gesture_type)
{
	if (tcm == NULL)
		return -1;
	if (is_resume) {
		LOGI("syna_ic_resume\n");
		return tcm->dev_resume(&tcm->pdev->dev, gesture_type);
	}
	LOGI("syna_ic_suspend\n");
	return tcm->dev_suspend(&tcm->pdev->dev, gesture_type);
}

static void syna_get_config(void)
{
	xiaomi_bdata.test_limit_name = xiaomi_bdata.synaptics_default_limit_name;
	LOGI("limit_name:%s\n", xiaomi_bdata.test_limit_name);
}

int xiaomi_parse_dt(struct device *dev)
{
	int retval;
	struct device_node *np = dev->of_node;

	retval = of_property_read_string(np, "synaptics,default-test-limit-name",
					 &xiaomi_bdata.synaptics_default_limit_name);
	if (retval && (retval != -EINVAL)) {
		xiaomi_bdata.synaptics_default_limit_name = NULL;
		LOGE("Unable to read default limit name\n");
	} else {
		LOGI("default limit_name:%s\n", xiaomi_bdata.synaptics_default_limit_name);
	}

	LOGI("complete\n");

	return 0;
}

const char *xiaomi_get_test_limit_name(void)
{
	LOGI("limit image name %s\n", xiaomi_bdata.test_limit_name);
	return xiaomi_bdata.test_limit_name;
}

static int syna_tcm_touch_doze_analysis(int input)
{
	int result = 0;
	struct syna_tcm *tcm_hcd = tcm;
	struct syna_hw_attn_data *attn = &tcm->hw_if->bdata_attn;

	LOGI("input: %d", input);
	if (tcm == NULL) {
		LOGE("tcm is null");
		return -1;
	}
	if (tcm_hcd->pwr_state != PWR_ON) {
        LOGI(" touch has suspend,return\n");
  		return result;
    }

	switch(input) {
		case TOUCH_0:
			tcm_hcd->doze_test = true;
			schedule_resume_suspend_work(PRI_TOUCH_ID, false);
			schedule_resume_suspend_work(PRI_TOUCH_ID, true);
			tcm_hcd->doze_test = false;
		break;
		case TOUCH_1:
			syna_dev_reflash_startup(tcm, true);
		break;
		case TOUCH_2:
			enable_irq(attn->irq_id);
			attn->irq_enabled = true;
		break;
		case TOUCH_3:
			disable_irq_nosync(attn->irq_id);
			attn->irq_enabled = false;
		break;
		case TOUCH_4:
			/* release irq */
			if (tcm_hcd->hw_if->bdata_attn.irq_id)
				tcm->dev_release_irq(tcm_hcd);
			/* register the interrupt handler */
			result = tcm->dev_request_irq(tcm_hcd);
			if (result < 0) {
				LOGE("Fail to request the interrupt line\n");
			} else {
				result = tcm_hcd->hw_if->ops_enable_irq(tcm_hcd->hw_if, true);
				if (result < 0)
					LOGE("Failed to enable interrupt\n");
			}
		break;
		case TOUCH_5:
			result = gpio_get_value(attn->irq_gpio) == 0 ? 0 : 1;
		break;
		default:
			LOGE("Don't support touch doze analysis\n");
			break;
	}
	return result;
}

#ifdef SYNAPTICS_DEBUGFS_ENABLE
static void syna_tcm_dbg_suspend(struct syna_tcm *tcm_hcd, bool enable)
{
	if (enable) {
		schedule_resume_suspend_work(PRI_TOUCH_ID, false);
	} else {
		schedule_resume_suspend_work(PRI_TOUCH_ID, true);
	}
}

static int syna_tcm_dbg_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t syna_tcm_dbg_read(struct file *file, char __user *buf, size_t size,
				loff_t *ppos)
{
	const char *str = "cmd support as below:\n \
	\necho \"irq-disable\" or \"irq-enable\" to ctrl irq\n \
	\necho \"tp-suspend-en\" or \"tp-suspend-off\" to ctrl panel in or off suspend status\n \
	\necho \"tp-sd-en\" or \"tp-sd-off\" to ctrl panel in or off sleep status\n";

	loff_t pos = *ppos;
	int len = strlen(str);

	if (pos < 0)
		return -EINVAL;
	if (pos >= len)
		return 0;

	if (copy_to_user(buf, str, len))
		return -EFAULT;

	*ppos = pos + len;

	return len;
}

static ssize_t syna_tcm_dbg_write(struct file *file, const char __user *buf,
					size_t size, loff_t *ppos)
{
	char *cmd = kzalloc(size + 1, GFP_KERNEL);
	int ret = size;
	struct syna_tcm *tcm_hcd = tcm;

	if (!cmd)
		return -ENOMEM;

	if (copy_from_user(cmd, buf, size)) {
		ret = -EFAULT;
		goto out;
	}

	cmd[size] = '\0';

	if (!strncmp(cmd, "irq-disable", 11)) {
		LOGI("touch irq is disabled!\n");
		tcm_hcd->hw_if->ops_enable_irq(tcm_hcd->hw_if,false);
	} else if (!strncmp(cmd, "irq-enable", 10)) {
		LOGI("touch irq is enabled!\n");
		tcm_hcd->hw_if->ops_enable_irq(tcm_hcd->hw_if,true);
	} else if (!strncmp(cmd, "tp-sd-en", 8)) {
		tcm_hcd->doze_test = true;
		schedule_resume_suspend_work(PRI_TOUCH_ID, false);
		tcm_hcd->doze_test = false;
	} else if (!strncmp(cmd, "tp-sd-off", 9)) {
		tcm_hcd->doze_test = true;
		schedule_resume_suspend_work(PRI_TOUCH_ID, true);
		tcm_hcd->doze_test = false;
	} else if (!strncmp(cmd, "tp-suspend-en", 13))
		syna_tcm_dbg_suspend(tcm_hcd, true);
	else if (!strncmp(cmd, "tp-suspend-off", 14))
		syna_tcm_dbg_suspend(tcm_hcd, false);
out:
	kfree(cmd);

	return ret;
}

static int syna_tcm_dbg_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static const struct file_operations tpdbg_operations = {
	.owner = THIS_MODULE,
	.open = syna_tcm_dbg_open,
	.read = syna_tcm_dbg_read,
	.write = syna_tcm_dbg_write,
	.release = syna_tcm_dbg_release,
};
#endif

int syna_tcm_palm_area_change_setting(int value)
{
	int retval = 0;

	if (tcm == NULL) {
		LOGE("tcm is null");
		return -1;
	}

	LOGI("palm area change setting, 0x%02x \n", value);

	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
		DC_PALM_AREA_CHANGE, value,RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Failed to palm area change setting, retval=%d\n", retval);
		goto exit;
	}

exit:
	return retval;
}

static int syna_palm_sensor_write(int value)
{
	if (tcm == NULL) {
		LOGE("tcm is null");
		return -1;
	}

  	if (!tcm->is_connected) {
  		LOGE("Not connected\n");
  		return -1;
  	}
	LOGI("syna_palm_sensor_write :%d\n", value);
	tcm->palm_sensor_enable = value;
	if (tcm->pwr_state != PWR_ON) {
		LOGI("in suspend state,not need");
		return 0;
	}

	syna_tcm_palm_area_change_setting(!!value);

	return 0;
}

bool pri_touch_is_resume(void)
{
	if (tcm == NULL) {
		LOGE("tcm is null");
		return false;
	}
	if (tcm->pwr_state == PWR_ON) {
		return true;
	} else
		return false;
}
EXPORT_SYMBOL_GPL(pri_touch_is_resume);

static void syna_set_mode_value(int mode, int *value)
{
	int retval;
	if (!tcm) {
		LOGE("tcm is null!!,return\n");
		return;
	}

	if (tcm->pwr_state != PWR_ON) {
		LOGI("pwr_state not pwr on,return\n");
		return;
	}
  	if (!tcm->is_connected) {
  		LOGE("Not connected\n");
  		return;
  	}
	if (*value < 0) {
		LOGE("error value [%d]\n", *value);
		return;
	}
	LOGI("mode %d,value %d", mode, *value);
	switch(mode) {
  		case DATA_MODE_48:
			retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_SPEED_TOUCH_MODE, *value, RESP_IN_ATTN);
			if (retval < 0) {
				LOGE("set dynamic config id:DC_SPEED_TOUCH_MODE failed, retval=%d\n", retval);
			}
  			break;
  		case DATA_MODE_49:
  			break;
  		case DATA_MODE_50:
  			break;
		case DATA_MODE_62:
			if (*value)
				retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_TAP_JITTER, 04, RESP_IN_ATTN);
			else
				retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_TAP_JITTER, 03, RESP_IN_ATTN);
			break;
  		case DATA_MODE_52:
			retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_SUPPER_JITTER_FILTER, *value, RESP_IN_ATTN);
			if (retval < 0) {
				LOGE("set dynamic config id:DC_SUPPER_JITTER_FILTER failed, retval=%d\n", retval);
			}
  			break;
  		case DATA_MODE_51: {
			bool gamemode_on = driver_get_touch_mode(PRI_TOUCH_ID, DATA_MODE_0);
			if (gamemode_on) {
				// Game mode is enabled
				if (*value == 0 || *value == 1) {
					tcm->current_super_resolution = 8;
				} else if (*value == 2 || *value == 3) {
					tcm->current_super_resolution = 16;
				} 
			} else {
				// Game mode is not enabled
				tcm->current_super_resolution = 1;
			}
		}
  			break;
  		default:
  			LOGE("not support mode!");
  			break;
	}
}

enum cmd_c7_sub_cmd {
	C7_SUB_CMD_SET_GRIP_ZONE = 0x00,
	C7_SUB_CMD_GET_GRIP_ZONE = 0x01,
	C7_SUB_CMD_SET_2_5D_GRIP_ZONE = 0x02
};

#define GRIP_RECT_NUM 12
#define GRIP_PARAMETER_NUM 8

static void syna_set_edge_filter_value(int val)
{
	int i,retval;
  	unsigned int length_out;
  	unsigned char *out_buf;
	unsigned short checksum = 0;
	unsigned char resp_code;
	bool gamemode_on;
	int direction;
	struct syna_grip_zone *grip_zone = NULL;

	if (!tcm) {
		LOGE("tcm is null!!,return\n");
		return;
	}

	if (tcm->pwr_state != PWR_ON) {
		LOGI("pwr_state not pwr on,return\n");
		return;
	}
  	if (!tcm->is_connected) {
  		LOGE("Not connected\n");
  		return;
  	}

	length_out = 1 + sizeof(struct syna_grip_zone) + 1;
  	out_buf = (unsigned char *)kzalloc(length_out, GFP_KERNEL);
  	if (!out_buf) {
  		LOGE("Failed to allocate memory for out_buf\n");
  		goto exit;
  	}

  	out_buf[0] = C7_SUB_CMD_SET_2_5D_GRIP_ZONE;
	gamemode_on = driver_get_touch_mode(PRI_TOUCH_ID, DATA_MODE_0);
	direction = driver_get_touch_mode(PRI_TOUCH_ID, DATA_MODE_8);
	grip_zone = (struct syna_grip_zone*)&out_buf[1];
	grip_zone->type = (gamemode_on ? (1 << 4) : 0) | (direction & 0x0F);
	grip_zone->corner_level = val;

  	for (i = 1; i < (length_out - 1); i++) {
  		checksum += out_buf[i];
  	}
	out_buf[length_out - 1] = (unsigned char)((checksum & 0xFF) + ((checksum>>8) & 0xFF));

  	retval = tcm->tcm_dev->write_message(tcm->tcm_dev,
  			CMD_MultiFunction,
  			out_buf,
  			length_out,
  			length_out,
  			&resp_code,
  			tcm->tcm_dev->msg_data.default_resp_reading);
  	if (retval < 0) {
  		LOGE("Failed to write command 0x%x to get grip setting, retval=%d\n", CMD_MultiFunction, retval);
		goto exit;
  	}

	exit:
		kfree(out_buf);
}

bool syna_get_wakeup_enable(void)
{
	return !!driver_get_touch_mode(PRI_TOUCH_ID, DATA_MODE_30);
}

static void syna_cmd_mode_update(long mode_update_flag, int mode_value[DATA_MODE_35])
{
	int retval;

	if (!tcm) {
		LOGE("tcm is null!!,return\n");
		return;
	}

	if (tcm->pwr_state != PWR_ON) {
		LOGI("pwr_state not pwr on,return\n");
		return;
	}
  	if (!tcm->is_connected) {
  		LOGE("Not connected\n");
  		return;
  	}
	LOGI("mode_update_flag 0x%02lX ", mode_update_flag);

	if (mode_update_flag & (1 << DATA_MODE_3)) {
		LOGI("DATA_MODE_3 %d", mode_value[DATA_MODE_3]);
		retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_SWIPE_IIRFILTER, mode_value[DATA_MODE_3], RESP_IN_ATTN);
		if (retval < 0) {
			LOGE("set dynamic config id:DATA_MODE_3 failed, retval=%d\n", retval);
		}
	}

	if (mode_update_flag & (1 << DATA_MODE_5)) {
		LOGI("DATA_MODE_5 %d", mode_value[DATA_MODE_5]);
		retval = syna_tcm_set_dynamic_config(tcm->tcm_dev, DC_TAP_JITTER, mode_value[DATA_MODE_5], RESP_IN_ATTN);
		if (retval < 0) {
			LOGE("set dynamic config id:DATA_MODE_5 failed, retval=%d\n", retval);
		}
	}

	if (mode_update_flag & ((1 << DATA_MODE_7)|(1 << DATA_MODE_8)|(1 << DATA_MODE_0))) {
		LOGI("DATA_MODE_7 %d DATA_MODE_8:%d DATA_MODE_0:%d", 
		mode_value[DATA_MODE_7],mode_value[DATA_MODE_8],mode_value[DATA_MODE_0]);
		syna_set_edge_filter_value(mode_value[DATA_MODE_7]);
	}
}


int get_capfold_information(void){
	int retval;
	unsigned short capfoldStaus;
	retval = syna_tcm_get_capfold_status(tcm->tcm_dev, &capfoldStaus, 0);
	if (retval < 0) {
		LOGE("get capfold status failed, retval:%d\n", retval);
	} else {
		LOGI("tcm->tcm_dev->isOpen  = %d  capfoldStaus = %d \n", tcm->tcm_dev->isOpen, capfoldStaus);
		if (tcm->tcm_dev->isOpen != capfoldStaus) {
			LOGI("capfoldStaus = %d \n", capfoldStaus);
			if (capfoldStaus == 1)
				qcom_smem_state_update_bits(state, AWAKE_BIT, 0);
			else
				qcom_smem_state_update_bits(state, AWAKE_BIT, AWAKE_BIT);
		}
	}
	return retval;
}
EXPORT_SYMBOL_GPL(get_capfold_information);

void xiaomi_touch_probe(struct syna_tcm *syna_tcm)
{
	struct spi_device *spi = NULL;

	if (syna_tcm == NULL)
		return;

	syna_get_config();
	tcm = syna_tcm;
	spi = tcm->hw_if->pdev;

  	hardware_param.x_resolution = 1224;
  	hardware_param.y_resolution = 2912;
	hardware_param.super_resolution_factor = 16;
  	memset(hardware_param.driver_version, 0, 64);
  	memset(hardware_param.fw_version, 0, 64);
  	memcpy(hardware_param.driver_version, SYNAPTICS_DRIVER_VERSION, strlen(SYNAPTICS_DRIVER_VERSION));
  	syna_tcm_fw_version_read(hardware_param.fw_version);

	memset(&hardware_operation, 0, sizeof(hardware_operation_t));
	hardware_operation.ic_resume_suspend = syna_ic_resume_suspend;
	hardware_operation.ic_self_test = syna_ic_self_test;
	hardware_operation.ic_data_collect = syna_ic_data_collect;
	hardware_operation.ic_get_lockdown_info = syna_tcm_lockdown_info_read;
	hardware_operation.ic_get_fw_version = syna_tcm_fw_version_read;
	hardware_operation.set_mode_value = syna_set_mode_value;
	hardware_operation.set_mode_long_value = NULL;
  	hardware_operation.cmd_update_func = syna_cmd_mode_update;
	hardware_operation.palm_sensor_write = syna_palm_sensor_write;

	hardware_operation.panel_vendor_read = syna_tcm_panel_vendor_read;
	hardware_operation.panel_color_read = syna_tcm_panel_color_read;
	hardware_operation.panel_display_read = syna_tcm_panel_display_read;
	hardware_operation.touch_vendor_read = syna_tcm_touch_vendor_read;
	hardware_operation.get_touch_ic_buffer = NULL;
	hardware_operation.touch_doze_analysis = syna_tcm_touch_doze_analysis;
	hardware_operation.touch_log_level_control = syna_tcm_touch_log_level_control;
	hardware_operation.ic_switch_mode = syna_tcm_switch_mode;
	hardware_operation.ic_set_charge_state = syna_tcm_set_charge_state;
	hardware_operation.set_thermal_temp = syna_set_thermal_temp;
	hardware_param.temp_change_value = 2;

	register_touch_panel(&spi->dev, PRI_TOUCH_ID, &hardware_param, &hardware_operation);
	xiaomi_register_panel_notifier(&spi->dev, PRI_TOUCH_ID,
		PANEL_EVENT_NOTIFICATION_PRIMARY, PANEL_EVENT_NOTIFIER_CLIENT_PRIMARY_TOUCH);

#ifdef SYNAPTICS_DEBUGFS_ENABLE
	tcm->debugfs = debugfs_create_dir("tp_debug_pri", NULL);
	if (tcm->debugfs) {
		debugfs_create_file("switch_state", 0660, tcm->debugfs, tcm,
					&tpdbg_operations);
	}
#endif
}

void syna_xiaomi_touch_remove(struct syna_tcm *syna_tcm)
{
struct spi_device *spi_dev = syna_tcm->hw_if->pdev;

	if (syna_tcm == NULL) {
		LOGE("syna_tcm null,return");
		return;
	}
	xiaomi_unregister_panel_notifier(&spi_dev->dev, PRI_TOUCH_ID);
	unregister_touch_panel(PRI_TOUCH_ID);
}