/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< main.c >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 25, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : _APP
 * SWC   : Application
 */

#define F_CPU 8000000UL
#include <util/delay.h>

#include "_LIB/STD_TYPES.h"

#include "1_PORT/PORT_interface.h"
#include "5_ADC/ADC_interface.h"
#include "5_CLCD/CLCD_interface.h"
#include "7_LM35/LM35_interface.h"


int main(void)
{
	/* Configuration */
	LM35_CONFIG LOC_LM35Config = {ADC_CHANNEL0, 5000};
	u8 LOC_u8Temperature = 0;
	u8 LOC_u8ErrorState = OK;

	/* Initialization */
	PORT_voidInit();
	ADC_voidInit ();
	CLCD_VoidInit();

	/* Static LCD Content */
	CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
	CLCD_VoidSendString((const u8 *)"Temperature:");

	/* Super Loop */
	while (1)
	{
		/* Sensor Read */
		LOC_u8ErrorState = LM35_u8GetTemp(&LOC_LM35Config, &LOC_u8Temperature);

		/* Display Update */
		CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);

		if (LOC_u8ErrorState == OK)
		{
			/* Keep the temperature field three characters wide. */
			if (LOC_u8Temperature < 100)
			{
				CLCD_VoidSendData(' ');
			}

			if (LOC_u8Temperature < 10)
			{
				CLCD_VoidSendData(' ');
			}

			CLCD_VoidSendNumber(LOC_u8Temperature);
			CLCD_VoidSendString((const u8 *)" C ");

			if (LOC_u8Temperature < 30)
			{
				CLCD_VoidSendString((const u8 *)"NORMAL    ");
			}
			else if (LOC_u8Temperature < 40)
			{
				CLCD_VoidSendString((const u8 *)"WARNING   ");
			}
			else
			{
				CLCD_VoidSendString((const u8 *)"DANGER    ");
			}
		}
		else
		{
			CLCD_VoidSendString((const u8 *)"Sensor Error    ");
		}

		/* 500 ms Refresh Delay */
		_delay_ms(250);
		_delay_ms(250);
	}
}
