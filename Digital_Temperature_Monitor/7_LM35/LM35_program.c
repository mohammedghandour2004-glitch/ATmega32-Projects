/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< LM35_program.c >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 6, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : _HAL
 * SWC   : LM35
 */

#include "_LIB/BIT_MATH.h"
#include "_LIB/STD_TYPES.h"

#include "7_LM35/LM35_interface.h"
#include "7_LM35/LM35_private.h"
#include "2_DIO/DIO_interface.h"
#include "5_ADC/ADC_interface.h"


/*
 * Brief:
 * This function reads the LM35 analog value through ADC
 * and converts it to temperature in Celsius.
 *
 * Parameters:
 * lm35             -> Pointer to LM35 configuration structure
 * Copy_u8TempValue -> Pointer to variable where temperature will be stored
 *
 * Return:
 * Error state
 */
u8 LM35_u8GetTemp(LM35_CONFIG *lm35, u8 *Copy_u8TempValue)
{
	u8 Local_u8ErrorState = OK;

	u16 Local_u16ADCResult;
	u16 Local_u16AnalogValue = 0;

	if ((lm35 == NULL) || (Copy_u8TempValue == NULL))
	{
		return NULL_POINTER;
	}

    /*
	 * ADC reference is already provided in mV.
	 * Example: 5000mV or 2560mV.
	 */
	u16 Local_u16ADCReference =
		lm35->Copy_u16ADCVoltageReference;

	if (Local_u16ADCReference == 0)
	{
		return NOK;
	}


    /*
     * Get ADC digital reading
     */
    Local_u8ErrorState = ADC_u8GetResultSync(
                            lm35->Copy_u8LM35Channel,
                            &Local_u16ADCResult
                         );


    /*
     * Check if ADC conversion was successful
     */
    if (Local_u8ErrorState != OK)
    {
        return Local_u8ErrorState;
    }


    /*
	 * ADC driver always returns a 10-bit result (0 to 1023),
	 * regardless of ADLAR configuration.
	 */
	Local_u16AnalogValue =
		(u16)(((u32)Local_u16ADCResult *
			   (u32)Local_u16ADCReference) / 1024UL);


    /*
     * Convert Voltage (mV) to Temperature (�C)
     *
     * LM35 output:
     * 10mV = 1�C
     */
    *Copy_u8TempValue = Local_u16AnalogValue / 10;


    return Local_u8ErrorState;
}

