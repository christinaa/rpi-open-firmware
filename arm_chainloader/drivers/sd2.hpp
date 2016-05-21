#pragma once

/* Error Code */
#define	SD_NO_ERROR		0x0
#define	SD_CRC_ERR		0x80
#define	SD_TO_ERR		0x40
#define	SD_NO_CARD		0x20
#define SD_BUSY			0x10
#define	SD_STS_ERR		0x08
#define SD_RSP_TIMEOUT		0x04
#define SD_IO_ERR		0x02
/* Return code for MMC switch bus */
#define SWITCH_SUCCESS		0
#define SWITCH_ERR		1
#define SWITCH_FAIL		2
/* MMC/SD Command Index */
/* Basic command (class 0) */
#define GO_IDLE_STATE		0
#define	SEND_OP_COND		1
#define	ALL_SEND_CID		2
#define	SET_RELATIVE_ADDR	3
#define	SEND_RELATIVE_ADDR	3
#define	SET_DSR			4
#define IO_SEND_OP_COND		5
#define	SWITCH			6
#define	SELECT_CARD		7
#define	DESELECT_CARD		7
/* CMD8 is "SEND_EXT_CSD" for MMC4.x Spec
 * while is "SEND_IF_COND" for SD 2.0
 */
#define	SEND_EXT_CSD		8
#define	SEND_IF_COND		8
#define	SEND_CSD		9
#define	SEND_CID		10
#define	VOLTAGE_SWITCH	    	11
#define	READ_DAT_UTIL_STOP	11
#define	STOP_TRANSMISSION	12
#define	SEND_STATUS		13
#define	GO_INACTIVE_STATE	15
#define	SET_BLOCKLEN		16
#define	READ_SINGLE_BLOCK	17
#define	READ_MULTIPLE_BLOCK	18
#define	SEND_TUNING_PATTERN	19
#define	BUSTEST_R		14
#define	BUSTEST_W		19
#define	WRITE_BLOCK		24
#define	WRITE_MULTIPLE_BLOCK	25
#define	PROGRAM_CSD		27
#define	ERASE_WR_BLK_START	32
#define	ERASE_WR_BLK_END	33
#define	ERASE_CMD		38
#define LOCK_UNLOCK 		42
#define	IO_RW_DIRECT		52
#define	APP_CMD			55
#define	GEN_CMD			56
#define	SET_BUS_WIDTH		6
#define	SD_STATUS		13
#define	SEND_NUM_WR_BLOCKS	22
#define	SET_WR_BLK_ERASE_COUNT	23
#define	SD_APP_OP_COND		41
#define	SET_CLR_CARD_DETECT	42
#define	SEND_SCR		51
#define	SD_READ_COMPLETE	0x00
#define	SD_READ_TO		0x01
#define	SD_READ_ADVENCE		0x02
#define	SD_CHECK_MODE		0x00
#define	SD_SWITCH_MODE		0x80
#define	SD_FUNC_GROUP_1	    	0x01
#define	SD_FUNC_GROUP_2	    	0x02
#define	SD_FUNC_GROUP_3	    	0x03
#define	SD_FUNC_GROUP_4	    	0x04
#define	SD_CHECK_SPEC_V1_1	0xFF
#define	NO_ARGUMENT	                        0x00
#define	CHECK_PATTERN	                    	0x000000AA
#define	VOLTAGE_SUPPLY_RANGE	            	0x00000100
#define	SUPPORT_HIGH_AND_EXTENDED_CAPACITY	0x40000000
#define	SUPPORT_MAX_POWER_PERMANCE	        0x10000000
#define	SUPPORT_1V8	                        0x01000000
#define	SWTICH_NO_ERR	  	0x00
#define	CARD_NOT_EXIST	  	0x01
#define	SPEC_NOT_SUPPORT  	0x02
#define	CHECK_MODE_ERR	  	0x03
#define	CHECK_NOT_READY	  	0x04
#define	SWITCH_CRC_ERR	  	0x05
#define	SWITCH_MODE_ERR	  	0x06
#define	SWITCH_PASS		0x07
