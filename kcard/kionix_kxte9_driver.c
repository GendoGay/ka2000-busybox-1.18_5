/*
 * KeyASIC KA2000 series software
 *
 * Copyright (C) 2013 KeyASIC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifdef __cplusplus
extern "C" {
#endif
    /*==================================================================================================

        Module Name:  kionix_kxte9_driver.c

        General Description: this file contains function calls to control Kionix KXTE9 via I2C.

        Driver Guide Document:  Rev 1.2 (3/31/2010)
    ====================================================================================================


    Revision History:
                                Modification     Tracking
    Author                          Date          Number     Description of Changes
    -------------------------   ------------    ----------   -------------------------------------------
    Calvin Ong                   03/31/2010                   Add new Initialization function.
                                                              1/ Replace kxte9_init.
                                                              2/ Renamed previous kxte9_init to kxte9_preload_init
                                                              3/ Further code clean up.
    Calvin Ong                   02/01/2010                   Correct code errors
    Calvin Ong                   29/07/2009                   Touch up codes.
    Izudin Cemer/Ken Foust       02/18/2009					  More clean up
    Ken Foust                    11/10/2008                   Cleaned up code for general release
    Izudin Cemer/Ken Foust       10/08/2008                   Cleaned up code for initial release
    Izudin Cemer/Ken Foust       08/27/2008                   Initial support

    ====================================================================================================
                                            INCLUDE FILES
    ==================================================================================================*/
#include "kionix_kxte9_driver.h"

    /*==================================================================================================
                                         GLOBAL VARIABLES
    ==================================================================================================*/
    //static UINT16 kxte9_g_range = 2000;   /* KXTE9 factory default G range in milli g */

	int g_i2c_handle=-1 ; // initial value 
    /*==================================================================================================
                                         LOCAL FUNCTIONS
    ==================================================================================================*/
    static void kxte9_enable_interrupt(void);
    static void kxte9_disable_interrupt(void);

    /*==================================================================================================
                                           KIONIX I2C DRIVER FUNCTIONS
    ==================================================================================================*/

    /*==================================================================================================

    FUNCTION: kxte9_read_bytes

    DESCRIPTION: THIS IS PSEUDO CODE
       this function must be written to read data from the kionix accelerometer in bytes.

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       Developer note: should add i2c interface into this function call.
    ==================================================================================================*/
    BOOLEAN kxte9_read_bytes(UINT8 reg, UINT8 *data, UINT8 length)
    {
        i2c_rw_cmd_type i2c_command;

        i2c_command.slave_addr	= KXTE9_I2C_SLV_ADDR;	/* Please provide value to define accelerometer slave address*/
        i2c_command.length		= length;				/* Please provide value to define number of I2C bytes*/
        i2c_command.slave_reg_addr	= reg;                  /* Please provide value of I2C register address*/
        i2c_command.data_ptr = data;					/* I2C read back data*/
		usleep (1);

        if(read(g_i2c_handle, &i2c_command, sizeof(i2c_rw_cmd_type)) == 1)
            return TRUE;

        return FALSE;
    }

    /*==================================================================================================

    FUNCTION: kxte9_write_byte

    DESCRIPTION: THIS IS PSEUDO CODE
       this function must be written to write a byte of data to the kionix accelerometer.

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
        Developer note: should add i2c interface into this function call.
    ==================================================================================================*/
    BOOLEAN kxte9_write_byte(UINT8 reg, UINT8 data)
    {
        i2c_rw_cmd_type i2c_command;

        i2c_command.slave_addr	= KXTE9_I2C_SLV_ADDR;	/* Please provide value to define accelerometer slave address*/
        i2c_command.length		= 1;
        i2c_command.slave_reg_addr	= reg;							/* Please provide value of I2C register address*/
        i2c_command.data_ptr = &data;
										
		usleep (1);                                                                   
        if(write(g_i2c_handle, &i2c_command, sizeof(i2c_rw_cmd_type)) == 1)
            return TRUE;

        return FALSE;
    }

    /*==================================================================================================
    FUNCTION: KXTE9_init
    DESCRIPTION:      this function initializes the sensor
    ARGUMENTS PASSED: Pointers to sensor information structures
    return VALUE:     None
    PRE-CONDITIONS:   None
    POST-CONDITIONS:  None
    IMPORTANT NOTES:  None
    ==================================================================================================*/
    BOOLEAN kxte9_init(KXTE9_INFO *accel_info)
    {
        UINT8 ctlreg_1 = 0;
        UINT8 ctlreg_2 = 0;
        UINT8 ctlreg_3 = 0;
        UINT8 int_ctlreg_1 = 0;
        UINT8 int_ctlreg_2 = 0;
        UINT8 read_temp_hyst = 0;
        UINT8 write_temp_hyst = 0;
        BOOLEAN status = TRUE;

        TILTINFO *dTILT;
        WUFINFO *dWUF;
        B2SINFO *dB2S;
        MASKINFO *dMASK;
        INTINFO *dINT;

        dTILT = &accel_info->tiltinfo;
        dWUF = &accel_info->wufinfo;
        dB2S = &accel_info->b2sinfo;
        dMASK = &accel_info->maskinfo;
        dINT = &accel_info->intinfo;

        //Disable sensor at the begining of initialization and reset CTRL_REG1
        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_PC1);
        kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
		usleep(10);


        /************************    TILT    **************************/


        //ODR - note: need to set the initial ODR, or else engine will not run when the sensor is enabled.

        /* ODR and Tilt Timer should always be set if we're enabling the tilt engine; Tilt Angle and Hysteresis are optional */
        if (dTILT->ODR >= 10) 					/* set ODRA and ODRB according to dTILT->ODR */
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);

            if (dTILT->ODR == 40)
            {
                SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
            }
            else if(dTILT->ODR == 125)
            {
                SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
                SET_REG_BIT(ctlreg_1, CTRL_REG1_BIT5);

            }
        }
        else if (dTILT->ODR == 3)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
        }


        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_TPE/*KXTE9_CTRL_REG1_TPE*/); /*default TPE = 1, hence clear TPE bit*/

        if (dTILT->enable == 1)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_TPE);	/* set TPE bit to enable the screen rotation (tilt) function */
            //Timer
            kxte9_tilt_timer((UINT8)dTILT->timer);

            //Angle
            if (dTILT->angle != 0)
            {
                if (kxte9_write_byte(KXTE9_I2C_TILT_ANGLE, (UINT8)dTILT->angle) != TRUE)
                {
                    status = FALSE;
                }
            }

            //hysteresis
            if (dTILT->hysteresis != 0)
            {
                if (kxte9_read_bytes(KXTE9_I2C_HYST_SET, &read_temp_hyst, 1) == TRUE)
                {
                    write_temp_hyst = read_temp_hyst & (UINT8)dTILT->hysteresis;
                    if (kxte9_write_byte(KXTE9_I2C_HYST_SET, write_temp_hyst) != TRUE)
                    {
                        status = FALSE;
                    }
                }
                else status = FALSE;
            }//hysteresis
        }//dTILT->enable



        /************************    WUF    **************************/
        if (dWUF->enable == 1)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE);	/* set WUFE to enable the wake up function */

            //Timer
            kxte9_wuf_timer((UINT8)dWUF->timer);

            //Threshold
            /* changing WUF Threshold is optional; factory default is 0.5g */
            if (dWUF->thresh != 0)
            {
                if (kxte9_write_byte(KXTE9_I2C_WUF_THRESH, (UINT8)dWUF->thresh) != TRUE)
                {
                    status = FALSE;
                }
            }//Threshold
        }//dWUF->enable

        /************************    B2S    **************************/

        if (dB2S->enable == 1)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE);	/* set B2SE to enable the sleep function */

            //Timer
            kxte9_b2s_timer((UINT8)dB2S->timer);

            //Threshold
            /* changing B2S Threshold is optional; factory default is 1.5g */
            if (dB2S->thresh != 0)
            {
                if (kxte9_write_byte(KXTE9_I2C_B2S_THRESH, (UINT8)dB2S->thresh) != TRUE)
                {
                    status = FALSE;
                }
            }//Threshold
        }//dB2S->enable


        //ODR
        //Setting all bits for Control Register 3 if WUF or B2S are enabled (eliminates register read)
        if (dWUF->enable || dB2S->enable)
        {
            if (dWUF->ODR >= 10) 						/* set OWUFA and OWUFB according to dWUF->ODR */
            {
                SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
                if (dWUF->ODR == 40)
                {
                    SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
                }
            }
            else if (dWUF->ODR == 3)
            {
                SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
            }

            if (dB2S->ODR >= 10) 						/* set OB2SA and OB2SB according to dB2S->ODR */
            {
                SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);
                if (dB2S->ODR == 40)
                {
                    SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
                }
            }
            else if (dB2S->ODR == 3)
            {
                SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            }
        }//end of if




        /************************    Interrupt    **************************/
        //Setting all bits for Control Register 2 if orientation direction masks are enabled (eliminates register read)
        if (dMASK->dir_mask_enable == 1)
        {
            if (dMASK->mask_le == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_LEM);
            }
            if (dMASK->mask_ri == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_RIM);
            }
            if (dMASK->mask_do == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_DOM);
            }
            if (dMASK->mask_up == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_UPM);
            }
            if (dMASK->mask_fd == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_FDM);
            }
            if (dMASK->mask_fu == 1)
            {
                SET_REG_BIT(ctlreg_2, CTRL_REG2_FUM);
            }

            if (kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctlreg_2) != TRUE)
            {
                status = FALSE;
            }
        }

        //Setting all bits for Interrupt Control Register 2 if Axis Masking is enabled (eliminates register read)
        if(dMASK->axis_mask_enable == 1)
        {
            if (dMASK->x_mask == 1)
            {
                SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_XBW);
            }
            if (dMASK->y_mask == 1)
            {
                SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_YBW);
            }
            if (dMASK->z_mask == 1)
            {
                SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_ZBW);
            }
            if (kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctlreg_2) != TRUE)
            {
                status = FALSE;
            }
        }


        //Setting all bits for Interrupt Control Register 1 if Interrupt is enabled (eliminates register read)
        if (dINT->enable == 1)
        {
            SET_REG_BIT(int_ctlreg_1, INT_CTRL_REG1_IEN);
        }
        if (dINT->polarity == 1)
        {
            SET_REG_BIT(int_ctlreg_1, INT_CTRL_REG1_IEA);
        }
        if (dINT->latch == 1)
        {
            SET_REG_BIT(int_ctlreg_1, INT_CTRL_REG1_IEL);
        }

        if (kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctlreg_1) != TRUE)
        {
            status = FALSE;
        }

        if (kxte9_write_byte(KXTE9_I2C_CTRL_REG3, ctlreg_3) != TRUE)
        {
            status = FALSE;   /* Write ODRs settings*/
        }
        if (kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1) != TRUE)
        {
            status = FALSE;   /* write Control Register 1 last when enabling the part */
        }

        return status;
    }
    /*==================================================================================================

    FUNCTION: kxte9_preload_init

    DESCRIPTION:
       this function initializes the kionix accelerometer for screen rotation.

    ARGUMENTS PASSED:
       None

    return VALUE:
       None

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_preload_init(void)
    {
        UINT8 ctlreg_1 = 0;
        UINT8 ctlreg_3 = 0;
        BOOLEAN status = TRUE;

        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_PC1);	/* disable accelerometer outputs */
        SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);	/* set ODR to 40 Hz */
        SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_TPE); /* disable the tilt position function */
        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE); /* disable wake up function */
        UNSET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE); /* disable back to sleep function */

        if (kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1) == TRUE)
        {
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);	/* set ODR to 40 Hz for B2S and WUF engines*/
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);

            if (kxte9_write_byte(KXTE9_I2C_CTRL_REG3, ctlreg_3) == TRUE)
            {
                if (kxte9_tilt_timer(20) == TRUE)
                {
                    if (kxte9_wuf_timer(4) == TRUE)
                    {
                        if (kxte9_b2s_timer(3) == TRUE)
                        {
                            if (kxte9_b2s_threshold(64) == TRUE)
                            {
                                if (kxte9_wuf_threshold(16) == TRUE)
                                {
                                    status = TRUE;
                                }
                                else
                                {
                                    status = FALSE;
                                }
                            }
                            else
                            {
                                status = FALSE;
                            }
                        }
                        else
                        {
                            status = FALSE;
                        }
                    }
                    else
                    {
                        status = FALSE;
                    }
                }
                else
                {
                    status = FALSE;
                }
            }
            else
            {
                status = FALSE;
            }
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_set_odr_frequency

    DESCRIPTION:
       this function sets the odr frequency for the kionix kxte9 accelerometer.

    ARGUMENTS PASSED:
       UINT16 frequency - filter frequency in hertz (1Hz, 3Hz, 10Hz, 40Hz)

    return VALUE:
       None.

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_set_odr_frequency(UINT16 frequency)
    {
        UINT8 ctlreg_1 = 0;
        UINT8 ctlreg_3 = 0;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) != TRUE)
        {
            return FALSE;
        }

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG3, &ctlreg_3, 1) != TRUE)
        {
            return FALSE;
        }

        switch (frequency)
        {
        case 1:		/* set all ODR's to 1Hz */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            break;
        case 3:      /* set all ODR's to 3Hz */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            break;
        case 10:    /* set all ODR's to 10Hz */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);
            UNSET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            break;
        case 40:   /* set all ODR's to 40Hz */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRA);
            SET_REG_BIT(ctlreg_1, CTRL_REG1_ODRB);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFA);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OWUFB);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SA);
            SET_REG_BIT(ctlreg_3, CTRL_REG3_OB2SB);
            break;
        default:
            return FALSE;
        }

        kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        kxte9_write_byte(KXTE9_I2C_CTRL_REG3, ctlreg_3);

        return TRUE;
    }


    /*==================================================================================================

    FUNCTION: kxte9_isr

    DESCRIPTION:
       this function is the interrupt service routine for the kionix kxte9 accelerometer.

    ARGUMENTS PASSED:
       None

    return VALUE:
       None.

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
            Developer note: Should modify for your system. Hardware specific. Please modify.
    ==================================================================================================*/
    void kxte9_isr(void)
    {

        // disable accelerometer interrupt first
        kxte9_disable_interrupt();

        // Set event to handle interrupt
        // add code here

        // enable accelerometer interrupt again
        kxte9_enable_interrupt();
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_interrupt

    DESCRIPTION:
       this function enables the interrupt for the kionix kxte9 accelerometer.

    ARGUMENTS PASSED:
       None

    return VALUE:
       None.

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
        Developer note: Should modify for your system. Hardware specific. Please modify.
    ==================================================================================================*/
    static void kxte9_enable_interrupt(void)
    {
        // set up interrupt for rising edge detection
        // enable hardware interrupt, and callback to kxte9_isr.
        //add code here
    }


    /*==================================================================================================

    FUNCTION: kxte9_disable_interrupt

    DESCRIPTION:
       this function disables the interrupt for the kionix kxte9 accelerometer.

    ARGUMENTS PASSED:
       None

    return VALUE:
       None.

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
        Developer note: Should modify for your system. Hardware specific. Please modify.
    ==================================================================================================*/
    static void kxte9_disable_interrupt(void)
    {
        //disable the physical interrupt pin on gpio
        //add code here
    }


    /*==================================================================================================

    FUNCTION: kxte9_service_interrupt

    DESCRIPTION:
       this function clears the IRQ status

    ARGUMENTS PASSED:
       None

    return VALUE:
       TRUE if an interrupt had been pending.

    PRE-CONDITIONS:
       None

    POST-CONDITIONS:
       Interrupt pending bit (MOTI in REGA) will be cleared

    IMPORTANT NOTES:
       do not call this from interrupt context, it accesses i2c by calling i2c_read()
    ==================================================================================================*/
    BOOLEAN kxte9_service_interrupt(UINT8 *source_of_interrupt)
    {
        BOOLEAN return_status = FALSE;
        UINT8 dummy = 0;

        // read the interrupt source register
        if (kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG1, source_of_interrupt, 1) == TRUE)
        {
            // clear the interrupt source information along with interrupt pin
            if (kxte9_read_bytes(KXTE9_I2C_INT_REL, &dummy, 1) == TRUE)
            {
                return_status = TRUE;
            }
            else
            {
                return_status = FALSE;
            }
        }
        else
        {
            return_status = FALSE;
        }

        return return_status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_outputs

    DESCRIPTION:
       this function enables 6-bit accelerometer outputs

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_enable_outputs(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_PC1); /* sets PC1 bit to be in power up state */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_tilt

    DESCRIPTION:
       this function enables the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_enable_tilt(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_TPE); /* sets TPE bit to enable screen rotation */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }

    /*==================================================================================================

    FUNCTION: kxte9_disable_tilt

    DESCRIPTION:
       this function disables the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_disable_tilt(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_TPE); /* unset TPE bit to disable screen rotation */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_activity_monitor

    DESCRIPTION:
       this function enables the wake up and back to sleep engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_enable_activity_monitor(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE); /* set WUFE bit to enable the wake up function */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE); /* set B2SE bit to enable the sleep function */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_disable_activity_monitor

    DESCRIPTION:
       this function disables the wake up and back to sleep engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_disable_activity_monitor(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE); /* unset the WUFE bit to disable the wake up function */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE); /* unset the B2SE bit to disable the sleep function */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_all

    DESCRIPTION:
       this function enables all the engines

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_enable_all(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            SET_REG_BIT(ctlreg_1, CTRL_REG1_PC1); /* set PC1 to enable the accelerometer outputs */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_TPE); /* set TPE bit to enable the screen rotation (tilt function) */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE); /* set WUFE to enable the wake up function */
            SET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE); /* set B2SE to enable the sleep function */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_disable_all

    DESCRIPTION:
       this function disables all the engines

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_disable_all(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE)
        {
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_PC1); /* unset the PC1 bit to disable the accelerometer outputs */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_TPE); /* unset the TPE bit to disable the screen rotation (tilt function) */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_WUFE); /* unset the WUFE bit to disable the wake up function */
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_B2SE); /* unset the B2SE bit to disable the sleep function */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_sleep

    DESCRIPTION:
       this function places the kxte9 into a standby state while retaining current register values.

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_sleep(void)
    {
        UINT8 ctlreg_1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG1, &ctlreg_1, 1) == TRUE )
        {
            UNSET_REG_BIT(ctlreg_1, CTRL_REG1_PC1); /* unset the PC1 bit to disable the accelerometer */
            kxte9_write_byte(KXTE9_I2C_CTRL_REG1, ctlreg_1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_cnt

    DESCRIPTION:
       this function reads the number of counts on the x, y, and z axis from the kionix accelerometer.

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_cnt(INT16 *x, INT16 *y, INT16 *z)
    {
        BOOLEAN status;
        UINT8 xyz[3];

        if ((status = kxte9_read_bytes(KXTE9_I2C_X_OUT, xyz, sizeof(xyz))) == TRUE)
        {
            *x = ((INT16)xyz[0]) >> 2;
            *y = ((INT16)xyz[1]) >> 2;
            *z = ((INT16)xyz[2]) >> 2;
        }

        return status;
    }

    /*==================================================================================================

    FUNCTION: kxte9_read_g

    DESCRIPTION:
       this function reads the G(gravity force) values on the x, y, and z axis from the kionix accelerometer.
       The unit used is milli G, or 1000 * G.
    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_g(INT16 *gx, INT16 *gy, INT16 *gz)
    {
        BOOLEAN status;
        INT16 x, y, z;
        INT16 sensitivity = BIT_SENSITIVITY_2_G;

        if ((status = kxte9_read_cnt(&x, &y, &z)) == TRUE)
        {
            /* calculate milli-G's */
            *gx = 1000 * (x - ZERO_G_OFFSET) / sensitivity;
            *gy = 1000 * (y - ZERO_G_OFFSET) / sensitivity;
            *gz = 1000 * (z - ZERO_G_OFFSET) / sensitivity;
        }

        return status;
    }

    /*==================================================================================================

    FUNCTION: kxte9_read_current_odr

    DESCRIPTION:
       this function reads the current ODR of the kionix accelerometer
    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_current_odr(UINT8 *ODR_rate)
    {
        BOOLEAN status;
        UINT8 status_reg;

        if (kxte9_read_bytes(KXTE9_I2C_STATUS_REG, &status_reg, 1) == TRUE)
        {
            status_reg &= 0xC0;
            status_reg >>= 2;

            switch (status_reg)
            {
            case 0:
                *ODR_rate = 1;
                status = TRUE;
                break;
            case 1:
                *ODR_rate = 3;
                status = TRUE;
                break;
            case 2:
                *ODR_rate = 10;
                status = TRUE;
                break;
            case 3:
                *ODR_rate = 40;
                status = TRUE;
                break;
            default:
                status = FALSE;
                break;
            }
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_interrupt_status

    DESCRIPTION:
       this function reads the physical pin interrupt status

    ARGUMENTS PASSED:

    return VALUE:
       TRUE if interrupt active, FALSE if interrupt inactive

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_interrupt_status(void)
    {
        BOOLEAN interrupt_status;
        UINT8 status_reg;

        if (kxte9_read_bytes(KXTE9_I2C_STATUS_REG, &status_reg, 1) == TRUE)
        {
            if ((status_reg & 0x10) == 0x00)
            {
                interrupt_status = FALSE;
            }
            else
            {
                interrupt_status = TRUE;
            }
        }
        else
        {
            interrupt_status = FALSE;
        }

        return interrupt_status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_data_overrun

    DESCRIPTION:
       this function reads STATUS_REG to determine if data over run occur

    ARGUMENTS PASSED:

    return VALUE:
       TRUE if data over run occured, FALSE if no data over run occured

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_data_overrun(void)
    {
        BOOLEAN DOR;
        UINT8 status_reg;

        if (kxte9_read_bytes(KXTE9_I2C_STATUS_REG, &status_reg, 1) == TRUE)
        {
            if ((status_reg & 0x20) == 0x00)
            {
                DOR = FALSE;
            }
            else
            {
                DOR = TRUE;
            }
        }
        else
        {
            DOR = FALSE;
        }

        return DOR;
    }


    /*==================================================================================================

    FUNCTION: kxte9_int_read_position_status

    DESCRIPTION:
       this function reads INT_SRC_REG1 to determine if there was a change in tilt

    ARGUMENTS PASSED:

    return VALUE:
       TRUE if tilt occured or FALSE if no tilt occured

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_position_status(void)
    {
        BOOLEAN position_status;
        UINT8 src_reg1;

        if (kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG1, &src_reg1, 1) == TRUE)
        {
            if((src_reg1 & 0x01) == 1)
            {
                position_status = TRUE;
            }
            else
            {
                position_status = FALSE;
            }
        }
        else
        {
            position_status = FALSE;
        }

        return position_status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_wuf_status

    DESCRIPTION:
       this function reads INT_SRC_REG1 to determine if wake up occured

    ARGUMENTS PASSED:

    return VALUE:
       TRUE if wake up occured or FALSE if no wake up occured

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_wuf_status(void)
    {
        BOOLEAN wuf_status;
        UINT8 src_reg1;

        if (kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG1, &src_reg1, 1) == TRUE)
        {
            if((src_reg1 & 0x02) == 0x02)
            {
                wuf_status = TRUE;
            }
            else
            {
                wuf_status = FALSE;
            }
        }
        else
        {
            wuf_status = FALSE;
        }

        return wuf_status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_b2s_status

    DESCRIPTION:
       this function reads INT_SRC_REG1 to determine if back to sleep occured

    ARGUMENTS PASSED:

    return VALUE:
       TRUE if back to sleep occured or FALSE if no back to sleep occured

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_b2s_status(void)
    {
        BOOLEAN b2s_status;
        UINT8 src_reg1;

        if (kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG1, &src_reg1, 1) == TRUE)
        {
            if((src_reg1 & 0x04) == 0x04)
            {
                b2s_status = TRUE;
            }
            else
            {
                b2s_status = FALSE;
            }
        }
        else
        {
            b2s_status = FALSE;
        }

        return b2s_status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_motion_direction

    DESCRIPTION:
       this function reads INT_SRC_REG2 to determine the direction motion event

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_motion_direction(UINT8 *int_src_reg2)
    {
        BOOLEAN status;

        status = kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG2, int_src_reg2, 1);

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_previous_position

    DESCRIPTION:
       this function reads the previous tilt position register

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_previous_position(UINT8 *previous_position)
    {
        BOOLEAN status;

        status = kxte9_read_bytes(KXTE9_I2C_TILT_POS_PRE, previous_position, 1);

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_read_current_position

    DESCRIPTION:
       this function reads the current tilt position register

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_read_current_position(UINT8 *current_position)
    {
        BOOLEAN status;

        status = kxte9_read_bytes(KXTE9_I2C_TILT_POS_CUR, current_position, 1);

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_reset

    DESCRIPTION:
       this function issues a software reset to Kionix accelerometer.

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_reset(void)
    {
        UINT8 ctrl_reg3 = 0;
        BOOLEAN status = TRUE;

		#if 0 
        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG3, &ctrl_reg3, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg3, CTRL_REG3_SRST);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG3, ctrl_reg3);
        }
        else
        {
            status = FALSE;
        }
		#endif
		
		printf ("reset motion sensor ic\n");
        kxte9_write_byte(KXTE9_I2C_CTRL_REG3, 0x86); //sw reset 
		usleep (10);
             
        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_enable_int

    DESCRIPTION:
       this function enables the physical interrupt on Kionix's accelerometer

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_enable_int(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEN);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_disable_int

    DESCRIPTION:
       this function disables the physical interrupt on Kionix's accelerometer

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_disable_int(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEN);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_int_activeh

    DESCRIPTION:
       this function sets the polarity of physical intettupt to active high on Kionix's accelerometer

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_int_activeh(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEA);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_int_activel

    DESCRIPTION:
       this function sets the polarity of physical intettupt to active low on Kionix's accelerometer

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_int_activel(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEA);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_int_latch

    DESCRIPTION:
       this function sets the physical interrupt on Kionix's accelerometer to a ltached state

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_int_latch(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEL);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_int_pulse

    DESCRIPTION:
       this function sets the physical interrupt on Kionix's accelerometer to a pulse state

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_int_pulse(void)
    {
        UINT8 int_ctrl_reg1 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG1, &int_ctrl_reg1, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg1, INT_CTRL_REG1_IEL);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG1, int_ctrl_reg1);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_tilt_timer

    DESCRIPTION:
       this function sets the number of screen rotation debounce samples

    ARGUMENTS PASSED:
    	Tilt timer vlue from 0-255

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_tilt_timer(UINT8 tilt_timer)
    {
        BOOLEAN status;

        if (kxte9_write_byte(KXTE9_I2C_TILT_TIMER, tilt_timer) == TRUE)
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_wuf_timer

    DESCRIPTION:
       this function sets the number of wake up debounce samples

    ARGUMENTS PASSED:
    	Wuf timer vlue from 0-255

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_wuf_timer(UINT8 wuf_timer)
    {
        BOOLEAN status;

        if (kxte9_write_byte(KXTE9_I2C_WUF_TIMER, wuf_timer) == TRUE)
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_b2s_timer

    DESCRIPTION:
       this function sets the number of back to sleep debounce samples

    ARGUMENTS PASSED:
    	B2s timer vlue from 0-255

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_b2s_timer(UINT8 b2s_timer)
    {
        BOOLEAN status;

        if (kxte9_write_byte(KXTE9_I2C_B2S_TIMER, b2s_timer) == TRUE)
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_b2s_threshold

    DESCRIPTION:
       this function writes back to sleep threshold to the register

    ARGUMENTS PASSED:
    	B2s threshold vlue in counts from 0-255

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_b2s_threshold(UINT8 b2s_threshold)
    {
        BOOLEAN status;

        if (kxte9_write_byte(KXTE9_I2C_B2S_THRESH, b2s_threshold) == TRUE)
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_wuf_threshold

    DESCRIPTION:
       this function writes a wake up threshold value to the register

    ARGUMENTS PASSED:
    	wuf threshold vlue in counts from 0-255

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_wuf_threshold(UINT8 wuf_threshold)
    {
        BOOLEAN status;

        if (kxte9_write_byte(KXTE9_I2C_WUF_THRESH, wuf_threshold) == TRUE)
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_z

    DESCRIPTION:
       this function masks z-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_z(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_ZBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_z

    DESCRIPTION:
       this function unmasks z-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_z(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_ZBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_y

    DESCRIPTION:
       this function masks y-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_y(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_YBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_y

    DESCRIPTION:
       this function unmasks y-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_y(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_YBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_x

    DESCRIPTION:
       this function unmasks x-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_x(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_XBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_x

    DESCRIPTION:
       this function unmasks x-axis from activity engine

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_x(void)
    {
        UINT8 int_ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_INT_CTRL_REG2, &int_ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(int_ctrl_reg2, INT_CTRL_REG2_XBW);
            kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_fu

    DESCRIPTION:
       this function masks face up in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_fu(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_FUM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_fd

    DESCRIPTION:
       this function masks face down in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_fd(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_FDM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_up

    DESCRIPTION:
       this function masks up state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_up(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_UPM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_do

    DESCRIPTION:
       this function masks down state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_do(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_DOM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_ri

    DESCRIPTION:
       this function masks right state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_ri(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_RIM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_mask_le

    DESCRIPTION:
       this function masks left state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_mask_le(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            SET_REG_BIT(ctrl_reg2, CTRL_REG2_LEM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_fu

    DESCRIPTION:
       this function unmasks face up in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_fu(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_FUM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_fd

    DESCRIPTION:
       this function unmasks face down in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_fd(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_FDM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_up

    DESCRIPTION:
       this function unmasks up state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_up(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_UPM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_do

    DESCRIPTION:
       this function unmasks down state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_do(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_DOM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_ri

    DESCRIPTION:
       this function unmasks right state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_ri(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_RIM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }


    /*==================================================================================================

    FUNCTION: kxte9_position_unmask_le

    DESCRIPTION:
       this function unmasks left state in the screen rotation function

    ARGUMENTS PASSED:

    return VALUE:
       None

    PRE-CONDITIONS:
       i2c_init() has been called

    POST-CONDITIONS:
       None

    IMPORTANT NOTES:
       None
    ==================================================================================================*/
    BOOLEAN kxte9_position_unmask_le(void)
    {
        UINT8 ctrl_reg2 = 0;
        BOOLEAN status = TRUE;

        if (kxte9_read_bytes(KXTE9_I2C_CTRL_REG2, &ctrl_reg2, 1) == TRUE)
        {
            UNSET_REG_BIT(ctrl_reg2, CTRL_REG2_LEM);
            kxte9_write_byte(KXTE9_I2C_CTRL_REG2, ctrl_reg2);
        }
        else
        {
            status = FALSE;
        }

        return status;
    }

    BOOLEAN kxte9_service_interrupt2(INT8U *source_of_interrupt, INT16 *xyz)
    {
        BOOLEAN return_status = FALSE;
        INT8U dummy = 0;
        INT8U data[3];
        INT16 xyz_read[3];

        // read 3 source registers irregardless of any interrupt event
        // TILT_POS_CUR ===>data[0]
        // INT_SRC_REG1 ==> data[1]
        // INT_SRC_REG2 ==> data[2]


        kxte9_read_g(&xyz_read[0], &xyz_read[1], &xyz_read[2]) ;

        if (kxte9_read_bytes(KXTE9_I2C_TILT_POS_CUR, data, 1) == TRUE) //read 1 byte
        {
            if (kxte9_read_bytes(KXTE9_I2C_INT_SRC_REG1, &data[1], 2) == TRUE) //read 2 bytes
            {
                // clear the interrupt source information along with interrupt pin, irregardless of any interrupt event
                if (kxte9_read_bytes(KXTE9_I2C_INT_REL, &dummy, 1) == TRUE)
                {
                    if(data[1] & 0x0F) //if interrupt occurs
                    {
                        return_status = TRUE;
                        //printf("Here, interrupt occured\n");
                    }
                }
            }
        }
        else
        {
            return_status = FALSE;
        }

        source_of_interrupt[0] = data[0]; //TILT_POS_CUR
        source_of_interrupt[1] = data[1]; //INT_SRC_REG1
        source_of_interrupt[2] = data[2]; //INT_SRC_REG2
        xyz[0] = xyz_read[0];
        xyz[1] = xyz_read[1];
        xyz[2] = xyz_read[2];

        return return_status;
    }

    BOOLEAN kxe9_mask_xyz(INT8U x, INT8U y, INT8U z)
    {
        INT8U int_ctlreg_2 = 0;
        //Setting all bits for Interrupt Control Register 2 if Axis Masking is enabled (eliminates register read)
        if (x == 1)
        {
            SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_XBW);
        }
        if (y == 1)
        {
            SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_YBW);
        }
        if (z == 1)
        {
            SET_REG_BIT(int_ctlreg_2, INT_CTRL_REG2_ZBW);
        }
        if (kxte9_write_byte(KXTE9_I2C_INT_CTRL_REG2, int_ctlreg_2))
            return TRUE;
        else
            return FALSE;

    }

	
	char open_i2c_handle(void)
	{
		if (g_i2c_handle==-1)
		{
			g_i2c_handle = open(MOTION_SENSOR_DEV_NODE,O_RDWR);
			if (g_i2c_handle<0)
			{
				printf ("cannot open %s  to access motion sensor iic\n ", MOTION_SENSOR_DEV_NODE);
				g_i2c_handle=-1;
				return -1; //error
			}
					
		}
		return 0;
	}

	char close_i2c_handle(void)
	{

		if (g_i2c_handle!=-1)
		{
			close(g_i2c_handle);

			g_i2c_handle=-1;
					
		}
		return 0;
	}

#ifdef __cplusplus
}
#endif
