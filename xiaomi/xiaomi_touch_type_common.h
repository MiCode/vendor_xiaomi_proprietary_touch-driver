#ifndef __XIAOMI_TOUCH_TYPE_COMMON_H__
#define __XIAOMI_TOUCH_TYPE_COMMON_H__

#include <linux/types.h>

#define CMD_DATA_BUF_SIZE	256
#define FOD_VALUE_LEN 5
/*
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;
typedef char s8;
typedef int32_t s32;
*/

enum suspend_state {
	XIAOMI_TOUCH_RESUME = 0,
	XIAOMI_TOUCH_SUSPEND,
	XIAOMI_TOUCH_LP1,
	XIAOMI_TOUCH_LP2,
	XIAOMI_TOUCH_SENSORHUB_ENABLE,
	XIAOMI_TOUCH_SENSORHUB_DISABLE,
	XIAOMI_TOUCH_SENSORHUB_NONUIENABLE,
	XIAOMI_TOUCH_ENABLE_SENSOR = 100,
	XIAOMI_TOUCH_DISABLE_SENSOR = 101,
};

enum THP_IC_MODE_TYPE {
	THP_IC_CMD_BASE                     = 3000,
	IC_MODE_0						= THP_IC_CMD_BASE + 1,
	IC_MODE_1						= THP_IC_CMD_BASE + 2,
	IC_MODE_2						= THP_IC_CMD_BASE + 3,
	IC_MODE_3						= THP_IC_CMD_BASE + 4,
	IC_MODE_4						= THP_IC_CMD_BASE + 5,
	IC_MODE_5						= THP_IC_CMD_BASE + 6,
	IC_MODE_6						= THP_IC_CMD_BASE + 7,
	IC_MODE_7						= THP_IC_CMD_BASE + 8,
	IC_MODE_8						= THP_IC_CMD_BASE + 9,
	IC_MODE_9						= THP_IC_CMD_BASE + 10,
	IC_MODE_10						= THP_IC_CMD_BASE + 11,
	IC_MODE_11						= THP_IC_CMD_BASE + 12,
	IC_MODE_12						= THP_IC_CMD_BASE + 13,
	IC_MODE_13						= THP_IC_CMD_BASE + 14,
	IC_MODE_14						= THP_IC_CMD_BASE + 15,
	IC_MODE_15						= THP_IC_CMD_BASE + 16,
	IC_MODE_16						= THP_IC_CMD_BASE + 17,
	IC_MODE_17						= THP_IC_CMD_BASE + 18,
	IC_MODE_18						= THP_IC_CMD_BASE + 19,
	IC_MODE_19						= THP_IC_CMD_BASE + 20,
	IC_MODE_20						= THP_IC_CMD_BASE + 21,
	IC_MODE_21						= THP_IC_CMD_BASE + 22,
	IC_MODE_22						= THP_IC_CMD_BASE + 23,
	IC_MODE_23						= THP_IC_CMD_BASE + 24,
	IC_MODE_24						= THP_IC_CMD_BASE + 25,
	IC_MODE_25						= THP_IC_CMD_BASE + 26,
	IC_MODE_26						= THP_IC_CMD_BASE + 27,
	IC_MODE_27						= THP_IC_CMD_BASE + 28,
	IC_MODE_28						= THP_IC_CMD_BASE + 29,
	IC_MODE_29						= THP_IC_CMD_BASE + 30,
	IC_MODE_30						= THP_IC_CMD_BASE + 31,
	IC_MODE_31						= THP_IC_CMD_BASE + 32,
	IC_MODE_32						= THP_IC_CMD_BASE + 33,
	IC_MODE_33						= THP_IC_CMD_BASE + 34,
	IC_MODE_34						= THP_IC_CMD_BASE + 35,
	IC_MODE_35						= THP_IC_CMD_BASE + 36,
	IC_MODE_36						= THP_IC_CMD_BASE + 37,
	IC_MODE_37						= THP_IC_CMD_BASE + 38,
	IC_MODE_38						= THP_IC_CMD_BASE + 39,
	IC_MODE_39						= THP_IC_CMD_BASE + 40,
	IC_MODE_40						= THP_IC_CMD_BASE + 41,
	IC_MODE_41						= THP_IC_CMD_BASE + 42,
	IC_MODE_42						= THP_IC_CMD_BASE + 43,
	IC_MODE_43						= THP_IC_CMD_BASE + 44,
	IC_MODE_44						= THP_IC_CMD_BASE + 45,
	IC_MODE_45						= THP_IC_CMD_BASE + 46,
	IC_MODE_46						= THP_IC_CMD_BASE + 47,
	IC_MODE_47						= THP_IC_CMD_BASE + 48,
	IC_MODE_48		 				= THP_IC_CMD_BASE + 49,
	IC_MODE_49						= THP_IC_CMD_BASE + 50,
	IC_MODE_50						= THP_IC_CMD_BASE + 51,
	IC_MODE_51						= THP_IC_CMD_BASE + 52,
	IC_MODE_52						= THP_IC_CMD_BASE + 53,
	IC_MODE_53						= THP_IC_CMD_BASE + 54,
	IC_MODE_54						= THP_IC_CMD_BASE + 55,
	IC_MODE_55						= THP_IC_CMD_BASE + 56,
	IC_MODE_56						= THP_IC_CMD_BASE + 57,
	IC_MODE_57						= THP_IC_CMD_BASE + 58,
	IC_MODE_58						= THP_IC_CMD_BASE + 59,
};

enum ioctl_cmd {
	COMMON_DATA_CMD = 0,
	HARDWARE_PARAM_CMD,
	SELECT_MMAP_CMD,
	SELECT_TOUCH_ID,
	GET_FRAME_DATA_INDEX,
	RAW_DATA_INDEX,
	UPDATE_REPORT_POINT,
};

enum common_data_cmd {
	SET_CUR_VALUE = 0,
	GET_CUR_VALUE,
	GET_DEF_VALUE,
	GET_MIN_VALUE,
	GET_MAX_VALUE,
	GET_MODE_VALUE,
	RESET_MODE,
	SET_LONG_VALUE,
	SET_THP_IC_CUR_VALUE,
	GET_THP_IC_CUR_VALUE,
	SET_CMD_FOR_THP,
	SET_CMD_FOR_DRIVER,
};

enum common_data_mode {
	DATA_MODE_0					= 0,
	DATA_MODE_1					= 1,
	DATA_MODE_2					= 2,
	DATA_MODE_3					= 3,
	DATA_MODE_4					= 4,
	DATA_MODE_5					= 5,
	DATA_MODE_6					= 6,
	DATA_MODE_7					= 7,
	DATA_MODE_8					= 8,
	DATA_MODE_9					= 9,
	DATA_MODE_10				= 10,
	DATA_MODE_11				= 11,
	DATA_MODE_12				= 12,
	DATA_MODE_13				= 13,
	DATA_MODE_14				= 14,
	DATA_MODE_15				= 15,
	DATA_MODE_16				= 16,
	DATA_MODE_17				= 17,
	DATA_MODE_18				= 18,
	DATA_MODE_19				= 19,
	DATA_MODE_20				= 20,
	DATA_MODE_21				= 21,
	DATA_MODE_22				= 22,
	DATA_MODE_23				= 23,
	DATA_MODE_24				= 24,
	DATA_MODE_25				= 25,
	DATA_MODE_26				= 26,
	DATA_MODE_27				= 27,
	DATA_MODE_28				= 28,
	DATA_MODE_29        	    = 29,/* 8650接收29用+105转发 8750可以直接转发29*/
	DATA_MODE_30				= 30,
	DATA_MODE_31				= 31,
	DATA_MODE_32				= 32,
	DATA_MODE_33                = 33,
	DATA_MODE_34			    = 34,
	DATA_MODE_35				= 35,
	DATA_MODE_41				= 100,
	DATA_MODE_42				= 101,
	DATA_MODE_43				= 102,
  	DATA_MODE_44 				= 103,
	DATA_MODE_45             	= 200,
	DATA_MODE_46                = DATA_MODE_45 + 0,
	DATA_MODE_47               	= DATA_MODE_45 + 1,
	DATA_MODE_48            	= DATA_MODE_45 + 2,
	DATA_MODE_49          		= DATA_MODE_45 + 3,
	DATA_MODE_50                = DATA_MODE_45 + 4,
	DATA_MODE_51              	= DATA_MODE_45 + 5,
	DATA_MODE_52             	= DATA_MODE_45 + 6,
	DATA_MODE_53        		= 10000,
	DATA_MODE_54    			= DATA_MODE_53 + 1,
	DATA_MODE_55    			= DATA_MODE_53 + 2,
	DATA_MODE_56     			= DATA_MODE_53 + 3,
	DATA_MODE_57    			= DATA_MODE_53 + 4,
	DATA_MODE_58     			= DATA_MODE_53 + 5,
	DATA_MODE_59,
	DATA_MODE_60    	 		= DATA_MODE_53 + 100,
	DATA_MODE_61  				= DATA_MODE_53 + 101,
	DATA_MODE_62 				= DATA_MODE_53 + 102,
	DATA_MODE_63 				= DATA_MODE_53 + 103,
	DATA_MODE_64 				= DATA_MODE_53 + 104,
	DATA_MODE_65 				= DATA_MODE_53 + 105,
	DATA_MODE_66 				= DATA_MODE_53 + 106,
	DATA_MODE_67     			= DATA_MODE_53 + 110,
	DATA_MODE_68 				= DATA_MODE_53 + 107,
	DATA_MODE_69    			= DATA_MODE_53 + 108,
	DATA_MODE_70 				= DATA_MODE_53 + 109,
	DATA_MODE_71,
	DATA_MODE_72          		= 20000,
	DATA_MODE_73             	= DATA_MODE_72 + 38,
};

typedef struct common_data {
	s8 touch_id;
	u8 cmd;
	u16 mode;
	u16 data_len;
	s32 data_buf[CMD_DATA_BUF_SIZE];
} common_data_t;

typedef struct hardware_param {
	u16 x_resolution;
	u16 y_resolution;
	u16 rx_num;
	u16 tx_num;
	u8 super_resolution_factor;
	u8 frame_data_page_size;
	u8 frame_data_buf_size;
	u8 raw_data_page_size;
	u8 raw_data_buf_size;
	u8 lockdown_info[8];
	char config_file_name[64];
	char driver_version[64];
	char fw_version[64];
	u8 temp_change_value;
} hardware_param_t;

enum touch_dump_type {
	DUMP_OFF = 0,
	DUMP_ON = 1,
	DUMP_BASE = 100,
};

#endif
