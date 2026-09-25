/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< LM35_interfae.h >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 6, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : _HAL
 * SWC   : LM35
 */

#ifndef _HAL_7_LM35_LM35_INTERFACE_H_
#define _HAL_7_LM35_LM35_INTERFACE_H_

typedef struct
	{
		u8 Copy_u8LM35Channel ;
		u16 Copy_u16ADCVoltageReference ;     /* ADC reference in mV: 5000, 2560, or external reference in mV */
	} LM35_CONFIG ;

	u8 LM35_u8GetTemp (LM35_CONFIG * lm35 , u8 * Copy_u8TempValue) ;


#endif /* _HAL_7_LM35_LM35_INTERFACE_H_ */
