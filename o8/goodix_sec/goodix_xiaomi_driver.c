#include <linux/soc/qcom/panel_event_notifier.h>
#include "goodix_xiaomi_driver.h"
#include "../../xiaomi/xiaomi_touch.h"
#include "goodix_ts_core.h"
#include <linux/timer.h>
static hardware_operation_t hardware_operation;
static hardware_param_t hardware_param;
static struct goodix_ts_core *ts_core = NULL;
struct timer_list my_timer;
static int goodix_short_open_test(void)
{
	struct ts_rawdata_info *info = NULL;
	int test_result;

	info = vzalloc(sizeof(*info));
	if (!info) {
		ts_err("Failed to alloc rawdata info memory");
		return GTP_RESULT_INVALID;
	}

	if (goodix_get_rawdata(&ts_core->pdev->dev, info)) {
		ts_err("Factory_test FAIL");
		test_result = GTP_RESULT_INVALID;
		goto exit;
	}

	if (2 == info->result[0]) {
		ts_info("test PASS!");
		test_result = GTP_RESULT_PASS;
	} else {
		ts_err("test FAILED!");
		test_result = GTP_RESULT_FAIL;
	}

exit:
	ts_info("resultInfo: %s", info->result);
	/* ret = snprintf(buf, PAGE_SIZE, "resultInfo: %s", info->result); */

	vfree(info);
	return test_result;
}

static int goodix_ic_self_test(char *type, int *result)
{
	struct goodix_fw_version chip_ver;
	struct goodix_ts_hw_ops *hw_ops;
	int retval = 0;

	if (!ts_core)
		return GTP_RESULT_INVALID;
	else
		hw_ops = ts_core->hw_ops;

	if (atomic_read(&ts_core->suspended)) {
		ts_info("in suspend mode,can not to do\n");
		*result = GTP_RESULT_INVALID;
		return 0;
	}

	if (!strncmp("short", type, 5) || !strncmp("open", type, 4)) {
		retval = goodix_short_open_test();
	} else if (!strncmp("i2c", type, 3)) {
		hw_ops->read_version(ts_core, &chip_ver);
		if (chip_ver.sensor_id == 255)
			retval = GTP_RESULT_PASS;
		else
			retval = GTP_RESULT_FAIL;
	}

	*result = retval;

	return 0;
}

static int goodix_ic_data_collect(char *buf, int *length)
{
	struct ts_rawdata_info *info;
	int tx;
	int rx;
	int ret;
	int i;
	int index;
	int buf_size = PAGE_SIZE * 3;
	int cnt = 0;

	if (!ts_core) {
		ts_err("ts_core null ptr");
		return -EIO;
	}

	if (atomic_read(&ts_core->suspended)) {
		ts_info("in suspend mode,can not read\n");
		return 0;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		ts_err("Failed to alloc rawdata info memory");
		return -ENOMEM;
	}

	ret = ts_core->hw_ops->get_capacitance_data(ts_core, info);
	if (ret < 0) {
		ts_err("failed to get_capacitance_data, exit!");
		goto exit;
	}

	rx = info->buff[0];
	tx = info->buff[1];
	cnt = snprintf(buf + cnt, buf_size - cnt, "TX:%d  RX:%d\n", tx, rx);
	cnt += snprintf(buf + cnt, buf_size - cnt, "mutual_rawdata:\n");
	index = 2;
	for (i = 0; i < tx * rx; i++) {
		cnt += snprintf(buf + cnt, buf_size - cnt, "%5d,", info->buff[index + i]);
		if ((i + 1) % tx == 0)
			cnt += snprintf(buf + cnt, buf_size - cnt, "\n");
	}
	cnt += snprintf(buf + cnt, buf_size - cnt, "mutual_diffdata:\n");
	index += tx * rx;
	for (i = 0; i < tx * rx; i++) {
		cnt += snprintf(buf + cnt, buf_size - cnt, "%3d,", info->buff[index + i]);
		if ((i + 1) % tx == 0)
			cnt += snprintf(buf + cnt, buf_size - cnt, "\n");
	}
	*length = cnt;
exit:
	kfree(info);
	return ret;
}

static int goodix_lockdown_info_read(void)
{
	int ret = 0;
	struct goodix_ts_hw_ops *hw_ops;
	if (!ts_core)
		return GTP_RESULT_INVALID;

	if (ts_core->lockdown_info_ready) {
		ts_info("lockdown info is ready, not read again from flash\n");
		return 0;
	}
	hw_ops = ts_core->hw_ops;
	ret = hw_ops->read(ts_core, GOODIX_LOCKDOWN_ADDR,
				ts_core->lockdown_info, GOODIX_LOCKDOWN_SIZE);
	if (ret) {
		ts_err("can't get lockdown");
		return -EINVAL;
	}
	ts_core->lockdown_info_ready = 1;

	ts_info("lockdown is:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x",
			ts_core->lockdown_info[0], ts_core->lockdown_info[1],
			ts_core->lockdown_info[2], ts_core->lockdown_info[3],
			ts_core->lockdown_info[4], ts_core->lockdown_info[5],
			ts_core->lockdown_info[6], ts_core->lockdown_info[7]);
	return 0;
}

static int goodix_get_lockdown_info(u8 lockdown_info[8])
{
	int ret = goodix_lockdown_info_read();
	if (ret < 0)
		return ret;
	lockdown_info[0] = ts_core->lockdown_info[0];
	lockdown_info[1] = ts_core->lockdown_info[1];
	lockdown_info[2] = ts_core->lockdown_info[2];
	lockdown_info[3] = ts_core->lockdown_info[3];
	lockdown_info[4] = ts_core->lockdown_info[4];
	lockdown_info[5] = ts_core->lockdown_info[5];
	lockdown_info[6] = ts_core->lockdown_info[6];
	lockdown_info[7] = ts_core->lockdown_info[7];
	return 0;
}

static int goodix_ic_resume_suspend(bool is_resume, u8 gesture_type)
{
    if (ts_core == NULL) {
        ts_err("xiaomi touch ts core null");
        return -1;
    }
    if (is_resume) {
        ts_info("goodix_ic_resume\n");
	if (ts_core->dev_resume) {
	    return ts_core->dev_resume(ts_core);
	} else {
	    ts_err("goodix_ic_resume null\n");
	    return -1;
	}
    }
    if (ts_core->dev_suspend) {
        ts_info("goodix_ic_suspend\n");
	return ts_core->dev_suspend(ts_core);
	} else {
            ts_err("goodix_ic_suspend null\n");
	    return -1;
    }
}

#ifdef GOODIX_DEBUGFS_ENABLE
static void tpdbg_suspend(struct goodix_ts_core *core_data, bool enable)
{
	schedule_resume_suspend_work(SEC_TOUCH_ID, !enable);
}

static int tpdbg_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t tpdbg_read(struct file *file, char __user *buf, size_t size,
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

static ssize_t tpdbg_write(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	struct goodix_ts_core *core_data = file->private_data;
	struct goodix_ts_hw_ops *hw_ops = core_data->hw_ops;
	char *cmd = kzalloc(size + 1, GFP_KERNEL);
	int ret = size;

	if (!cmd)
		return -ENOMEM;

	if (core_data->init_stage < CORE_INIT_STAGE2) {
		ts_err("initialization not completed");
		ret = -EFAULT;
		goto out;
	}

	if (copy_from_user(cmd, buf, size)) {
		ret = -EFAULT;
		goto out;
	}

	cmd[size] = '\0';

	if (!strncmp(cmd, "irq-disable", 11))
		hw_ops->irq_enable(core_data, false);
	else if (!strncmp(cmd, "irq-enable", 10))
		hw_ops->irq_enable(core_data, true);
	else if (!strncmp(cmd, "tp-sd-en", 8))
		tpdbg_suspend(core_data, true);
	else if (!strncmp(cmd, "tp-sd-off", 9))
		tpdbg_suspend(core_data, false);
	else if (!strncmp(cmd, "tp-suspend-en", 13))
		tpdbg_suspend(core_data, true);
	else if (!strncmp(cmd, "tp-suspend-off", 14))
		tpdbg_suspend(core_data, false);
out:
	kfree(cmd);

	return ret;
}

static int tpdbg_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static const struct file_operations tpdbg_operations = {
	.owner = THIS_MODULE,
	.open = tpdbg_open,
	.read = tpdbg_read,
	.write = tpdbg_write,
	.release = tpdbg_release,
};
#endif

static int goodix_touch_log_level_control(bool flag)
{
	coord_log_flag = flag;
	return 0;
}

static void goodix_set_charge_state(int state)
{
	if (!ts_core) {
		ts_err("ts_core not inited");
		return;
	} else {
		ts_info("state:%d", state);
	}
	if (atomic_read(&ts_core->suspended)) {
		ts_info("in suspend mode,don't need to set charge state\n");
		return;
	}
	if (state)
		ts_core->hw_ops->charger_on(ts_core, state);
	else
		ts_core->hw_ops->charger_on(ts_core, state);
}

#define GOODIX_TEMPCMD	0x60
#define GOODIX_TEMPCMD_LEN	0x05
int goodix_set_thermal_temp(int temp, bool force)
{
	int ret = 0;
	int temp0 = 0;
	struct goodix_ts_cmd cmd;

	if (atomic_read(&ts_core->suspended) == 1) {
		ts_info("in suspend mode,don't need to set charge state\n");
		return -1;
	}
	if (force) {
		temp0 = get_bms_temp_common();
		if (abs(temp0) >= INVAILD_TEMPERATURE)
			return -1;

		temp = (temp0 + 5) / 10; // Rounding, in degrees Celsius
	}

	ts_info("temp: %d", temp);
	cmd.cmd = GOODIX_TEMPCMD;
	cmd.len = GOODIX_TEMPCMD_LEN;
	cmd.data[0] = temp;
	ret = ts_core->hw_ops->send_cmd(ts_core, &cmd);
	if (ret)
		ts_err("failed send thermal_temp cmd");
	else
		ts_info("send thermal_temp cmd success");

	return ret;
}

static void goodix_set_gesture_type(u8 val)
{
	if ((val & GESTURE_SINGLETAP_EVENT))
		ts_core->gesture_type |= (1 << 12);
	else
		ts_core->gesture_type &= ~(1 << 12);

	if ((val & GESTURE_DOUBLETAP_EVENT))
		ts_core->gesture_type |= (1 << 7);
	else
		ts_core->gesture_type &= ~(1 << 7);
}

static void goodix_ic_switch_mode(u8 _gesture_type)
{
	if (!ts_core) {
		ts_err("goodix core data is NULL");
		return;
	}
#ifdef CONFIG_TOUCH_FACTORY_BUILD
	ts_info("factory version,skip set gesture mode\n");
	return;
#endif
	goodix_set_gesture_type(_gesture_type);
	if (0 == atomic_read(&ts_core->suspended)) {
		ts_info("tp is in resume state, wait suspend to send cmd!");
		return;
	}
	ts_info("_gesture_type is 0x%x", _gesture_type);
	if (ts_core->dev_resume) {
		ts_core->dev_resume(ts_core);
	}
	mdelay(30);
	if (ts_core->dev_suspend) {
		ts_core->dev_suspend(ts_core);
	}
}

static int goodix_palm_sensor_write(int value)
{
	if (!ts_core) {
		ts_err("goodix core data is NULL");
		return -EINVAL;
	}

	if (ts_core->init_stage < CORE_INIT_STAGE2) {
		ts_err("initialization not completed");
		return -EFAULT;
	}

	ts_info("palm sensor value : %d", value);
	ts_core->palm_sensor_enable = value;
	if (atomic_read(&ts_core->suspended)) {
  		ts_info("in suspend mode,don't need to write\n");
		return 0;
	}
	ts_core->hw_ops->palm_on(ts_core, !!value);

	return 0;
}

static int goodix_touch_doze_analysis(int input)
{
	int error =  0;
	int irq_status = 0;
	int update_flag = UPDATE_MODE_FORCE | UPDATE_MODE_BLOCK | UPDATE_MODE_SRC_REQUEST;
	const struct goodix_ts_board_data *ts_bdata = NULL;

	ts_info("input: %d", input);
	if (!ts_core) {
		ts_err("cd is null");
		return -EIO;
	}
	if (ts_core->init_stage < CORE_INIT_STAGE2) {
		ts_err("initialization not completed");
		return -EFAULT;
	}


	if (atomic_read(&ts_core->suspended)) {
  		ts_info("touch has suspend,return\n");
		return 0;
	}

	ts_bdata = board_data(ts_core);
	switch(input) {
		case TOUCH_0:
			ts_core->doze_test = 1;
			schedule_resume_suspend_work(SEC_TOUCH_ID, false);
			schedule_resume_suspend_work(SEC_TOUCH_ID, true);
			ts_core->doze_test = 0;
		break;
		case TOUCH_1:
			error = goodix_do_fw_update(NULL, update_flag);
			if (!error)
				ts_info("success do update work");
		break;
		case TOUCH_2:
			enable_irq(ts_core->irq);
		break;
		case TOUCH_3:
			disable_irq(ts_core->irq);
		break;
		case TOUCH_4:
			goodix_ts_irq_setup(ts_core);
		break;
		case TOUCH_5:
			irq_status = gpio_get_value(ts_bdata->irq_gpio) == 0 ? 0 : 1;
		break;
		case TOUCH_6:
			schedule_resume_suspend_work(SEC_TOUCH_ID, false);
		break;
		case TOUCH_7:
			schedule_resume_suspend_work(SEC_TOUCH_ID, true);
		break;
		case TOUCH_8:
			error = goodix_ts_power_on(ts_core);
			if (error < 0)
				ts_err("Failed to enable regulators");
		break;
		case TOUCH_9:
			error = goodix_ts_power_off(ts_core);
			if (error < 0)
				ts_err("Failed to disable regulators");
		break;
		default:
			ts_err("don't support touch doze analysis");
			break;
	}
	return irq_status;
}

static int goodix_fw_version_info_read(char *buf)
{
	int cnt = 0;
	int length = 64;

	if (!ts_core) {
		ts_err("goodix core data is NULL");
		return -EINVAL;
	}

  	if (ts_core->init_stage < CORE_INIT_STAGE2)
	{
		ts_info("goodix init fail,return");
  		return -EINVAL;
	}

	cnt += snprintf(&buf[0], length,
		"patch_vid:%02x%02x%02x%02x ",ts_core->fw_version.patch_vid[0],
		ts_core->fw_version.patch_vid[1], ts_core->fw_version.patch_vid[2],
		ts_core->fw_version.patch_vid[3]);
	cnt += snprintf(&buf[cnt], length, "cfg_id:%x ", ts_core->ic_info.version.config_id);
	cnt += snprintf(&buf[cnt], length, "cfg_version:%x ", ts_core->ic_info.version.config_version);
	cnt += snprintf(&buf[cnt], length, "sensorid:%d", ts_core->fw_version.sensor_id);
	return cnt;
}

void goodix_orientation_update(u8 orientation)
{
	u8 data = 0;

	ts_info("DATA_MODE_8 %d", orientation);
	if (PANEL_ORIENTATION_DEGREE_90 == orientation)
		orientation = 1;
	else if (PANEL_ORIENTATION_DEGREE_180 == orientation)
		orientation = 3;
	else if (PANEL_ORIENTATION_DEGREE_270 == orientation)
		orientation = 2;
	else
		orientation = 0;

	data |= (orientation << 6);
	ts_core->hw_ops->panel_orien(ts_core, data);
}


static void goodix_set_mode_value(int mode, int *value){
	struct goodix_ts_cmd cmd;
	if(*value != 0 && *value != 1)
		ts_err("Invaild data0 %d", *value);
	cmd.cmd = INPUT_METHOD_CMD;
	cmd.len = 5;
	cmd.data[0] = *value;
	cmd.data[1] = *value ? 0x3B : 0x3A;
	if (ts_core->hw_ops->send_cmd(ts_core, &cmd))
		ts_err("failed send input_method cmd");
	else
		ts_info("send input_method cmd success");
}
static void goodix_cmd_mode_update(long mode_update_flag, int mode_value[DATA_MODE_35])
{
	u8 orientation;

#ifdef CONFIG_TOUCH_FACTORY_BUILD
	ts_info("factory version,skip mode set\n");
	return;
#endif

	if (!ts_core) {
		ts_err("ts_core not inited");
		return;
	}

	if (atomic_read(&ts_core->suspended)) {
		ts_info("in suspend mode,return\n");
		return;
	}
	ts_info("mode_update_flag 0x%02lX", mode_update_flag);

	if (mode_update_flag & (1 << DATA_MODE_8)) {
		orientation = (u8)mode_value[DATA_MODE_8];
		goodix_orientation_update(orientation);
	}
}

int nfc_interference_event(struct goodix_ts_core *cd, u8 val)
{
	struct goodix_ts_cmd cmd;
	//ts_info("enter_send_ic_cmd %d",val);
	cmd.cmd = GOODIX_NFC_CMD;
	cmd.len = 5;
	cmd.data[0] = val;
	if (cd->hw_ops->send_cmd(cd, &cmd)) {
		ts_err("failed send nfc cmd");
		return -EINVAL;
	} else {
		ts_info("send nfc cmd success %d",val);
		ts_core->ic_nfc_status = val;
	}

	return 0;
}


void nfc_on_noise_off_work(struct work_struct *work)
{
	struct goodix_ts_cmd cmd;
	//ts_info("enter send 01 ");
	cmd.cmd = GOODIX_NFC_CMD;
	cmd.len = 5;
	cmd.data[0] = NFC_ON_NOISE_END_EVENT;
	if (ts_core->hw_ops->send_cmd(ts_core, &cmd)) {
		ts_err("failed send nfc cmd");
	} else {
		ts_info("send nfc cmd success %d",NFC_ON_NOISE_END_EVENT);
		ts_core->ic_nfc_status = NFC_ON_NOISE_END_EVENT;
	}
}


void nfc_on_noise_off_event(struct timer_list *t)
{
	queue_work(ts_core->nfc_workqueue, &ts_core->nfc_work);
}

void goodix_nfc_to_touch_event(u8 value)
{
	ts_info("enter %d",value);

	switch (value) {
	case 0:
		ts_core->nfc_cmd = NFC_OFF_EVENT;
		if(ts_core->suspend == 0)
			nfc_interference_event(ts_core, NFC_OFF_EVENT);
		break;
	case 1:
		ts_core->nfc_cmd = NFC_ON_NOISE_END_EVENT;
		if(ts_core->suspend == 0)
			nfc_interference_event(ts_core, NFC_ON_NOISE_END_EVENT);
		break;
	case 2:
		ts_core->nfc_cmd = NFC_NOISE_START_EVENT;
		if(ts_core->nfc_cmd != ts_core->ic_nfc_status && ts_core->suspend == 0)
			nfc_interference_event(ts_core, NFC_NOISE_START_EVENT);
		else
			ts_info("equal_donot_send_cmd:%d",ts_core->nfc_cmd);
		break;
	case 3:
		ts_core->nfc_cmd = NFC_ON_NOISE_END_EVENT;
		if(ts_core->nfc_cmd != ts_core->ic_nfc_status && ts_core->suspend == 0)
			mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));	
		break;
	default:
		break;
	}
	//ts_info("exit------------- 0x%x",ts_core->nfc_cmd);

}

bool goodix_get_wakeup_enable(void)
{
	return !!driver_get_touch_mode(SEC_TOUCH_ID, DATA_MODE_30);
}

int xiaomi_touch_goodix_probe(struct goodix_ts_core *core_data)
{
    if (core_data == NULL) {
        ts_err("xiaomi touch failed core_data null");
        return -1;
    }
    ts_core = core_data;
    hardware_param.temp_change_value = 2;

    memset(hardware_param.driver_version, 0, 64);
    memset(hardware_param.fw_version, 0, 64);
    memcpy(hardware_param.driver_version, GOODIX_DRIVER_VERSION, strlen(GOODIX_DRIVER_VERSION));
    goodix_fw_version_info_read(hardware_param.fw_version);
    memset(&hardware_operation, 0, sizeof(hardware_operation_t));
    hardware_operation.ic_resume_suspend = goodix_ic_resume_suspend;
    hardware_operation.ic_self_test = goodix_ic_self_test;
    hardware_operation.ic_data_collect = goodix_ic_data_collect;
    hardware_operation.ic_get_lockdown_info = goodix_get_lockdown_info;
    hardware_operation.ic_set_charge_state = goodix_set_charge_state;
    hardware_operation.ic_switch_mode = goodix_ic_switch_mode;
    hardware_operation.ic_get_fw_version = goodix_fw_version_info_read;
    hardware_operation.set_mode_value = goodix_set_mode_value;
    hardware_operation.set_mode_long_value = NULL;
    hardware_operation.cmd_update_func = goodix_cmd_mode_update;
    hardware_operation.palm_sensor_write = goodix_palm_sensor_write;
    hardware_operation.panel_vendor_read = NULL;
    hardware_operation.panel_color_read = NULL;
    hardware_operation.panel_display_read = NULL;
    hardware_operation.touch_vendor_read = NULL;
    hardware_operation.get_touch_ic_buffer = NULL;
    hardware_operation.touch_doze_analysis = goodix_touch_doze_analysis;
    hardware_operation.set_nfc_to_touch_event = goodix_nfc_to_touch_event;
    hardware_operation.touch_log_level_control = goodix_touch_log_level_control;
    hardware_operation.set_thermal_temp = goodix_set_thermal_temp;
    register_touch_panel(ts_core->bus->dev, SEC_TOUCH_ID, &hardware_param, &hardware_operation);
    xiaomi_register_panel_notifier(ts_core->bus->dev, SEC_TOUCH_ID,
        PANEL_EVENT_NOTIFICATION_SECONDARY, PANEL_EVENT_NOTIFIER_CLIENT_SECONDARY_TOUCH);

	timer_setup(&my_timer, nfc_on_noise_off_event, 0);
	ts_core->nfc_workqueue = create_singlethread_workqueue("goodix_nfc");
	INIT_WORK(&ts_core->nfc_work, nfc_on_noise_off_work);
#ifdef GOODIX_DEBUGFS_ENABLE
	ts_core->debugfs = debugfs_create_dir("tp_debug_sec", NULL);
	if (ts_core->debugfs) {
		debugfs_create_file("switch_state", 0660, ts_core->debugfs, ts_core,
					&tpdbg_operations);
	}
#endif
    return 0;
}

void goodix_xiaomi_touch_remove(struct goodix_ts_core *core_data)
{
	if (core_data == NULL) {
		ts_err("core_data null,return");
		return;
	}
	xiaomi_unregister_panel_notifier(core_data->bus->dev, SEC_TOUCH_ID);
	unregister_touch_panel(SEC_TOUCH_ID);

	del_timer(&my_timer);

	cancel_work_sync(&ts_core->nfc_work);
	flush_workqueue(ts_core->nfc_workqueue);
	destroy_workqueue(ts_core->nfc_workqueue);
}