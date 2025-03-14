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
 * @file syna_tcm2_platform_spi.c
 *
 * This file is the reference code of I2C module used for communicating with
 * Synaptics TouchCom device using I2C
 */

#include <linux/spi/spi.h>
#include <linux/interrupt.h>

#include "syna_tcm2.h"
#include "syna_tcm2_cdev.h"
#include "syna_tcm2_platform.h"
#include "../xiaomi/xiaomi_touch.h"

#if (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
#define SPI_NO_DELAY_USEC
#endif

#define SPI_MODULE_NAME "synaptics_tcm_spi"

static unsigned char *rx_buf;
static unsigned char *tx_buf;

static unsigned int buf_size;

static struct spi_transfer *xfer;

static struct platform_device *syna_spi_device;

#ifdef TOUCH_SPI_CS_CLK_DELAY
struct spi_geni_qcom_ctrl_data {
	u32 spi_cs_clk_delay;
	u32 spi_inter_words_delay;
};

static struct spi_geni_qcom_ctrl_data qcom_ctrl_data = {
	.spi_cs_clk_delay = 30,
	.spi_inter_words_delay = 0,
};
#endif

/**
 * @brief FOR TUNE DEBUG LOG
 * 0 -- disable (default)
 * 1 -- read all the report and response
 * 2 -- don't print the report except identify report
 * 3 -- don't print the report (>REPORT_TOUCH)
 */
int tune_debug_enable = 0;//FOR_LOG_DEBUG
module_param_named(tune_debug_enable, tune_debug_enable, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define SYNA_SPI_TRANSFER_READ 0
#define SYNA_SPI_TRANSFER_WRITE 1
#define SYNA_SPI_PRINT_BUF_SIZE 256
#define SYNA_SPI_PRINT_BUF_LEFT_SIZE 5
#define SYNA_TCM_MESSAGE_MARKER 0xa5
static void syna_print_xfer_data(unsigned char *data, unsigned int length, unsigned int wr_rd)
{
	static unsigned char last_code = 0;
	unsigned char code = 0;
	char print_buf[SYNA_SPI_PRINT_BUF_SIZE] = {0};
	int i, cnt, offset, left;

	if(!tune_debug_enable)
		return;

	if (!data)
		goto exit;

	if ((wr_rd == SYNA_SPI_TRANSFER_READ) && (data[0] == SYNA_TCM_MESSAGE_MARKER)) {
		code = data[1];
	}

	if(tune_debug_enable == 1) {
		/* print all the report and response */
		/* print all the write data */
	} else if (tune_debug_enable == 2) {
		/* skip all the report data except identify report */
		if (((code == STATUS_CONTINUED_READ) && (last_code > REPORT_IDENTIFY) ) || (code > REPORT_IDENTIFY))
			goto exit;
	} else if (tune_debug_enable == 3) {
		/* skip all the report (> REPORT_TOUCH) */
		if (((code == STATUS_CONTINUED_READ) && (last_code > REPORT_TOUCH) ) || (code > REPORT_TOUCH))
			goto exit;
	} else {
		goto exit;
	}

	offset = 0;
	cnt = snprintf(print_buf + offset, SYNA_SPI_PRINT_BUF_SIZE - offset, "%s ",
					(wr_rd == SYNA_SPI_TRANSFER_READ) ? "RD:" : "WR:");
	offset += cnt;
	for (i = 0; i < length; i++) {
		left = SYNA_SPI_PRINT_BUF_SIZE - offset;
		if (left <= SYNA_SPI_PRINT_BUF_LEFT_SIZE) {
			break;
		}
		cnt = snprintf(print_buf + offset, SYNA_SPI_PRINT_BUF_SIZE - offset, "%02x ", data[i]);
		offset += cnt;
	}
	cnt = snprintf(print_buf + offset, SYNA_SPI_PRINT_BUF_SIZE - offset, "\n");
	offset += cnt;
	LOGI("%s", print_buf);

exit:
	last_code = code;
	return;
}


/**
 * syna_request_managed_device()
 *
 * Request and return the device pointer for managed
 *
 * @param
 *     none.
 *
 * @return
 *     a device pointer allocated previously
 */
#if defined(DEV_MANAGED_API) || defined(USE_DRM_PANEL_NOTIFIER)
struct device *syna_request_managed_device(void)
{
	if (!syna_spi_device)
		return NULL;

	return &syna_spi_device->dev;
}
#endif


/**
 * syna_spi_request_gpio()
 *
 * Setup the given gpio
 *
 * @param
 *    [ in] gpio:   the target gpio
 *    [ in] config: '1' for setting up, and '0' to release the gpio
 *    [ in] dir:    default direction of gpio
 *    [ in] state:  default state of gpio
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_request_gpio(int gpio, bool config, int dir,
		int state, char *label)
{
	int retval;
#ifdef DEV_MANAGED_API
	struct device *dev = syna_request_managed_device();

	if (!dev) {
		LOGE("Invalid managed device\n");
		return -ENODEV;
	}
#endif

	if (gpio < 0) {
		LOGE("Invalid gpio pin\n");
		return -EINVAL;
	}

	if (config) {
		retval = snprintf(label, 16, "tcm_gpio_%d\n", gpio);
		if (retval < 0) {
			LOGE("Fail to set GPIO label\n");
			return retval;
		}
#ifdef DEV_MANAGED_API
		retval = devm_gpio_request(dev, gpio, label);
#else /* Legacy API */
		retval = gpio_request(gpio, label);
#endif
		if (retval < 0) {
			LOGE("Fail to request GPIO %d\n", gpio);
			return retval;
		}

		if (dir == 0)
			retval = gpio_direction_input(gpio);
		else
			retval = gpio_direction_output(gpio, state);

		if (retval < 0) {
			LOGE("Fail to set GPIO %d direction\n", gpio);
			return retval;
		}
	} else {
#ifdef DEV_MANAGED_API
		gpio_free(gpio);
#else /* Legacy API */
		gpio_free(gpio);
#endif
	}

	return 0;
}
/**
 * syna_spi_free_gpios()
 *
 * Release the GPIOs requested previously
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_free_gpios(struct syna_hw_interface *hw_if)
{
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	/* release gpios */
	if (rst->reset_gpio > 0)
		syna_spi_request_gpio(rst->reset_gpio, false, 0, 0, NULL);
	if (attn->irq_gpio > 0)
		syna_spi_request_gpio(attn->irq_gpio, false, 0, 0, NULL);
	if (bus->switch_gpio > 0)
		syna_spi_request_gpio(bus->switch_gpio, false, 0, 0, NULL);

	return 0;
}
/**
 * syna_spi_config_gpios()
 *
 * Initialize the GPIOs defined in device tree
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_config_gpios(struct syna_hw_interface *hw_if)
{
	int retval;
	static char str_irq_gpio[32] = {0};
	static char str_rst_gpio[32] = {0};
	static char str_io_switch_gpio[32] = {0};
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	if (attn->irq_gpio > 0) {
		retval = syna_spi_request_gpio(attn->irq_gpio,
				true, 0, 0, str_irq_gpio);
		if (retval < 0) {
			LOGE("Fail to configure interrupt GPIO %d\n",
				attn->irq_gpio);
			goto err_set_gpio_irq;
		}
	}

	if (rst->reset_gpio > 0) {
		retval = syna_spi_request_gpio(rst->reset_gpio,
				true, 1, !rst->reset_on_state,
				str_rst_gpio);
		if (retval < 0) {
			LOGE("Fail to configure reset GPIO %d\n",
				rst->reset_gpio);
			goto err_set_gpio_reset;
		}
	}

	if (bus->switch_gpio > 0) {
		retval = syna_spi_request_gpio(bus->switch_gpio,
				true, 1, bus->switch_state,
				str_io_switch_gpio);
		if (retval < 0) {
			LOGE("Fail to configure switch GPIO %d\n",
				bus->switch_gpio);
			goto err_set_gpio_switch;
		}
	}

	return 0;

err_set_gpio_switch:
	if (rst->reset_gpio >= 0)
		syna_spi_request_gpio(rst->reset_gpio, false, 0, 0, NULL);
err_set_gpio_reset:
	if (attn->irq_gpio >= 0)
		syna_spi_request_gpio(attn->irq_gpio, false, 0, 0, NULL);
err_set_gpio_irq:
	return retval;
}

/**
 * syna_spi_parse_dt()
 *
 * Parse and obtain board specific data from the device tree source file.
 * Keep the data in structure syna_tcm_hw_data for later using.
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] dev: device model
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
#ifdef CONFIG_OF
static int syna_spi_parse_dt(struct syna_hw_interface *hw_if,
		struct device *dev)
{
	int retval;
	u32 value;
	struct property *prop;
	struct device_node *np = dev->of_node;
	const char *name;
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	prop = of_find_property(np, "synaptics,irq-gpio", NULL);
	if (prop && prop->length) {
		attn->irq_gpio = of_get_named_gpio(np,
				"synaptics,irq-gpio", 0);
		attn->irq_flags = SYNAPTICS_TCM_IRQF_TRIGGER_MASK;
	} else {
		attn->irq_gpio = -1;
	}

	retval = of_property_read_u32(np, "synaptics,irq-on-state", &value);
	if (retval < 0)
		attn->irq_on_state = 0;
	else
		attn->irq_on_state = value;

	prop = of_find_property(np, "synaptics,power-supply", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,power-supply",
				&value);
		if (retval < 0) {
			LOGE("Fail to read power-supply property\n");
			return retval;
		}

		pwr->psu = value;

	} else {
		pwr->psu = (int)PSU_REGULATOR;
	}

	retval = of_property_read_string(np, "synaptics,avdd-name", &name);
	if (retval < 0)
		pwr->avdd_reg_name = NULL;
	else
		pwr->avdd_reg_name = name;

	retval = of_property_read_string(np, "synaptics,iovdd-name", &name);
	if (retval < 0)
		pwr->vdd_reg_name = NULL;
	else
		pwr->vdd_reg_name = name;

	prop = of_find_property(np, "synaptics,vdd-gpio", NULL);
	if (prop && prop->length) {
		pwr->vdd_gpio = of_get_named_gpio(np,
				"synaptics,vdd-gpio", 0);
	} else {
		pwr->vdd_gpio = -1;
	}

	prop = of_find_property(np, "synaptics,avdd-gpio", NULL);
	if (prop && prop->length) {
		pwr->avdd_gpio = of_get_named_gpio(np,
				"synaptics,avdd-gpio", 0);
	} else {
		pwr->avdd_gpio = -1;
	}

	prop = of_find_property(np, "synaptics,power-on-state", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,power-on-state",
				&value);
		if (retval < 0) {
			LOGE("Fail to read power-on-state property\n");
			return retval;
		}

		pwr->power_on_state = value;

	} else {
		pwr->power_on_state = 0;
	}

	prop = of_find_property(np, "synaptics,power-delay-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,power-delay-ms",
				&value);
		if (retval < 0) {
			LOGE("Fail to read power-delay-ms property\n");
			return retval;
		}

		pwr->power_delay_ms = value;

	} else {
		pwr->power_delay_ms = 0;
	}

	prop = of_find_property(np, "synaptics,reset-gpio", NULL);
	if (prop && prop->length) {
		rst->reset_gpio = of_get_named_gpio(np,
				"synaptics,reset-gpio", 0);
	} else {
		rst->reset_gpio = -1;
	}

	prop = of_find_property(np, "synaptics,reset-on-state", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-on-state",
				&value);
		if (retval < 0) {
			LOGE("Fail to read reset-on-state property\n");
			return retval;
		}

		rst->reset_on_state = value;

	} else {
		rst->reset_on_state = 0;
	}

	prop = of_find_property(np, "synaptics,reset-active-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-active-ms",
				&value);
		if (retval < 0) {
			LOGE("Fail to read reset-active-ms property\n");
			return retval;
		}

		rst->reset_active_ms = value;

	} else {
		rst->reset_active_ms = 0;
	}

	prop = of_find_property(np, "synaptics,reset-delay-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-delay-ms",
				&value);
		if (retval < 0) {
			LOGE("Fail to read reset-delay-ms property\n");
			return retval;
		}

		rst->reset_delay_ms = value;

	} else {
		rst->reset_delay_ms = 0;
	}

	prop = of_find_property(np, "synaptics,spi-byte-delay-us", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np,
				"synaptics,spi-byte-delay-us", &value);
		if (retval < 0) {
			LOGE("Fail to read byte-delay-us property\n");
			return retval;
		}

		bus->spi_byte_delay_us = value;

	} else {
		bus->spi_byte_delay_us = 0;
	}

	prop = of_find_property(np, "synaptics,spi-block-delay-us", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np,
				"synaptics,spi-block-delay-us", &value);
		if (retval < 0) {
			LOGE("Fail to read block-delay-us property\n");
			return retval;
		}
			bus->spi_block_delay_us = value;

	} else {
		bus->spi_block_delay_us = 0;
	}

	prop = of_find_property(np, "synaptics,spi-mode", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,spi-mode",
				&value);
		if (retval < 0) {
			LOGE("Fail to read synaptics,spi-mode property\n");
			return retval;
		}

		bus->spi_mode = value;

	} else {
		bus->spi_mode = 0;
	}

	prop = of_find_property(np, "synaptics,io-switch-gpio", NULL);
	if (prop && prop->length) {
		bus->switch_gpio = of_get_named_gpio(np,
				"synaptics,io-switch-gpio", 0);
	} else {
		bus->switch_gpio = -1;
	}

	prop = of_find_property(np, "synaptics,io-switch", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,io-switch",
				&value);
		if (retval < 0) {
			LOGE("Fail to read io-switch property\n");
			return retval;
		}

		bus->switch_state = value;

	} else {
		bus->switch_state = 1;
	}

	return xiaomi_parse_dt(dev);
}
#endif

/**
 * syna_tcm_spi_alloc_mem()
 *
 * Manage and allocate the memory to buf being as a temporary buffer for IO
 *
 * @param
 *    [ in] count: number of spi_transfer structures to send
 *    [ in] size:  size of temporary buffer
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_alloc_mem(unsigned int count, unsigned int size)
{
	static unsigned int xfer_count;

	if (count > xfer_count) {
		syna_pal_mem_free((void *)xfer);
		xfer = syna_pal_mem_alloc(count, sizeof(*xfer));
		if (!xfer) {
			LOGE("Fail to allocate memory for xfer\n");
			xfer_count = 0;
			return -ENOMEM;
		}
		xfer_count = count;
	} else {
		syna_pal_mem_set(xfer, 0, count * sizeof(*xfer));
	}

	if (size > buf_size) {
		if (rx_buf) {
			syna_pal_mem_free((void *)rx_buf);
			rx_buf = NULL;
		}
		if (tx_buf) {
			syna_pal_mem_free((void *)tx_buf);
			tx_buf = NULL;
		}

		rx_buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!rx_buf) {
			LOGE("Fail to allocate memory for rx_buf\n");
			buf_size = 0;
			return -ENOMEM;
		}
		tx_buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!tx_buf) {
			LOGE("Fail to allocate memory for tx_buf\n");
			buf_size = 0;
			return -ENOMEM;
		}

		buf_size = size;
	}

	return 0;
}


/**
 * syna_spi_read()
 *
 * TouchCom over SPI requires the host to assert the SSB signal to address
 * the device and retrieve the data.
 *
 * @param
 *    [ in] hw_if:   the handle of hw interface
 *    [out] rd_data: buffer for storing data retrieved from device
 *    [ in] rd_len: number of bytes retrieved from device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_read(struct syna_hw_interface *hw_if,
		unsigned char *rd_data, unsigned int rd_len)
{
	int retval;
	unsigned int idx;
	struct spi_message msg;
	struct spi_device *spi = hw_if->pdev;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;
	struct syna_tcm *tcm = NULL;
#if defined(TOUCH_PLATFORM_XRING)
	unsigned int bak_rd_len = rd_len;
#endif

	if (!spi) {
		LOGE("Invalid bus io device\n");
		return -ENXIO;
	}

	if (!syna_spi_device) {
		LOGE("Invalid platform device\n");
		return -ENXIO;
	}

	tcm = platform_get_drvdata(syna_spi_device);
	if (!tcm) {
		LOGW("Invalid tcm device\n");
		return -ENXIO;
	}

	syna_pal_mutex_lock(&bus->io_mutex);
/*#ifdef TOUCH_SENSORHUB_SUPPORT*/
#if 0
	retval = pm_runtime_get_sync(spi->master->dev.parent);
	if (retval < 0) {
		LOGE("Fail to failed to get sync, retval = %d\n", retval);
	}
#endif
	pm_stay_awake(&syna_spi_device->dev);
	if (tcm->tp_pm_suspend) {
		LOGI("Touch is in pm_suspend status!\n");
		retval = wait_for_completion_timeout(&tcm->pm_resume_completion, msecs_to_jiffies(500));
		if (!retval) {
			retval = -EINVAL;
			LOGE("wait_for_completion_timeout!\n");
			goto exit;
		}
	}

	if ((rd_len & 0xffff) == 0xffff) {
		LOGE("Invalid read length 0x%X\n", (rd_len & 0xffff));
		retval = -EINVAL;
		goto exit;
	}

#if defined(TOUCH_PLATFORM_XRING)
/* NOTE:
 * Due to xring franklin 1 platform requirment,
 * 64-byte-alignment is needed to enable DMA transfer
 */
	if (rd_len > 256 && rd_len % 64)
		rd_len = rd_len + (64 - rd_len % 64);
#endif

	spi_message_init(&msg);

	if (bus->spi_byte_delay_us == 0)
		retval = syna_spi_alloc_mem(1, rd_len);
	else
		retval = syna_spi_alloc_mem(rd_len, rd_len);
	if (retval < 0) {
		LOGE("Fail to allocate memory\n");
		goto exit;
	}

	if (bus->spi_byte_delay_us == 0) {
		syna_pal_mem_set(tx_buf, 0xff, rd_len);
		xfer[0].len = rd_len;
#if defined(TOUCH_PLATFORM_XRING)
		xfer[0].tx_buf = NULL;
#else
		xfer[0].tx_buf = tx_buf;
#endif
		xfer[0].rx_buf = rx_buf;
#ifndef SPI_NO_DELAY_USEC
		if (bus->spi_block_delay_us)
			xfer[0].delay_usecs = bus->spi_block_delay_us;
#endif
		spi_message_add_tail(&xfer[0], &msg);
	} else {
		tx_buf[0] = 0xff;
		for (idx = 0; idx < rd_len; idx++) {
			xfer[idx].len = 1;
			xfer[idx].tx_buf = tx_buf;
			xfer[idx].rx_buf = &rx_buf[idx];
#ifndef SPI_NO_DELAY_USEC
			xfer[idx].delay_usecs = bus->spi_byte_delay_us;
			if (bus->spi_block_delay_us && (idx == rd_len - 1))
				xfer[idx].delay_usecs = bus->spi_block_delay_us;
#endif
			spi_message_add_tail(&xfer[idx], &msg);
		}
	}

	retval = spi_sync(spi, &msg);
	if (retval != 0) {
		LOGE("Failed to complete SPI transfer, error = %d\n", retval);
		goto exit;
	}
#if defined(TOUCH_PLATFORM_XRING)
	rd_len = bak_rd_len;
#endif
	retval = syna_pal_mem_cpy(rd_data, rd_len, rx_buf, rd_len, rd_len);
	if (retval < 0) {
		LOGE("Fail to copy rx_buf to rd_data\n");
		goto exit;
	}

	syna_print_xfer_data(rd_data, rd_len, SYNA_SPI_TRANSFER_READ);

	retval = rd_len;

exit:
	pm_relax(&syna_spi_device->dev);
/*#ifdef TOUCH_SENSORHUB_SUPPORT*/
#if 0
	retval = pm_runtime_put_sync(spi->master->dev.parent);
	if (retval < 0) {
		LOGE("Fail to failed to put sync, retval = %d\n", retval);
	}
#endif
	syna_pal_mutex_unlock(&bus->io_mutex);

	return retval;
}

/**
 * syna_spi_write()
 *
 * TouchCom over SPI requires the host to assert the SSB signal to address
 * the device and send the data to the device.
 *
 * @param
 *    [ in] hw_if:   the handle of hw interface
 *    [ in] wr_data: written data
 *    [ in] wr_len: length of written data in bytes
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_write(struct syna_hw_interface *hw_if,
		unsigned char *wr_data, unsigned int wr_len)
{
	int retval;
	unsigned int idx;
	struct spi_message msg;
	struct spi_device *spi = hw_if->pdev;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;
	struct syna_tcm *tcm = NULL;

	if (!spi) {
		LOGE("Invalid bus io device\n");
		return -ENXIO;
	}

	if (!syna_spi_device) {
		LOGE("Invalid platform device\n");
		return -ENXIO;
	}

	tcm = platform_get_drvdata(syna_spi_device);
	if (!tcm) {
		LOGW("Invalid tcm device\n");
		return -ENXIO;
	}
	syna_pal_mutex_lock(&bus->io_mutex);
/*#ifdef TOUCH_SENSORHUB_SUPPORT*/
#if 0
	retval = pm_runtime_get_sync(spi->master->dev.parent);
	if (retval < 0) {
		LOGE("Fail to failed to get sync, retval = %d\n", retval);
	}
#endif
	pm_stay_awake(&syna_spi_device->dev);
	if (tcm->tp_pm_suspend) {
		LOGI("Touch is in pm_suspend status!\n");
		retval = wait_for_completion_timeout(&tcm->pm_resume_completion, msecs_to_jiffies(500));
		if (!retval) {
			retval = -EINVAL;
			LOGE("wait_for_completion_timeout!\n");
			goto exit;
		}
	}

	if ((wr_len & 0xffff) == 0xffff) {
		LOGE("Invalid write length 0x%X\n", (wr_len & 0xffff));
		retval = -EINVAL;
		goto exit;
	}

	spi_message_init(&msg);

	if (bus->spi_byte_delay_us == 0)
		retval = syna_spi_alloc_mem(1, wr_len);
	else
		retval = syna_spi_alloc_mem(wr_len, wr_len);
	if (retval < 0) {
		LOGE("Failed to allocate memory\n");
		goto exit;
	}

	retval = syna_pal_mem_cpy(tx_buf, wr_len, wr_data, wr_len, wr_len);
	if (retval < 0) {
		LOGE("Fail to copy wr_data to tx_buf\n");
		goto exit;
	}

	if (bus->spi_byte_delay_us == 0) {
		xfer[0].len = wr_len;
		xfer[0].tx_buf = tx_buf;
#ifndef SPI_NO_DELAY_USEC
		if (bus->spi_block_delay_us)
			xfer[0].delay_usecs = bus->spi_block_delay_us;
#endif
		spi_message_add_tail(&xfer[0], &msg);
	} else {
		for (idx = 0; idx < wr_len; idx++) {
			xfer[idx].len = 1;
			xfer[idx].tx_buf = &tx_buf[idx];
#ifndef SPI_NO_DELAY_USEC
			xfer[idx].delay_usecs = bus->spi_byte_delay_us;
			if (bus->spi_block_delay_us && (idx == wr_len - 1))
				xfer[idx].delay_usecs = bus->spi_block_delay_us;
#endif
			spi_message_add_tail(&xfer[idx], &msg);
		}
	}

	retval = spi_sync(spi, &msg);
	if (retval != 0) {
		LOGE("Fail to complete SPI transfer, error = %d\n", retval);
		goto exit;
	}

	syna_print_xfer_data(wr_data, wr_len, SYNA_SPI_TRANSFER_WRITE);

	retval = wr_len;

exit:
	pm_relax(&syna_spi_device->dev);
/*#ifdef TOUCH_SENSORHUB_SUPPORT*/
#if 0
	retval = pm_runtime_put_sync(spi->master->dev.parent);
	if (retval < 0) {
		LOGE("Fail to failed to put sync, retval = %d\n", retval);
	}
#endif
	syna_pal_mutex_unlock(&bus->io_mutex);

	return retval;
}

/**
 * syna_spi_hw_reset()
 *
 * Toggle the hardware gpio pin to perform the chip reset
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *     none.
 */
static void syna_spi_hw_reset(struct syna_hw_interface *hw_if, int reset_delay_ms)
{
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;

	if (rst->reset_gpio == 0)
		return;

	LOGI("Prepare to toggle reset, hold:%d delay:%d\n",
		rst->reset_active_ms, (reset_delay_ms != 0) ? reset_delay_ms : rst->reset_delay_ms);

	gpio_set_value(rst->reset_gpio, (rst->reset_on_state & 0x01));
	syna_pal_sleep_ms(rst->reset_active_ms);
	gpio_set_value(rst->reset_gpio, ((!rst->reset_on_state) & 0x01));
	if (reset_delay_ms != 0)
		syna_pal_sleep_ms(reset_delay_ms);
	else
		syna_pal_sleep_ms(rst->reset_delay_ms);
}


/**
 * syna_spi_enable_pwr_gpio()
 *
 * Helper to enable power supply through GPIO
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] en:    '1' for enabling, and '0' for disabling
 *
 * @return
 *    none
 */
static int syna_spi_enable_pwr_gpio(struct syna_hw_interface *hw_if,
		bool en)
{
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	int state = (en) ? pwr->power_on_state : !pwr->power_on_state;

	if (pwr->avdd_gpio > 0)
		gpio_set_value(pwr->avdd_gpio, state);

	if (pwr->vdd_gpio > 0)
		gpio_set_value(pwr->vdd_gpio, state);

	return 0;
}

/**
 * syna_tcm_pinctrl_init()
 *
 * init the pinctrl
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_tcm_pinctrl_init(struct syna_hw_interface *hw_if)
{
	int retval = 0;
	struct spi_device *spi = hw_if->pdev;
	struct syna_hw_pct_data *pct = &hw_if->bdata_pct;

	/* Get pinctrl if target uses pinctrl */
	pct->pinctrl = devm_pinctrl_get(&spi->dev);
	if (IS_ERR_OR_NULL(pct->pinctrl)) {
		retval = PTR_ERR(pct->pinctrl);
		LOGE("Cannot find pinctrl, ret = %d!\n",retval);
		goto err_pinctrl_get;
	}

	pct->pinctrl_state_active
	    = pinctrl_lookup_state(pct->pinctrl, PINCTRL_STATE_ACTIVE);
	if (IS_ERR_OR_NULL(pct->pinctrl_state_active)) {
		retval = PTR_ERR(pct->pinctrl_state_active);
		LOGE("Cannot find %s pinctrl, ret = %d!\n",PINCTRL_STATE_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	pct->pinctrl_state_suspend
	    = pinctrl_lookup_state(pct->pinctrl, PINCTRL_STATE_SUSPEND);
	if (IS_ERR_OR_NULL(pct->pinctrl_state_suspend)) {
		retval = PTR_ERR(pct->pinctrl_state_suspend);
		LOGE("Cannot find %s pinctrl, ret = %d!\n",PINCTRL_STATE_SUSPEND, retval);
		goto err_pinctrl_lookup;
	}

	LOGI("pinctrl init success");
	return 0;
err_pinctrl_lookup:
	devm_pinctrl_put(pct->pinctrl);
err_pinctrl_get:
	pct->pinctrl = NULL;
	return retval;
}

/**
 * syna_spi_enable_regulator()
 *
 * Enable or disable the regulator
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] en:    '1' for enabling, and '0' for disabling
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_enable_regulator(struct syna_hw_interface *hw_if,
		bool en)
{
	int retval;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	struct regulator *vdd_reg = pwr->vdd_reg_dev;
	struct regulator *avdd_reg = pwr->avdd_reg_dev;

	if (!en) {
		retval = 0;
		goto disable_pwr_reg;
	}

	if (avdd_reg) {
		retval = regulator_enable(avdd_reg);
		if (retval < 0) {
			LOGE("Fail to enable avdd regulator\n");
			goto disable_avdd_reg;
		}
	}

	usleep_range(3000, 3100);

	if (vdd_reg) {
		retval = regulator_enable(vdd_reg);
		if (retval < 0) {
			LOGE("Fail to enable vdd regulator\n");
			goto exit;
		}
	}

	return 0;

disable_pwr_reg:
	if (vdd_reg) {
		regulator_disable(vdd_reg);
		usleep_range(3000, 3100);
	}
disable_avdd_reg:
	if (avdd_reg)
		regulator_disable(avdd_reg);
exit:
	return retval;
}

/**
 * syna_spi_power_on()
 *
 * Power on touch controller through regulators or gpios for PWM
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] en:    '1' for powering on, and '0' for powering off
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_power_on(struct syna_hw_interface *hw_if,
		bool en)
{
	int retval;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	LOGD("Prepare to power %s device through %s ...\n",
		(en) ? "on" : "off",
		(pwr->psu == PSU_REGULATOR) ? "regulator" : "gpio control");

	if (pwr->psu == PSU_REGULATOR)
		retval = syna_spi_enable_regulator(hw_if, en);
	else
		retval = syna_spi_enable_pwr_gpio(hw_if, en);

	if (retval < 0) {
		LOGE("Fail to power %s device\n", (en) ? "on" : "off");
		return retval;
	}

	syna_pal_sleep_ms(pwr->power_delay_ms);

	LOGI("Device power %s\n", (en) ? "on" : "off");

	return 0;
}

/**
 * syna_spi_get_regulator()
 *
 * Acquire or release the regulator
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] get:   '1' for getting the regulator, and '0' for removing
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_get_regulator(struct syna_hw_interface *hw_if,
		bool get)
{
	int retval;
	struct spi_device *spi = hw_if->pdev;
	struct device *dev = &spi->dev;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (!get) {
		retval = 0;
		goto regulator_put;
	}

	if (pwr->vdd_reg_name != NULL && *pwr->vdd_reg_name != 0) {
#ifdef DEV_MANAGED_API
		pwr->vdd_reg_dev = devm_regulator_get(dev, pwr->vdd_reg_name);
#else /* Legacy API */
		pwr->vdd_reg_dev = regulator_get(dev, pwr->vdd_reg_name);
#endif
		if (IS_ERR((struct regulator *)pwr->vdd_reg_dev)) {
			LOGW("Vdd regulator is not ready\n");
			retval = PTR_ERR((struct regulator *)pwr->vdd_reg_dev);
			goto exit;
		}
	}

	if (pwr->avdd_reg_name != NULL && *pwr->avdd_reg_name != 0) {
#ifdef DEV_MANAGED_API
		pwr->avdd_reg_dev = devm_regulator_get(dev, pwr->avdd_reg_name);
#else /* Legacy API */
		pwr->avdd_reg_dev = regulator_get(dev, pwr->avdd_reg_name);
#endif
		if (IS_ERR((struct regulator *)pwr->avdd_reg_dev)) {
			LOGW("AVdd regulator is not ready\n");
			retval = PTR_ERR((struct regulator *)pwr->avdd_reg_dev);
			goto regulator_vdd_put;
		}
	}

	return 0;

regulator_put:
	if (pwr->vdd_reg_dev) {
#ifdef DEV_MANAGED_API
		devm_regulator_put(pwr->vdd_reg_dev);
#else /* Legacy API */
		regulator_put(pwr->vdd_reg_dev);
#endif
		pwr->vdd_reg_dev = NULL;
	}
regulator_vdd_put:
	if (pwr->avdd_reg_dev) {
#ifdef DEV_MANAGED_API
		devm_regulator_put(pwr->avdd_reg_dev);
#else /* Legacy API */
		regulator_put(pwr->avdd_reg_dev);
#endif
		pwr->avdd_reg_dev = NULL;
	}
exit:
	return retval;
}

/**
 * syna_i2c_config_psu()
 *
 * Initialize the power supply unit
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_config_psu(struct syna_hw_interface *hw_if)
{
	int retval;
	static char str_vdd_gpio[32] = {0};
	static char str_avdd_gpio[32] = {0};
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (pwr->psu != PSU_REGULATOR) {
		/* set up power gpio */
		if (pwr->vdd_gpio > 0) {
			retval = syna_spi_request_gpio(pwr->vdd_gpio,
					true, 1, !pwr->power_on_state,
					str_vdd_gpio);
			if (retval < 0) {
				LOGE("Fail to configure vdd GPIO %d\n",
					pwr->vdd_gpio);
				return retval;
			}
		}

		if (pwr->avdd_gpio > 0) {
			retval = syna_spi_request_gpio(pwr->avdd_gpio,
					true, 1, !pwr->power_on_state,
					str_avdd_gpio);
			if (retval < 0) {
				LOGE("Fail to configure avdd GPIO %d\n",
					pwr->avdd_gpio);

				syna_spi_request_gpio(pwr->vdd_gpio,
					false, 0, 0, NULL);
				return retval;
			}
		}
	} else {
		/* set up regulator */
		retval = syna_spi_get_regulator(hw_if, true);
		if (retval < 0) {
			LOGE("Fail to configure regulators\n");
			return retval;
		}
	}

	return 0;
}

/**
 * syna_spi_release_psu()
 *
 * Release the power supply unit
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_release_psu(struct syna_hw_interface *hw_if)
{
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (pwr->psu != PSU_GPIO) {
		if (pwr->avdd_gpio > 0)
			syna_spi_request_gpio(pwr->avdd_gpio, false, 0, 0, NULL);
		if (pwr->vdd_gpio > 0)
			syna_spi_request_gpio(pwr->vdd_gpio, false, 0, 0, NULL);
	} else {
		syna_spi_get_regulator(hw_if, false);
	}

	return 0;
}

/**
 * syna_spi_enable_irq()
 *
 * Enable or disable the handling of interrupt
 *
 * @param
 *    [ in] hw_if: the handle of hw interface
 *    [ in] en:    '1' for enabling, and '0' for disabling
 *
 * @return
 *    0 on success; otherwise, on error.
 */
static int syna_spi_enable_irq(struct syna_hw_interface *hw_if,
		bool en)
{
	int retval = 0;
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct irq_desc *desc;

	if (attn->irq_id == 0)
		return 0;

	desc = irq_to_desc(attn->irq_id);
	syna_pal_mutex_lock(&attn->irq_en_mutex);

	/* enable the handling of interrupt */
	if (en) {
		if (attn->irq_enabled) {
			LOGD("Interrupt already enabled\n");
			retval = 0;
			goto exit;
		}
		while (desc->depth > 1) {
			LOGI("depth unbalance, enable irq again\n");
			enable_irq(attn->irq_id);
		}
		enable_irq(attn->irq_id);
		attn->irq_enabled = true;

		LOGI("Interrupt enabled\n");
	}
	/* disable the handling of interrupt */
	else {
		if (!attn->irq_enabled) {
			LOGD("Interrupt already disabled\n");
			retval = 0;
			goto exit;
		}

		disable_irq_nosync(attn->irq_id);
		attn->irq_enabled = false;

		LOGI("Interrupt disabled\n");
	}

exit:
	syna_pal_mutex_unlock(&attn->irq_en_mutex);

	return retval;
}
/**
 * syna_hw_interface
 *
 * Provide the hardware specific settings in defaults.
 * Be noted the followings could be changed after .dtsi is parsed
 */
static struct syna_hw_interface syna_spi_hw_if = {
	.bdata_io = {
		.type = BUS_TYPE_SPI,
		.rd_chunk_size = RD_CHUNK_SIZE,
		.wr_chunk_size = WR_CHUNK_SIZE,
	},
	.bdata_attn = {
		.irq_enabled = false,
		.irq_on_state = 0,
	},
	.bdata_rst = {
		.reset_on_state = 0,
		.reset_delay_ms = 200,
		.reset_active_ms = 20,
	},
	.bdata_pwr = {
		.power_on_state = 1,
		.power_delay_ms = 200,
	},
	.ops_power_on = syna_spi_power_on,
	.ops_hw_reset = syna_spi_hw_reset,
	.ops_read_data = syna_spi_read,
	.ops_write_data = syna_spi_write,
	.ops_enable_irq = syna_spi_enable_irq,
	.ops_config_gpio = syna_spi_config_gpios,
};

/**
 * syna_spi_probe()
 *
 * Prepare the specific hardware interface and register the platform spi device
 *
 * @param
 *    [ in] spi: spi device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int syna_spi_probe(struct spi_device *spi)
{
	int retval;
	struct syna_hw_attn_data *attn = &syna_spi_hw_if.bdata_attn;
	struct syna_hw_bus_data *bus = &syna_spi_hw_if.bdata_io;
	struct syna_hw_pct_data *pct = &syna_spi_hw_if.bdata_pct;

	if (spi->master->flags & SPI_MASTER_HALF_DUPLEX) {
		LOGE("Full duplex not supported by host\n");
		return -EIO;
	}

	/* allocate an spi platform device */
	syna_spi_device = platform_device_alloc(PLATFORM_DRIVER_NAME, 0);
	if (!syna_spi_device) {
		LOGE("Fail to allocate platform device\n");
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	retval = syna_spi_parse_dt(&syna_spi_hw_if, &spi->dev);
	if (retval < 0) {
		LOGE("Fail to parse dt\n");
		return -EIO;

	}
#endif

	syna_pal_mutex_alloc(&attn->irq_en_mutex);
	syna_pal_mutex_alloc(&bus->io_mutex);

	switch (bus->spi_mode) {
	case 0:
		spi->mode = SPI_MODE_0;
		break;
	case 1:
		spi->mode = SPI_MODE_1;
		break;
	case 2:
		spi->mode = SPI_MODE_2;
		break;
	case 3:
		spi->mode = SPI_MODE_3;
		break;
	}

#ifdef TOUCH_SPI_CS_CLK_DELAY
	spi->controller_data = &qcom_ctrl_data;
#endif
	/* keep the i/o device */
	syna_spi_hw_if.pdev = spi;

	/*syna_spi_device->dev.parent = &spi->dev;*/
	syna_spi_device->dev.platform_data = &syna_spi_hw_if;

	spi->bits_per_word = 8;

	/* set up spi driver */
	retval = spi_setup(spi);
	if (retval < 0) {
		LOGE("Fail to set up SPI protocol driver\n");
		return retval;
	}

	/* initialize pinctrl */
	retval = syna_tcm_pinctrl_init(&syna_spi_hw_if);
	if (!retval && pct->pinctrl) {
		retval =
		    pinctrl_select_state(pct->pinctrl,
					 pct->pinctrl_state_active);
		if (retval < 0)
			LOGE("Failed to select %s pinstate %d\n", PINCTRL_STATE_ACTIVE, retval);
	} else
		LOGE("Failed to init pinctrl\n");

	/* initialize power unit */
	retval = syna_spi_config_psu(&syna_spi_hw_if);
	if (retval < 0) {
		LOGE("Fail to config power unit\n");
		return retval;
	}

	/* do i/o switch if defined */
	if (bus->switch_gpio >= 0)
		gpio_set_value(bus->switch_gpio, bus->switch_state);

	/* register the spi platform device */
	retval = platform_device_add(syna_spi_device);
	if (retval < 0) {
		LOGE("Fail to add platform device\n");
		return retval;
	}

	return 0;
}

/**
 * syna_spi_remove()
 *
 * Unregister the platform spi device
 *
 * @param
 *    [ in] spi: spi device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static void syna_spi_remove(struct spi_device *spi)
{
	struct syna_hw_attn_data *attn = &syna_spi_hw_if.bdata_attn;
	struct syna_hw_bus_data *bus = &syna_spi_hw_if.bdata_io;

	/* release gpios */
	syna_spi_free_gpios(&syna_spi_hw_if);

	/* disable the regulators */
	syna_spi_release_psu(&syna_spi_hw_if);

	/* release mutex */
	syna_pal_mutex_free(&attn->irq_en_mutex);
	syna_pal_mutex_free(&bus->io_mutex);

	/* remove the platform device */
	syna_spi_device->dev.platform_data = NULL;
	platform_device_unregister(syna_spi_device);
}

/**
 * Describe an spi device driver and its related declarations
 */
static const struct spi_device_id syna_spi_id_table[] = {
	{SPI_MODULE_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(spi, syna_spi_id_table);

#ifdef CONFIG_OF
static const struct of_device_id syna_spi_of_match_table[] = {
	{
		.compatible = "synaptics,tcm-spi",
	},
	{},
};
MODULE_DEVICE_TABLE(of, syna_spi_of_match_table);
#else
#define syna_spi_of_match_table NULL
#endif

static struct spi_driver syna_spi_driver = {
	.driver = {
		.name = SPI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = syna_spi_of_match_table,
	},
	.probe = syna_spi_probe,
	.remove = syna_spi_remove,
	.id_table = syna_spi_id_table,
};


/**
 * syna_hw_interface_init()
 *
 * Initialize the lower-level hardware interface module.
 * After returning, the handle of hw interface should be ready.
 *
 * @param
 *    void
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
int syna_hw_interface_init(void)
{
	return spi_register_driver(&syna_spi_driver);
}

/**
 * syna_hw_interface_exit()
 *
 * Delete the lower-level hardware interface module
 *
 * @param
 *    void
 *
 * @return
 *    none.
 */
void syna_hw_interface_exit(void)
{
	if (rx_buf) {
		syna_pal_mem_free((void *)rx_buf);
		rx_buf = NULL;
	}

	if (tx_buf) {
		syna_pal_mem_free((void *)tx_buf);
		tx_buf = NULL;
	}

	if (xfer) {
		syna_pal_mem_free((void *)xfer);
		xfer = NULL;
	}

	spi_unregister_driver(&syna_spi_driver);
}

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TouchCom SPI Bus Module");
MODULE_LICENSE("GPL v2");

