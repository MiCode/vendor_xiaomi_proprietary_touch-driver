#ifndef __SYNA_XIAOMI_DRIVER_H__
#define __SYNA_XIAOMI_DRIVER_H__

#include "syna_tcm2.h"

#define SYNAPTICS_DEBUGFS_ENABLE

#ifdef SYNAPTICS_DEBUGFS_ENABLE
#include <linux/debugfs.h>
#endif

#define PRI_TOUCH_ID	(0)

struct synaptics_config_info {
	u8 tp_vendor;
	const char *synaptics_cfg_name;
	const char *synaptics_fw_name;
	const char *synaptics_limit_name;
};

typedef struct {
	bool x_flip;
	bool y_flip;
	int max_x;
	int max_y;
	int fod_lx;
	int fod_ly;
	int fod_x_size;
	int fod_y_size;
	int tx_num;
	int rx_num;
	int special_rx_num;
	int special_tx_num;
	int frame_data_page_size;
	int frame_data_buf_size;
	int raw_data_page_size;
	int raw_data_buf_size;
	bool support_super_resolution;
	size_t config_array_size;
	struct synaptics_config_info *config_array;
	const char *synaptics_default_cfg_name;
	const char *synaptics_default_fw_name;
	const char *synaptics_default_limit_name;
	const char *config_file_name;
	const char *fw_image_name;
	const char *test_limit_name;
} xiaomi_bdata_t;

typedef struct {
	bool lockdown_info_ready;
	char lockdown_info[8];
#ifdef CONFIG_TRUSTED_TOUCH
	struct completion tui_finish;
	bool tui_process;
#endif
} xiaomi_driver_data_t;

int syna_tcm_palm_area_change_setting(int value);
int get_capfold_information(void);
#endif