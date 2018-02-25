#ifndef _KIONIX_KTXE9_DRIVER_H_
#define _KIONIX_KTXE9_DRIVER_H_
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>


/*==================================================================================================

     Header Name: Kionix_ktxe9_driver.h

     General Description: Function prototypes for an accelerometer with an I2C interface.

     Driver Guide Document:  Rev 1.2 (3/31/2010)
====================================================================================================

Revision History:
                            Modification     Tracking
Author                          Date          Number     Description of Changes
-------------------------   ------------    ----------   -------------------------------------------
Calvin Ong					 03/31/2010                   Include initialization structures
Calvin Ong                   02/01/2010                   Correct code errors
Calvin Ong                   04/20/2009                   Setup for Kionix Driver interface



====================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

//kxte9 register map

/* Read only Registers*/
#define KXTE9_I2C_SLV_ADDR 		0x0F
#define KXTE9_I2C_ST_RESP	 	0x0C
#define KXTE9_I2C_WHO_AM_I	 	0x0F
#define KXTE9_I2C_TILT_POS_CUR	0x10
#define KXTE9_I2C_TILT_POS_PRE	0x11
#define KXTE9_I2C_X_OUT			0x12
#define KXTE9_I2C_Y_OUT			0x13
#define KXTE9_I2C_Z_OUT			0x14
#define KXTE9_I2C_INT_SRC_REG1	0x16
#define KXTE9_I2C_INT_SRC_REG2	0x17
#define KXTE9_I2C_STATUS_REG	0x18
#define KXTE9_I2C_INT_REL		0x1A

/*Read-write Control Registers */
#define KXTE9_I2C_CTRL_REG1		0x1B
#define KXTE9_I2C_CTRL_REG2		0x1C
#define KXTE9_I2C_CTRL_REG3		0x1D
#define KXTE9_I2C_INT_CTRL_REG1	0x1E
#define KXTE9_I2C_INT_CTRL_REG2	0x1F
#define KXTE9_I2C_TILT_TIMER	0x28
#define KXTE9_I2C_WUF_TIMER		0x29
#define KXTE9_I2C_B2S_TIMER		0x2A
#define KXTE9_I2C_WUF_THRESH	0x5A
#define KXTE9_I2C_B2S_THRESH	0x5B
#define KXTE9_I2C_TILT_ANGLE    0x5C
#define KXTE9_I2C_HYST_SET      0x5F

#define CTRL_REG1_TPE			0x01	/* enables tilt position */
#define CTRL_REG1_WUFE			0x02	/* enables wake up function */
#define CTRL_REG1_B2SE			0x04	/* enables back to sleep function */
#define CTRL_REG1_ODRB			0x08	/* initial output data rate */
#define CTRL_REG1_ODRA			0x10	/* initial output data rate */
#define CTRL_REG1_BIT5          0x20
#define CTRL_REG1_PC1			0x80	/* operatin mode 1=full pwer mode; 0=stand by */

#define CTRL_REG2_FUM			0x01	/* face up state mask */
#define CTRL_REG2_FDM			0x02	/* face down state mask */
#define CTRL_REG2_UPM			0x04	/* up state mask */
#define CTRL_REG2_DOM			0x08	/* down state mask */
#define CTRL_REG2_RIM			0x10	/* right state mask */
#define CTRL_REG2_LEM			0x20	/* left state mask */

#define CTRL_REG3_OWUFB			0x01	/* active mode output data rate */
#define CTRL_REG3_OWUFA			0x02	/* active mode output data rate */
#define CTRL_REG3_OB2SB			0x04	/* inactive mode output data rate */
#define CTRL_REG3_OB2SA			0x08	/* inactive mode output data rate */
#define CTRL_REG3_STC			0x10	/* self-test function */
#define CTRL_REG3_SRST			0x80	/* software reset */

#define INT_CTRL_REG1_IEL		0x04 	/* sets the respnse of physical interrupt pin, latched or pulse */
#define INT_CTRL_REG1_IEA		0x08	/* polarity of the physical interrupt pin */
#define INT_CTRL_REG1_IEN		0x10	/* enables physical interrupt */

#define INT_CTRL_REG2_ZBW		0x20	/* Z axis mask */
#define INT_CTRL_REG2_YBW		0x40	/* Y axis mask */
#define INT_CTRL_REG2_XBW		0x80	/* X axis mask */

#define FULL_SCALE_RANGE_2_G    2000    /* indicates full scale g-range of the kxte9 */
#define BIT_SENSITIVITY_2_G     16      /* indicates sensitivity of the kxte9 ((2^6)/4) */
#define ZERO_G_OFFSET 		    32      /* indicates 0g offset of the kxte9 ((2^6)/2) */


/*==================================================================================================
                                            MACROS
==================================================================================================*/
#define SET_REG_BIT(r,b) r |= b
#define UNSET_REG_BIT(r,b) r &= ~b

// path to device node to talk to iic driver at kernel level
#define MOTION_SENSOR_DEV_NODE "/dev/gpio_i2c"

/*==================================================================================================
                                           DATA TYPES
==================================================================================================*/
//developer note: Need to change data types names to suit your own system
typedef char                        CHAR;       /* 8 bit character                  */
typedef unsigned char              UINT8;      /* Unsigned  8 bit quantity         */
typedef unsigned char              INT8U;      /* Unsigned  8 bit quantity         */

typedef unsigned short int         UINT16;     /* Unsigned  16 bit quantity        */
typedef signed short int           INT16;      /* Signed    16 bit quantity        */
typedef unsigned int               UINT32;     /* Unsigned  32 bit quantity        */
typedef signed int                 INT32;      /* Signed    32 bit quantity        */

typedef enum {FALSE = 0, TRUE} BOOLEAN;

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

#define DISABLE					0		/* disabled mode */
#define ENABLE					1		/* enabled mode */

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
// structure that holds the parameters for I2C communiation
typedef struct
{
    INT8U slave_addr;
    INT8U length;
    INT8U slave_reg_addr;
    INT8U *data_ptr;

} i2c_rw_cmd_type;


typedef struct
{
    int dir_mask_enable;
    int axis_mask_enable;
    int mask_le;
    int mask_ri;
    int mask_do;
    int mask_up;
    int mask_fd;
    int mask_fu;
    int x_mask;
    int y_mask;
    int z_mask;

} MASKINFO;




/* Interrupt control */
typedef struct
{
    int enable;		//1 = enable
    int polarity;	//1 = active high
    int latch;		//1 = unlatched
} INTINFO;





/* TILT_INFO structure to setup TILT function */
typedef struct
{
    int enable;		//1=enable
    int ODR;		//ODR rate 1Hz, 3Hz, 10Hz, 40Hz
    int timer;		// Tilt count
    int angle;		//device Orientation Angle
    int hysteresis; //Hysteresis
} TILTINFO;

/* WUP_INFO structure to setup Wake Up function */
typedef struct
{
    int enable;		//1=enable
    int ODR;		//ODR rate 1Hz, 3Hz, 10Hz, 40Hz
    int timer;		//Wake-up timer. Every count is output at a  delay period of (1/ODR)
    int thresh;		//Wake-up threshold to detect the change in g (inactivity to activity)
} WUFINFO;


/* B2S_INFO structure to setup Back to Sleep function */
typedef struct
{
    int enable;		//1=enable
    int ODR;		//ODR rate 1Hz, 3Hz, 10Hz, 40Hz
    int timer;		//Back-to-Sleep timer. Every count is output at a  delay period of (1/ODR)
    int thresh;		//Back-to-Sleep threshold to detect the change in g (activity to inactivity)
} B2SINFO;



typedef struct
{
    TILTINFO tiltinfo;
    WUFINFO  wufinfo;
    B2SINFO  b2sinfo;
    INTINFO  intinfo;
    MASKINFO maskinfo;

} KXTE9_INFO;

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
extern int g_i2c_handle;

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/
BOOLEAN kxte9_read_bytes(UINT8 reg, UINT8 *data, UINT8 length);
BOOLEAN kxte9_write_byte(UINT8 reg, UINT8 data);


extern BOOLEAN kxte9_init(KXTE9_INFO *accel_info);
extern BOOLEAN kxte9_preload_init(void);

//Control Functions
extern BOOLEAN kxte9_enable_outputs(void);
extern BOOLEAN kxte9_sleep(void);
extern BOOLEAN kxte9_reset(void);
extern BOOLEAN kxte9_enable_all(void);
extern BOOLEAN kxte9_disable_all(void);
extern BOOLEAN kxte9_enable_tilt(void);
extern BOOLEAN kxte9_disable_tilt(void);
extern BOOLEAN kxte9_enable_activity_monitor(void);
extern BOOLEAN kxte9_disable_activity_monitor(void);


// ORD Frequency
extern BOOLEAN kxte9_set_odr_frequency(UINT16 frequency);
extern BOOLEAN kxte9_set_tilt_odr_frequency(UINT16 frequency);
extern BOOLEAN kxte9_set_wuf_odr_frequency(UINT16 frequency);
extern BOOLEAN kxte9_set_b2s_odr_frequency(UINT16 frequency);
extern BOOLEAN kxte9_read_current_odr(UINT8 *ODR_rate);


//Timer / Threshold / Angle
extern BOOLEAN kxte9_tilt_timer(UINT8 tilt_timer);
extern BOOLEAN kxte9_wuf_timer(UINT8 wuf_timer);
extern BOOLEAN kxte9_b2s_timer(UINT8 b2s_timer);
extern BOOLEAN kxte9_wuf_threshold(UINT8 wuf_threshold);
extern BOOLEAN kxte9_b2s_threshold(UINT8 b2s_threshold);
extern BOOLEAN kxte9_set_tilt_angle(UINT8 tilt_angle);
extern BOOLEAN kxte9_set_hysteresis(UINT8 hysteresis);


//Read Functions
extern BOOLEAN kxte9_read_previous_tilt_position(UINT8 *previous_position);
extern BOOLEAN kxte9_read_current_tilt_position(UINT8 *current_position);
extern BOOLEAN kxte9_read_motion_direction(UINT8 *int_src_reg2);
extern BOOLEAN kxte9_read_position_status(void);
extern BOOLEAN kxte9_read_wuf_status(void);
extern BOOLEAN kxte9_read_b2s_status(void);
extern BOOLEAN kxte9_read_g(INT16 *gx, INT16 *gy, INT16 *gz);
extern BOOLEAN kxte9_read_data_overrun(void);
extern BOOLEAN kxte9_read_cnt(INT16 *x, INT16 *y, INT16 *z);


//interrupt control
extern BOOLEAN kxte9_enable_int(void);
extern BOOLEAN kxte9_disable_int(void);
extern BOOLEAN kxte9_read_interrupt_status(void);
extern BOOLEAN kxte9_int_activeh(void);
extern BOOLEAN kxte9_int_activel(void);
extern BOOLEAN kxte9_int_latch(void);
extern BOOLEAN kxte9_int_pulse(void);
extern BOOLEAN kxte9_service_interrupt(UINT8 *source_of_interrupt);
extern BOOLEAN kxte9_service_interrupt2(INT8U *source_of_interrupt, INT16 *xyz);

//Masking Functions
extern BOOLEAN kxte9_position_mask_x(void);
extern BOOLEAN kxte9_position_mask_y(void);
extern BOOLEAN kxte9_position_mask_z(void);
extern BOOLEAN kxte9_position_unmask_x(void);
extern BOOLEAN kxte9_position_unmask_y(void);
extern BOOLEAN kxte9_position_unmask_z(void);
extern BOOLEAN kxte9_position_mask_fu(void);
extern BOOLEAN kxte9_position_mask_fd(void);
extern BOOLEAN kxte9_position_mask_up(void);
extern BOOLEAN kxte9_position_mask_do(void);
extern BOOLEAN kxte9_position_mask_ri(void);
extern BOOLEAN kxte9_position_mask_le(void);
extern BOOLEAN kxte9_position_unmask_fu(void);
extern BOOLEAN kxte9_position_unmask_fd(void);
extern BOOLEAN kxte9_position_unmask_up(void);
extern BOOLEAN kxte9_position_unmask_do(void);
extern BOOLEAN kxte9_position_unmask_ri(void);
extern BOOLEAN kxte9_position_unmask_le(void);
extern BOOLEAN kxe9_mask_xyz(INT8U x, INT8U y, INT8U z);

/**************************************************/
//Optional. Hardware/platform specific. Please modify
extern void kxte9_isr(void);
//extern void kxte9_enable_interrupt(void)
//extern void kxte9_disable_interrupt(void)

/**
* open the i2c_handle - should be called at the beginning before any register access happens
*/
char open_i2c_handle(void);

/**
* close the i2c handle - no more access to motion sensor registers
*/
char close_i2c_handle(void);

/*================================================================================================*/


#endif
