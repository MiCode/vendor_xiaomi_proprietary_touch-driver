// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics TouchCom touchscreen driver
 *
 * Copyright (C) 2017-2020 Synaptics Incorporated. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

/**
 * @file syna_tcm2_sysfs.c
 *
 * This file implements sysfs attributes in the reference driver.
 */

#include <linux/string.h>

#include "syna_tcm2.h"
#include "synaptics_touchcom_core_dev.h"
#include "synaptics_touchcom_func_base.h"
#ifdef HAS_TESTING_FEATURE
#include "syna_tcm2_testing.h"
#endif

bool debug_log_flag = false;

/* g_sysfs_dir represents the root directory of sysfs nodes being created
 */
static struct kobject *g_sysfs_dir;

/**
 * syna_sysfs_info_show()
 *
 * Attribute to show the device and driver information to the console.
 *
 * @param
 *    [ in] kobj:  an instance of kobj
 *    [ in] attr:  an instance of kobj attribute structure
 *    [out] buf:  string buffer shown on console
 *
 * @return
 *    on success, number of characters being output;
 *    otherwise, negative value on error.
 */
static ssize_t syna_sysfs_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int retval;
	unsigned int count;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;
	struct tcm_dev *tcm_dev;
	int i;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);
	tcm_dev = tcm->tcm_dev;

	count = 0;

	retval = snprintf(buf, PAGE_SIZE - count,
			"Driver version:     %d.%s\n",
			SYNAPTICS_TCM_DRIVER_VERSION,
			SYNAPTICS_TCM_DRIVER_SUBVER);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
			"Core lib version:   %d.%02d\n\n",
			(unsigned char)(SYNA_TCM_CORE_LIB_VERSION >> 8),
			(unsigned char)SYNA_TCM_CORE_LIB_VERSION);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
			 "Character Dev. Node: /dev/%s* (%d)\n\n",
			CHAR_DEVICE_NAME, tcm->char_dev_ref_count);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	if (!tcm->is_connected) {
		retval = snprintf(buf, PAGE_SIZE - count,
				"Device is NOT connected\n");
		count += retval;
		retval = count;
		goto exit;
	}

	if (tcm->pwr_state == BARE_MODE) {
		retval = count;
		goto exit;
	}

	retval = snprintf(buf, PAGE_SIZE - count,
			"TouchComm version:  %d\n", tcm_dev->id_info.version);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	switch (tcm_dev->id_info.mode) {
	case MODE_APPLICATION_FIRMWARE:
		retval = snprintf(buf, PAGE_SIZE - count,
				"Firmware mode:      Application Firmware, 0x%02x\n",
				tcm_dev->id_info.mode);
		if (retval < 0)
			goto exit;
		break;
	case MODE_BOOTLOADER:
		retval = snprintf(buf, PAGE_SIZE - count,
				"Firmware mode:      Bootloader, 0x%02x\n",
				tcm_dev->id_info.mode);
		if (retval < 0)
			goto exit;
		break;
	case MODE_ROMBOOTLOADER:
		retval = snprintf(buf, PAGE_SIZE - count,
				"Firmware mode:      Rom Bootloader, 0x%02x\n",
				tcm_dev->id_info.mode);
		if (retval < 0)
			goto exit;
		break;
	default:
		retval = snprintf(buf, PAGE_SIZE - count,
				"Firmware mode:      Mode 0x%02x\n",
				tcm_dev->id_info.mode);
		if (retval < 0)
			goto exit;
		break;
	}
	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
			"Part number:        ");
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = syna_pal_mem_cpy(buf,
			PAGE_SIZE - count,
			tcm_dev->id_info.part_number,
			sizeof(tcm_dev->id_info.part_number),
			sizeof(tcm_dev->id_info.part_number));
	if (retval < 0) {
		LOGE("Fail to copy part number string\n");
		goto exit;
	}
	buf += sizeof(tcm_dev->id_info.part_number);
	count += sizeof(tcm_dev->id_info.part_number);

	retval = snprintf(buf, PAGE_SIZE - count, "\n");
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
			"Packrat number:     %d\n\n", tcm_dev->packrat_number);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	if (tcm_dev->id_info.mode != MODE_APPLICATION_FIRMWARE) {
		retval = count;
		goto exit;
	}

	retval = snprintf(buf, PAGE_SIZE - count, "Config ID:          ");
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	for (i = 0; i < MAX_SIZE_CONFIG_ID; i++) {
		retval = snprintf(buf, PAGE_SIZE - count,
			"0x%2x ", tcm_dev->config_id[i]);
		if (retval < 0)
			goto exit;
		buf += retval;
		count += retval;
	}

	retval = snprintf(buf, PAGE_SIZE - count, "\n");
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
		"Max X & Y:          %d, %d\n", tcm_dev->max_x, tcm_dev->max_y);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
		"Num of objects:     %d\n", tcm_dev->max_objects);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
		"Num of cols & rows: %d, %d\n", tcm_dev->cols, tcm_dev->rows);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
		"Max. Read Size:     %d bytes\n", tcm_dev->max_rd_size);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = snprintf(buf, PAGE_SIZE - count,
		"Max. Write Size:    %d bytes\n", tcm_dev->max_wr_size);
	if (retval < 0)
		goto exit;

	buf += retval;
	count += retval;

	retval = count;

exit:
	return retval;
}

static struct kobj_attribute kobj_attr_info =
	__ATTR(info, 0444, syna_sysfs_info_show, NULL);

/**
 * syna_sysfs_irq_en_store()
 *
 * Attribute to disable/enable the irq
 *
 * @param
 *    [ in] kobj:  an instance of kobj
 *    [ in] attr:  an instance of kobj attribute structure
 *    [ in] buf:   string buffer input
 *    [ in] count: size of buffer input
 *
 * @return
 *    on success, return count; otherwise, return error code
 */
static ssize_t syna_sysfs_irq_en_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	unsigned int input;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (kstrtouint(buf, 10, &input))
		return -EINVAL;

	if (!tcm->hw_if->ops_enable_irq)
		return 0;

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if (tcm->pwr_state == BARE_MODE) {
		LOGN("In bare connection mode, no irq support\n");
		retval = count;
		goto exit;
	}

	/* disable the interrupt line */
	if (input == 0) {
		retval = tcm->hw_if->ops_enable_irq(tcm->hw_if, false);
		if (retval < 0) {
			LOGE("Fail to disable interrupt\n");
			goto exit;
		}
	} else if (input == 1) {
	/* enable the interrupt line */
		retval = tcm->hw_if->ops_enable_irq(tcm->hw_if, true);
		if (retval < 0) {
			LOGE("Fail to enable interrupt\n");
			goto exit;
		}
	} else {
		LOGW("Unknown option %d (0:disable / 1:enable)\n", input);
		retval = -EINVAL;
		goto exit;
	}

	retval = count;

exit:
	return retval;
}

static struct kobj_attribute kobj_attr_irq_en =
	__ATTR(irq_en, 0220, NULL, syna_sysfs_irq_en_store);

/**
 * syna_sysfs_reset_store()
 *
 * Attribute to issue a reset.
 * "1" for a sw reset; "2" for a hardware reset
 *
 * @param
 *    [ in] kobj:  an instance of kobj
 *    [ in] attr:  an instance of kobj attribute structure
 *    [ in] buf:   string buffer input
 *    [ in] count: size of buffer input
 *
 * @return
 *    on success, return count; otherwise, return error code
 */
static ssize_t syna_sysfs_reset_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	unsigned int input;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (kstrtouint(buf, 10, &input))
		return -EINVAL;

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if ((tcm->pwr_state == BARE_MODE) || (input == 2)) {
		if (!tcm->hw_if->ops_hw_reset) {
			LOGE("No hardware reset support\n");
			goto exit;
		}

		tcm->hw_if->ops_hw_reset(tcm->hw_if,0);

		/* enable the interrupt to process the identify report
		 * after the hardware reset.
		 */
		if (!tcm->hw_if->bdata_attn.irq_enabled) {
			tcm->hw_if->ops_enable_irq(tcm->hw_if, true);
			/* disable it and back to original status */
			syna_pal_sleep_ms(100);
			tcm->hw_if->ops_enable_irq(tcm->hw_if, false);
		}
	} else if (input == 1) {
		retval = syna_tcm_reset(tcm->tcm_dev);
		if (retval < 0) {
			LOGE("Fail to do reset\n");
			goto exit;
		}
	} else {
		LOGW("Unknown option %d (1:sw / 2:hw)\n", input);
		retval = -EINVAL;
		goto exit;
	}

	/* check the fw setup in case the settings is changed */
	if (IS_APP_FW_MODE(tcm->tcm_dev->dev_mode)) {
		retval = tcm->dev_set_up_app_fw(tcm);
		if (retval < 0) {
			LOGE("Fail to set up app fw\n");
			goto exit;
		}
	}

	retval = count;

exit:
	return retval;
}

static struct kobj_attribute kobj_attr_reset =
	__ATTR(reset, 0220, NULL, syna_sysfs_reset_store);

/**
 * syna_sysfs_debug_store()
 *
 * Attribute to disable/enable the debug log
 *
 * @param
 *    [ in] kobj:  an instance of kobj
 *    [ in] attr:  an instance of kobj attribute structure
 *    [ in] buf:   string buffer input
 *    [ in] count: size of buffer input
 *
 * @return
 *    on success, return count; otherwise, return 0
 */
static ssize_t syna_sysfs_debug_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	unsigned int input;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (kstrtouint(buf, 10, &input))
		return -EINVAL;

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if (input == 0) {
		debug_log_flag = false;
	} else if (input == 1) {
		debug_log_flag = true;
	} else {
		LOGW("Unknown option %d (0:disable / 1:enable)\n", input);
		goto exit;
	}
	retval = count;

exit:
	return retval;
}
static struct kobj_attribute kobj_attr_debug =
	__ATTR(debug, 0220, NULL, syna_sysfs_debug_store);

/**
 * syna_sysfs_pwr_store()
 *
 * Attribute to change the power state.
 *
 * @param
 *    [ in] kobj:  an instance of kobj
 *    [ in] attr:  an instance of kobj attribute structure
 *    [ in] buf:   string buffer input
 *    [ in] count: size of buffer input
 *
 * @return
 *    on success, return count; otherwise, return error code
 */
static ssize_t syna_sysfs_pwr_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if (strncmp(buf, "resume", 6) == 0) {
		if (tcm->dev_resume)
			tcm->dev_resume(p_dev, xiaomi_get_gesture_type(PRI_TOUCH_ID));
	} else if (strncmp(buf, "suspend", 7) == 0) {
		if (tcm->dev_suspend)
			tcm->dev_suspend(p_dev, xiaomi_get_gesture_type(PRI_TOUCH_ID));
	} else {
		LOGW("Unknown option %s\n", buf);
		retval = -EINVAL;
		goto exit;
	}

	retval = count;

exit:
	return retval;
}

static struct kobj_attribute kobj_attr_pwr =
	__ATTR(power_state, 0220, NULL, syna_sysfs_pwr_store);

static ssize_t syna_open_close_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;
	bool enable = false;
	unsigned short value = 0;
	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if (strncmp(buf, "enable", 1) == 0) {
		enable = true;
	} else {
		enable = false;
	}

	value = enable ? 0:1;
	LOGI("syna_open_close_store %d",value);
	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
			DC_DISABLE_OPEN_CLOSE_REPORT,
			value,
			RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Fail to set open_colse report via DC command value:%d\n",value);
	}

	return count;
}

static struct kobj_attribute kobj_attr_open_close =
	__ATTR(open_close, 0644, NULL, syna_open_close_store);

static ssize_t syna_sysfs_enable_touch_scan_store(struct kobject *kobj,
		struct kobj_attribute *attr,const char *buf, size_t count)
{
	int retval = 0;
	struct device *p_dev;
	struct kobject *p_kobj;
	struct syna_tcm *tcm;
	unsigned short value = 0;


	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (!tcm->is_connected) {
		LOGW("Device is NOT connected\n");
		return count;
	}

	if (strncmp(buf, "enable", 1) == 0) {
		value = 0x00;
	} else {
		value = 0x03;
	}

	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
				DC_ENABLE_WAKEUP_GESTURE_MODE,
				value,
				RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Failed to set dynamic_config, retval=%d\n", retval);

	}	
	return count;
}
static struct kobj_attribute kobj_attr_enable_touch_scan =
	__ATTR(enable_touch_scan, 0644, NULL, syna_sysfs_enable_touch_scan_store);

static ssize_t syna_sysfs_disable_trans_abs_capfold(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	unsigned short value = 0;
	struct kobject *p_kobj;
	struct device *p_dev;
	struct syna_tcm *tcm;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);

	if (!tcm->is_connected) {
		LOGE("Device is NOT connected\n");
		return count;
	}

	if (strncmp(buf, "0", 1) == 0) {
		value = 0x00;
	} else if(strncmp(buf, "1", 1) == 0){
		value = 0x01;
	}else if(strncmp(buf, "2", 1) == 0){
		value = 0x02;
	}else if(strncmp(buf, "3", 1) == 0){
		value = 0x8000;
	}

	retval = syna_tcm_set_dynamic_config(tcm->tcm_dev,
				DC_DISABLE_CAPFOLD,
				value,
				RESP_IN_ATTN);
	if (retval < 0) {
		LOGE("Failed to set dynamic_config, retval=%d\n", retval);
		goto exit;
	}
	LOGI("Set dynamic_config successful!\n");
exit:
	return count;
}

static struct kobj_attribute kobj_attr_capfold_disable =
	__ATTR(capfold_disable, 0220, NULL, syna_sysfs_disable_trans_abs_capfold);

static ssize_t syna_capfold_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned short status;
	int retval = 0;
	struct kobject *p_kobj;
	struct device *p_dev;
	struct syna_tcm *tcm;
	struct tcm_dev *tcm_dev;

	p_kobj = g_sysfs_dir->parent;
	p_dev = container_of(p_kobj, struct device, kobj);
	tcm = dev_get_drvdata(p_dev);
	tcm_dev = tcm->tcm_dev;
	
	
	retval = syna_tcm_get_capfold_status(tcm_dev, &status, 0);
	if(retval < 0) {
		retval = snprintf(buf, PAGE_SIZE, "Fail to get capfold_status\n");
		return retval;
	}
	retval = snprintf(buf, PAGE_SIZE, "capfold_status: %d\n", status);
	return retval;
}

static struct kobj_attribute kobj_attr_capfold_status =
	__ATTR(capfold_status, 0644, syna_capfold_status_show, NULL);

/**
 * declaration of sysfs attributes
 */
static struct attribute *attrs[] = {
	&kobj_attr_info.attr,
	&kobj_attr_irq_en.attr,
	&kobj_attr_reset.attr,
	&kobj_attr_pwr.attr,
	&kobj_attr_capfold_disable.attr,
	&kobj_attr_debug.attr,
	&kobj_attr_enable_touch_scan.attr,
	&kobj_attr_open_close.attr,
	&kobj_attr_capfold_status.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

/**
 * syna_sysfs_create_dir()
 *
 * Create a directory and register it with sysfs.
 * Then, create all defined sysfs files.
 *
 * @param
 *    [ in] tcm:  the driver handle
 *    [ in] pdev: an instance of platform device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
int syna_sysfs_create_dir(struct syna_tcm *tcm,
		struct platform_device *pdev)
{
	int retval = 0;

	g_sysfs_dir = kobject_create_and_add(PLATFORM_DRIVER_NAME,
			&pdev->dev.kobj);
	if (!g_sysfs_dir) {
		LOGE("Fail to create sysfs directory\n");
		return -ENOTDIR;
	}

	tcm->sysfs_dir = g_sysfs_dir;

	retval = sysfs_create_group(g_sysfs_dir, &attr_group);
	if (retval < 0) {
		LOGE("Fail to create sysfs group\n");

		kobject_put(tcm->sysfs_dir);
		return retval;
	}

#ifdef HAS_TESTING_FEATURE
	retval = syna_testing_create_dir(tcm, g_sysfs_dir);
	if (retval < 0) {
		LOGE("Fail to create testing sysfs\n");

		sysfs_remove_group(tcm->sysfs_dir, &attr_group);
		kobject_put(tcm->sysfs_dir);
		return retval;
	}
#endif

	return 0;
}
/**
 * syna_sysfs_remove_dir()
 *
 * Remove the allocate sysfs directory
 *
 * @param
 *    [ in] tcm: the driver handle
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
void syna_sysfs_remove_dir(struct syna_tcm *tcm)
{
	if (!tcm) {
		LOGE("Invalid tcm device handle\n");
		return;
	}

	if (tcm->sysfs_dir) {
#ifdef HAS_TESTING_FEATURE
		syna_testing_remove_dir();
#endif

		sysfs_remove_group(tcm->sysfs_dir, &attr_group);

		kobject_put(tcm->sysfs_dir);
	}

}
