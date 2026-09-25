/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< main.c >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 26, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : APP
 */

#include "_LIB/STD_TYPES.h"

#include "1_PORT/PORT_interface.h"
#include "5_CLCD/CLCD_interface.h"
#include "6_KPD/KPD_interface.h"

#define CALC_MAX_EXPRESSION_LENGTH  32U
#define CALC_LCD_ROW_LENGTH          16U
#define CALC_MAX_STACK_DEPTH         ((CALC_MAX_EXPRESSION_LENGTH + 1U) / 2U)
#define CALC_S32_MAX                 2147483647L
#define CALC_S32_MIN                 (-2147483647L - 1L)

typedef enum
{
	CALC_ENTERING_EXPRESSION,
	CALC_RESULT_DISPLAYED,
	CALC_ERROR
} CalculatorState_t;

typedef enum
{
	CALC_EVAL_OK,
	CALC_EVAL_DIVISION_BY_ZERO,
	CALC_EVAL_MATH_ERROR,
	CALC_EVAL_INVALID_EXPRESSION
} CalculatorEvalStatus_t;

static CalculatorState_t APP_enuState;
static u8  APP_au8Expression[CALC_MAX_EXPRESSION_LENGTH + 1U];
static u8  APP_u8ExpressionLength;
static s32 APP_s32LastResult;

static const u8 APP_au8DivisionByZero[] = "DIV BY ZERO";
static const u8 APP_au8MathError[] = "MATH ERROR";
static const u8 APP_au8ExpressionFull[] = "EXPR FULL";
static const u8 APP_au8InputError[] = "INPUT ERROR";

static void APP_voidResetCalculator(void);
static void APP_voidProcessKey(u8 Copy_u8Key);
static u8   APP_u8IsDigit(u8 Copy_u8Key);
static u8   APP_u8IsOperator(u8 Copy_u8Key);
static u8   APP_u8AppendCharacter(u8 Copy_u8Character);
static void APP_voidAppendSignedNumber(s32 Copy_s32Number);
static void APP_voidStartExpressionFromResult(void);
static void APP_voidDisplayExpression(void);
static void APP_voidDisplaySignedNumber(s32 Copy_s32Number);
static void APP_voidDisplayResult(s32 Copy_s32Result);
static void APP_voidShowError(const u8 * Copy_pu8Message);
static u8   APP_u8GetPrecedence(u8 Copy_u8Operator);
static CalculatorEvalStatus_t APP_enuExecuteOperation(s32 Copy_s32Left,
		s32 Copy_s32Right, u8 Copy_u8Operator, s32 * Copy_ps32Result);
static CalculatorEvalStatus_t APP_enuReduceTopOperator(s32 * Copy_ps32ValueStack,
		u8 * Copy_pu8ValueCount, u8 * Copy_pu8OperatorStack,
		u8 * Copy_pu8OperatorCount);
static CalculatorEvalStatus_t APP_enuEvaluateExpression(s32 * Copy_ps32Result);

int main(void)
{
	u8 LOC_u8Key;

	PORT_VoidInit();
	CLCD_VoidInit();
	KPD_Init();

	APP_voidResetCalculator();
	CLCD_VoidClearScreen();

	while (1)
	{
		LOC_u8Key = KPD_u8GetPressed();

		if (LOC_u8Key != NOTPRESSED)
		{
			APP_voidProcessKey(LOC_u8Key);
		}
	}
}

/* Reset the expression, result and calculator state. */
static void APP_voidResetCalculator(void)
{
	APP_enuState = CALC_ENTERING_EXPRESSION;
	APP_u8ExpressionLength = 0U;
	APP_au8Expression[0] = '\0';
	APP_s32LastResult = 0L;
}

/* Process one valid keypad event for the current calculator state. */
static void APP_voidProcessKey(u8 Copy_u8Key)
{
	CalculatorEvalStatus_t LOC_enuEvalStatus;
	s32 LOC_s32Result;

	if (Copy_u8Key == '?')
	{
		APP_voidResetCalculator();
		CLCD_VoidClearScreen();
		return;
	}

	/* Errors remain visible until the user presses Clear. */
	if (APP_enuState == CALC_ERROR)
	{
		return;
	}

	if (APP_u8IsDigit(Copy_u8Key) == 1U)
	{
		/* A digit after '=' begins a new expression. */
		if (APP_enuState == CALC_RESULT_DISPLAYED)
		{
			APP_voidResetCalculator();
		}

		if (APP_u8AppendCharacter(Copy_u8Key) == 1U)
		{
			APP_voidDisplayExpression();
		}
		else
		{
			APP_voidShowError(APP_au8ExpressionFull);
		}
	}
	else if (APP_u8IsOperator(Copy_u8Key) == 1U)
	{
		/* An operator after '=' continues from the previous result. */
		if (APP_enuState == CALC_RESULT_DISPLAYED)
		{
			APP_voidStartExpressionFromResult();
		}

		if ((APP_u8ExpressionLength > 0U) &&
			(APP_u8IsDigit(APP_au8Expression[APP_u8ExpressionLength - 1U]) == 1U))
		{
			if (APP_u8AppendCharacter(Copy_u8Key) == 1U)
			{
				APP_voidDisplayExpression();
			}
			else
			{
				APP_voidShowError(APP_au8ExpressionFull);
			}
		}
	}
	else if (Copy_u8Key == '=')
	{
		if ((APP_enuState == CALC_ENTERING_EXPRESSION) &&
			(APP_u8ExpressionLength > 0U) &&
			(APP_u8IsDigit(APP_au8Expression[APP_u8ExpressionLength - 1U]) == 1U))
		{
			LOC_enuEvalStatus = APP_enuEvaluateExpression(&LOC_s32Result);

			if (LOC_enuEvalStatus == CALC_EVAL_OK)
			{
				APP_s32LastResult = LOC_s32Result;
				APP_voidDisplayResult(LOC_s32Result);
				APP_enuState = CALC_RESULT_DISPLAYED;
			}
			else if (LOC_enuEvalStatus == CALC_EVAL_DIVISION_BY_ZERO)
			{
				APP_voidShowError(APP_au8DivisionByZero);
			}
			else if (LOC_enuEvalStatus == CALC_EVAL_MATH_ERROR)
			{
				APP_voidShowError(APP_au8MathError);
			}
			else
			{
				APP_voidShowError(APP_au8InputError);
			}
		}
	}
}

/* Check whether a keypad key is a decimal digit. */
static u8 APP_u8IsDigit(u8 Copy_u8Key)
{
	return ((Copy_u8Key >= '0') && (Copy_u8Key <= '9'));
}

/* Check whether a keypad key is a supported binary operator. */
static u8 APP_u8IsOperator(u8 Copy_u8Key)
{
	return ((Copy_u8Key == '+') || (Copy_u8Key == '-') ||
			(Copy_u8Key == '*') || (Copy_u8Key == '/'));
}

/* Append one input character while preserving buffer termination. */
static u8 APP_u8AppendCharacter(u8 Copy_u8Character)
{
	if (APP_u8ExpressionLength >= CALC_MAX_EXPRESSION_LENGTH)
	{
		return 0U;
	}

	APP_au8Expression[APP_u8ExpressionLength] = Copy_u8Character;
	APP_u8ExpressionLength++;
	APP_au8Expression[APP_u8ExpressionLength] = '\0';
	return 1U;
}

/* Seed a continuation expression with the previously displayed result. */
static void APP_voidAppendSignedNumber(s32 Copy_s32Number)
{
	u8 LOC_au8Digits[10];
	u8 LOC_u8DigitCount = 0U;
	u32 LOC_u32Magnitude;

	if (Copy_s32Number < 0L)
	{
		APP_u8AppendCharacter('-');
		LOC_u32Magnitude = 0UL - (u32)Copy_s32Number;
	}
	else
	{
		LOC_u32Magnitude = (u32)Copy_s32Number;
	}

	do
	{
		LOC_au8Digits[LOC_u8DigitCount] = (u8)(LOC_u32Magnitude % 10UL);
		LOC_u8DigitCount++;
		LOC_u32Magnitude /= 10UL;
	} while (LOC_u32Magnitude != 0UL);

	while (LOC_u8DigitCount > 0U)
	{
		LOC_u8DigitCount--;
		APP_u8AppendCharacter((u8)(LOC_au8Digits[LOC_u8DigitCount] + '0'));
	}
}

/* Replace the completed expression with the previous result. */
static void APP_voidStartExpressionFromResult(void)
{
	APP_u8ExpressionLength = 0U;
	APP_au8Expression[0] = '\0';
	APP_enuState = CALC_ENTERING_EXPRESSION;
	APP_voidAppendSignedNumber(APP_s32LastResult);
}

/* Display the newest 16-character window of the current expression. */
static void APP_voidDisplayExpression(void)
{
	u8 LOC_u8Index;
	u8 LOC_u8StartIndex = 0U;

	if (APP_u8ExpressionLength > CALC_LCD_ROW_LENGTH)
	{
		LOC_u8StartIndex = APP_u8ExpressionLength - CALC_LCD_ROW_LENGTH;
	}

	CLCD_VoidClearScreen();
	CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);

	for (LOC_u8Index = LOC_u8StartIndex;
		 LOC_u8Index < APP_u8ExpressionLength;
		 LOC_u8Index++)
	{
		CLCD_VoidSendData(APP_au8Expression[LOC_u8Index]);
	}
}

/* Send a signed result using the LCD's unsigned number API. */
static void APP_voidDisplaySignedNumber(s32 Copy_s32Number)
{
	u32 LOC_u32Magnitude;

	if (Copy_s32Number < 0L)
	{
		CLCD_VoidSendData('-');
		LOC_u32Magnitude = 0UL - (u32)Copy_s32Number;
	}
	else
	{
		LOC_u32Magnitude = (u32)Copy_s32Number;
	}

	CLCD_VoidSendNumber((u64)LOC_u32Magnitude);
}

/* Display the expression and its evaluated result. */
static void APP_voidDisplayResult(s32 Copy_s32Result)
{
	APP_voidDisplayExpression();
	CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
	CLCD_VoidSendData('=');
	APP_voidDisplaySignedNumber(Copy_s32Result);
}

/* Display an error and lock input until Clear is pressed. */
static void APP_voidShowError(const u8 * Copy_pu8Message)
{
	CLCD_VoidClearScreen();
	CLCD_VoidSendString(Copy_pu8Message);
	APP_enuState = CALC_ERROR;
}

/* Return the precedence used by the expression evaluator. */
static u8 APP_u8GetPrecedence(u8 Copy_u8Operator)
{
	if ((Copy_u8Operator == '*') || (Copy_u8Operator == '/'))
	{
		return 2U;
	}

	if ((Copy_u8Operator == '+') || (Copy_u8Operator == '-'))
	{
		return 1U;
	}

	return 0U;
}

/* Execute one operation with an s64 intermediate to prevent s32 overflow. */
static CalculatorEvalStatus_t APP_enuExecuteOperation(s32 Copy_s32Left,
		s32 Copy_s32Right, u8 Copy_u8Operator, s32 * Copy_ps32Result)
{
	s64 LOC_s64Result;

	if ((Copy_u8Operator == '/') && (Copy_s32Right == 0L))
	{
		return CALC_EVAL_DIVISION_BY_ZERO;
	}

	switch (Copy_u8Operator)
	{
		case '+':
			LOC_s64Result = (s64)Copy_s32Left + (s64)Copy_s32Right;
			break;

		case '-':
			LOC_s64Result = (s64)Copy_s32Left - (s64)Copy_s32Right;
			break;

		case '*':
			LOC_s64Result = (s64)Copy_s32Left * (s64)Copy_s32Right;
			break;

		case '/':
			LOC_s64Result = (s64)Copy_s32Left / (s64)Copy_s32Right;
			break;

		default:
			return CALC_EVAL_INVALID_EXPRESSION;
	}

	if ((LOC_s64Result > (s64)CALC_S32_MAX) ||
		(LOC_s64Result < (s64)CALC_S32_MIN))
	{
		return CALC_EVAL_MATH_ERROR;
	}

	*Copy_ps32Result = (s32)LOC_s64Result;
	return CALC_EVAL_OK;
}

/* Reduce the top operator and its two values from the evaluation stacks. */
static CalculatorEvalStatus_t APP_enuReduceTopOperator(s32 * Copy_ps32ValueStack,
		u8 * Copy_pu8ValueCount, u8 * Copy_pu8OperatorStack,
		u8 * Copy_pu8OperatorCount)
{
	CalculatorEvalStatus_t LOC_enuStatus;
	s32 LOC_s32Left;
	s32 LOC_s32Right;
	s32 LOC_s32Result;
	u8 LOC_u8Operator;

	if ((*Copy_pu8ValueCount < 2U) || (*Copy_pu8OperatorCount == 0U))
	{
		return CALC_EVAL_INVALID_EXPRESSION;
	}

	*Copy_pu8OperatorCount -= 1U;
	LOC_u8Operator = Copy_pu8OperatorStack[*Copy_pu8OperatorCount];

	*Copy_pu8ValueCount -= 1U;
	LOC_s32Right = Copy_ps32ValueStack[*Copy_pu8ValueCount];
	*Copy_pu8ValueCount -= 1U;
	LOC_s32Left = Copy_ps32ValueStack[*Copy_pu8ValueCount];

	LOC_enuStatus = APP_enuExecuteOperation(LOC_s32Left, LOC_s32Right,
			LOC_u8Operator, &LOC_s32Result);
	if (LOC_enuStatus != CALC_EVAL_OK)
	{
		return LOC_enuStatus;
	}

	Copy_ps32ValueStack[*Copy_pu8ValueCount] = LOC_s32Result;
	*Copy_pu8ValueCount += 1U;
	return CALC_EVAL_OK;
}

/* Evaluate the complete expression using fixed value and operator stacks. */
static CalculatorEvalStatus_t APP_enuEvaluateExpression(s32 * Copy_ps32Result)
{
	s32 LOC_as32ValueStack[CALC_MAX_STACK_DEPTH];
	u8 LOC_au8OperatorStack[CALC_MAX_STACK_DEPTH];
	u8 LOC_u8ValueCount = 0U;
	u8 LOC_u8OperatorCount = 0U;
	u8 LOC_u8Index = 0U;
	u8 LOC_u8ExpectNumber = 1U;
	u8 LOC_u8Negative;
	u8 LOC_u8Digit;
	u32 LOC_u32Magnitude;
	u32 LOC_u32Limit;
	s32 LOC_s32Number;
	CalculatorEvalStatus_t LOC_enuStatus;

	while (LOC_u8Index < APP_u8ExpressionLength)
	{
		if ((APP_u8IsDigit(APP_au8Expression[LOC_u8Index]) == 1U) ||
			((LOC_u8Index == 0U) && (APP_au8Expression[LOC_u8Index] == '-')))
		{
			if (LOC_u8ExpectNumber == 0U)
			{
				return CALC_EVAL_INVALID_EXPRESSION;
			}

			LOC_u8Negative = 0U;
			if (APP_au8Expression[LOC_u8Index] == '-')
			{
				LOC_u8Negative = 1U;
				LOC_u8Index++;
			}

			if ((LOC_u8Index >= APP_u8ExpressionLength) ||
				(APP_u8IsDigit(APP_au8Expression[LOC_u8Index]) == 0U))
			{
				return CALC_EVAL_INVALID_EXPRESSION;
			}

			LOC_u32Magnitude = 0UL;
			LOC_u32Limit = (LOC_u8Negative == 1U) ? 2147483648UL : (u32)CALC_S32_MAX;

			/* Parse a complete multi-digit number without overflow. */
			while ((LOC_u8Index < APP_u8ExpressionLength) &&
				   (APP_u8IsDigit(APP_au8Expression[LOC_u8Index]) == 1U))
			{
				LOC_u8Digit = APP_au8Expression[LOC_u8Index] - '0';
				if ((LOC_u32Magnitude > (LOC_u32Limit / 10UL)) ||
					((LOC_u32Magnitude == (LOC_u32Limit / 10UL)) &&
					 (LOC_u8Digit > (LOC_u32Limit % 10UL))))
				{
					return CALC_EVAL_MATH_ERROR;
				}

				LOC_u32Magnitude = (LOC_u32Magnitude * 10UL) + LOC_u8Digit;
				LOC_u8Index++;
			}

			if ((LOC_u8Negative == 1U) && (LOC_u32Magnitude == 2147483648UL))
			{
				LOC_s32Number = CALC_S32_MIN;
			}
			else if (LOC_u8Negative == 1U)
			{
				LOC_s32Number = -(s32)LOC_u32Magnitude;
			}
			else
			{
				LOC_s32Number = (s32)LOC_u32Magnitude;
			}

			if (LOC_u8ValueCount >= CALC_MAX_STACK_DEPTH)
			{
				return CALC_EVAL_INVALID_EXPRESSION;
			}

			LOC_as32ValueStack[LOC_u8ValueCount] = LOC_s32Number;
			LOC_u8ValueCount++;
			LOC_u8ExpectNumber = 0U;
		}
		else if (APP_u8IsOperator(APP_au8Expression[LOC_u8Index]) == 1U)
		{
			if (LOC_u8ExpectNumber == 1U)
			{
				return CALC_EVAL_INVALID_EXPRESSION;
			}

			/* Equal precedence is reduced first for left-to-right behavior. */
			while ((LOC_u8OperatorCount > 0U) &&
				   (APP_u8GetPrecedence(LOC_au8OperatorStack[LOC_u8OperatorCount - 1U]) >=
					APP_u8GetPrecedence(APP_au8Expression[LOC_u8Index])))
			{
				LOC_enuStatus = APP_enuReduceTopOperator(LOC_as32ValueStack,
						&LOC_u8ValueCount, LOC_au8OperatorStack, &LOC_u8OperatorCount);
				if (LOC_enuStatus != CALC_EVAL_OK)
				{
					return LOC_enuStatus;
				}
			}

			if (LOC_u8OperatorCount >= CALC_MAX_STACK_DEPTH)
			{
				return CALC_EVAL_INVALID_EXPRESSION;
			}

			LOC_au8OperatorStack[LOC_u8OperatorCount] = APP_au8Expression[LOC_u8Index];
			LOC_u8OperatorCount++;
			LOC_u8Index++;
			LOC_u8ExpectNumber = 1U;
		}
		else
		{
			return CALC_EVAL_INVALID_EXPRESSION;
		}
	}

	if (LOC_u8ExpectNumber == 1U)
	{
		return CALC_EVAL_INVALID_EXPRESSION;
	}

	while (LOC_u8OperatorCount > 0U)
	{
		LOC_enuStatus = APP_enuReduceTopOperator(LOC_as32ValueStack,
				&LOC_u8ValueCount, LOC_au8OperatorStack, &LOC_u8OperatorCount);
		if (LOC_enuStatus != CALC_EVAL_OK)
		{
			return LOC_enuStatus;
		}
	}

	if (LOC_u8ValueCount != 1U)
	{
		return CALC_EVAL_INVALID_EXPRESSION;
	}

	*Copy_ps32Result = LOC_as32ValueStack[0];
	return CALC_EVAL_OK;
}
