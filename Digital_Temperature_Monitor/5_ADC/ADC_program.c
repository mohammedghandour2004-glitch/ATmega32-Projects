/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ADC_program.c >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 6, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : _MCAL
 * SWC   : ADC
 */


//=====================================================================================================================
#include "_LIB/BIT_MATH.h"
#include "_LIB/STD_TYPES.h"
#include "5_ADC/ADC_config.h"
#include "5_ADC/ADC_interface.h"
#include "5_ADC/ADC_private.h"

/*Hint : Must include ADC_CONFIG before ADC_PRIVATE*/

//=====================================================================================================================


/*Global pointer to carry the conversion result in the Asynchronous execution*/
static u16 * ADC_pu16AsynchConversionResult = NULL;

/*Global pointer to function to carry the notification function called by ISR*/
static void (* ADC_pvNotificationFunc)(void)= NULL;

/*Global flag for the ADC ISR State*/
static u8 ADC_u8ISRState ;

/*Global pointer to carry the chain Channel*/
static u8 * ADC_pu8ChainChannel ;

/*Global variable to carry the chain size*/
static u8 ADC_u8ChainSize ;

/*Global variable to indicate for the current ADC Chain*/
static u8 ADC_u8Index ;

/*Global flag for the ADC Busy State*/
static u8 ADC_u8State= IDLE ;

static u16 ADC_u16GetConversionResult(void);

//=====================================================================================================================

void ADC_voidInit(void)
{
	/*Select the voltage reference*/
	#if ADC_VREF == AREF
		CLR_BIT(ADMUX , ADMUX_REFS0) ;
		CLR_BIT(ADMUX , ADMUX_REFS1) ;

	#elif ADC_VREF == AVCC
		SET_BIT(ADMUX , ADMUX_REFS0) ;
		CLR_BIT(ADMUX , ADMUX_REFS1) ;

	#elif ADC_VREF == INTERNAL_2_56
		SET_BIT(ADMUX , ADMUX_REFS0) ;
		SET_BIT(ADMUX , ADMUX_REFS1) ;

	#else
		#error "Wrong ADC_VREF config"

	#endif


	/*Set Left Adjust Result*/
	#if ADC_ADJUSTMENT == RIGHT_ADJUSTMENT
			CLR_BIT(ADMUX , ADMUX_ADLAR) ;

	#elif ADC_ADJUSTMENT == LEFT_ADJUSTMENT
			SET_BIT(ADMUX , ADMUX_ADLAR) ;

	#else
		#error "Wrong ADC_ADJUSTMENT config"

	#endif

	/*Set Prescaler Value*/
	ADCSRA &= ADC_PRE_MASK ;
	ADCSRA |= ADC_PRESCALLER ;

	/* ENABLE The Peripheral & Interrupt  */

	/*Enable ADC Peripheral*/
	#if ADC_STATUS == ADC_DISABLE
		CLR_BIT(ADCSRA , ADCSRA_ADEN) ;
	#elif ADC_STATUS == ADC_ENABLE
		SET_BIT(ADCSRA , ADCSRA_ADEN) ;
	#else
	#error "Wrong ADC_STATUS config"
	#endif

	/*ADC interrupt is enabled only by asynchronous conversion APIs.*/
	CLR_BIT(ADCSRA , ADCSRA_ADIE) ;

}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void ADC_voidEnable (void)
{
	SET_BIT(ADCSRA , ADCSRA_ADEN) ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void ADC_voidDisable (void)
{
	CLR_BIT(ADCSRA , ADCSRA_ADEN) ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void ADC_voidInterruptEnable (void)
{
	SET_BIT(ADCSRA , ADCSRA_ADIE) ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void ADC_voidInterruptDisable (void)
{
	CLR_BIT(ADCSRA , ADCSRA_ADIE) ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

u8 ADC_u8SetPrescaler (u8 Copy_u8Prescaler)
{
	u8 Local_u8ErrorState = OK ;

	if (Copy_u8Prescaler < 8)
	{
		/*Set Prescaler Value*/
		ADCSRA &= ADC_PRE_MASK ;
		ADCSRA |= Copy_u8Prescaler ;
	}
	else
	{
		Local_u8ErrorState = NOK ;
	}

	return Local_u8ErrorState ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

u8 ADC_u8GetResultSync (u8 Copy_u8Channel , u16 * Copy_pu16Result)
{
	u8 Local_u8ErrorState = OK ;
	u32 Local_u32TimeoutCounter = 0 ;
	if (Copy_pu16Result == NULL)
	{
		Local_u8ErrorState = NULL_POINTER ;
	}
	else if (Copy_u8Channel > ADC_CHANNEL7)
	{
		Local_u8ErrorState = NOK ;
	}
	else
	{
		if (ADC_u8State == IDLE)
		{
			/*ADC is now Busy*/
			ADC_u8State = BUSY ;

			/*Set required channel*/
			ADMUX &= ADC_CH_MASK ;
			ADMUX |= Copy_u8Channel ;

			/*Start Conversion*/
			SET_BIT(ADCSRA , ADCSRA_ADSC) ;

			/*Waiting until the conversion is complete*/
			while (((GET_BIT(ADCSRA , ADCSRA_ADIF)) == 0) && (Local_u32TimeoutCounter < ADC_TIMEOUT))
			{
				Local_u32TimeoutCounter++ ;
			}
			if (Local_u32TimeoutCounter == ADC_TIMEOUT)
			{
				Local_u8ErrorState = TIMEOUT_STATE ;
			}
			else
			{
				/*Clear the interrupt flag*/
				SET_BIT(ADCSRA , ADCSRA_ADIF) ;

				/*Return Conversion Result*/
				*Copy_pu16Result = ADC_u16GetConversionResult() ;
			}

			/*ADC is IDLE*/
			ADC_u8State = IDLE ;
		}
		else
		{
			Local_u8ErrorState = BUSY_STATE ;
		}


	}
	return Local_u8ErrorState ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

u8 ADC_u8StartConversionAsynch (u8 Copy_u8Channel , u16 * Copy_pu16Result , void (*Copy_pvNotificationFunc)(void))
{
	u8 Local_u8ErrorState = OK ;

	if ((Copy_pu16Result == NULL) || (Copy_pvNotificationFunc == NULL))
	{
		Local_u8ErrorState = NULL_POINTER ;
	}
	else if (Copy_u8Channel > ADC_CHANNEL7)
	{
		Local_u8ErrorState = NOK ;
	}
	else
	{
		if (ADC_u8State == IDLE)
		{
			/*ADC is now Busy*/
			ADC_u8State = BUSY ;

			/*Set ISR State*/
			ADC_u8ISRState = SINGLE_CHANNEL_ASYNCH ;

			/*Initialize the global result pointer*/
			ADC_pu16AsynchConversionResult = Copy_pu16Result;

			/*Initialize the global notification function pointer*/
			ADC_pvNotificationFunc= Copy_pvNotificationFunc;

			/*Set required channel*/
			ADMUX &= ADC_CH_MASK ;
			ADMUX |= Copy_u8Channel ;

			/*Disable ADC interrupt while preparing the asynchronous request*/
			CLR_BIT(ADCSRA , ADCSRA_ADIE) ;

			/*Clear any old conversion complete flag before enabling ADC interrupt*/
			SET_BIT(ADCSRA , ADCSRA_ADIF) ;

			/*ADC Conversion Complete Interrupt Enable*/
			SET_BIT(ADCSRA , ADCSRA_ADIE) ;

			/*Start Conversion*/
			SET_BIT(ADCSRA , ADCSRA_ADSC) ;
		}
		else
		{
			Local_u8ErrorState = BUSY_STATE ;
		}
	}

	return Local_u8ErrorState ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

u8 ADC_u8StartChainAsynch (Chain_t * Copy_Chain)
{
	u8 Local_u8ErrorState = OK ;
	u8 Local_u8Index ;

	if ((Copy_Chain == NULL) || (Copy_Chain->Channel == NULL) || (Copy_Chain->NotificationFunc == NULL) || (Copy_Chain->Result == NULL))
	{
		Local_u8ErrorState = NULL_POINTER ;
	}
	else if (Copy_Chain->Size == 0)
	{
		Local_u8ErrorState = NOK ;
	}
	else
	{
		for (Local_u8Index = 0 ; Local_u8Index < Copy_Chain->Size ; Local_u8Index++)
		{
			if (Copy_Chain->Channel[Local_u8Index] > ADC_CHANNEL7)
			{
				Local_u8ErrorState = NOK ;
				break ;
			}
		}

		if ((Local_u8ErrorState == OK) && (ADC_u8State == IDLE))
		{
			/*ADC is now Busy*/
			ADC_u8State = BUSY ;

			/*Set ISR State*/
			ADC_u8ISRState = CHAIN_CHANNEL_ASYNCH ;

			/*Assign the ADC data globally*/
			ADC_pu16AsynchConversionResult = Copy_Chain->Result ;
			ADC_pu8ChainChannel = Copy_Chain->Channel ;
			ADC_u8ChainSize = Copy_Chain->Size ;
			ADC_pvNotificationFunc = Copy_Chain->NotificationFunc ;

			/*Set Index to first element*/
			ADC_u8Index = 0 ;

			/*Set required channel*/
			ADMUX &= ADC_CH_MASK ;
			ADMUX |= ADC_pu8ChainChannel[ADC_u8Index] ;

			/*Disable ADC interrupt while preparing the asynchronous request*/
			CLR_BIT(ADCSRA , ADCSRA_ADIE) ;

			/*Clear any old conversion complete flag before enabling ADC interrupt*/
			SET_BIT(ADCSRA , ADCSRA_ADIF) ;

			/*ADC Conversion Complete Interrupt Enable*/
			SET_BIT(ADCSRA , ADCSRA_ADIE) ;

			/*Start Conversion*/
			SET_BIT(ADCSRA , ADCSRA_ADSC) ;
		}
		else if (Local_u8ErrorState == OK)
		{
			Local_u8ErrorState = BUSY_STATE ;
		}

	}
	return Local_u8ErrorState ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

/* ISR for ADC conversion complete */
void __vector_16 (void)__attribute__((signal)) ;
void __vector_16 (void)
{
	if (ADC_u8ISRState == SINGLE_CHANNEL_ASYNCH)
	{
		/*Return Conversion Result*/
		*ADC_pu16AsynchConversionResult = ADC_u16GetConversionResult() ;

		/*Disable the Conversion Complete Interrupt*/
		CLR_BIT(ADCSRA , ADCSRA_ADIE) ;

		/*ADC is IDLE*/
		ADC_u8State = IDLE ;

		/*Call Notification Function*/
		ADC_pvNotificationFunc() ;
	}
	else
	{
		/*Return Conversion Result*/
		ADC_pu16AsynchConversionResult[ADC_u8Index] = ADC_u16GetConversionResult() ;

		/*Increment Data index of the chain*/
		ADC_u8Index++ ;

		if (ADC_u8Index == ADC_u8ChainSize)
		{
			/*Disable the Conversion Complete Interrupt*/
			CLR_BIT(ADCSRA , ADCSRA_ADIE) ;

			/*ADC is IDLE*/
			ADC_u8State = IDLE ;

			/*Call Notification Function*/
			ADC_pvNotificationFunc() ;
		}
		else
		{
			/*Set required channel*/
			ADMUX &= ADC_CH_MASK ;
			ADMUX |= ADC_pu8ChainChannel[ADC_u8Index] ;

			/*Start Conversion*/
			SET_BIT(ADCSRA , ADCSRA_ADSC) ;
		}
	}
}

static u16 ADC_u16GetConversionResult(void)
{
	u8 Local_u8ADCL = ADCL ;
	u8 Local_u8ADCH = ADCH ;

	#if ADC_ADJUSTMENT == RIGHT_ADJUSTMENT
		return ((u16)Local_u8ADCL | ((u16)Local_u8ADCH << 8)) ;

	#elif ADC_ADJUSTMENT == LEFT_ADJUSTMENT
		return (((u16)Local_u8ADCH << 2) | (Local_u8ADCL >> 6)) ;
	#else
		#error "Wrong ADC_ADJUSTMENT config"
	#endif
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
