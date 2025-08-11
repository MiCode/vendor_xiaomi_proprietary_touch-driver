#ifndef __GOODIX_XIAOMI_DRIVER_H__
#define __GOODIX_XIAOMI_DRIVER_H__

#define SEC_TOUCH_ID	(1)
#define GTP_RESULT_INVALID				0
#define GTP_RESULT_FAIL					1
#define GTP_RESULT_PASS					2
#define GOODIX_NFC_CMD	0x34
#define NFC_OFF_EVENT	0x00
#define NFC_ON_NOISE_END_EVENT	0x01
#define NFC_NOISE_START_EVENT	0x03
#define INPUT_METHOD_CMD	0x35

typedef struct xiaomi_pdata {
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
	char config_file_name[64];
	char fw_image_name[64];
	char test_limit_name[64];
} xiaomi_bdata_t;
#endif
