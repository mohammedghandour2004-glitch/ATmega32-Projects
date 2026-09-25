/*
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< main.c >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Created on: September 22, 2026
 * Author: MOHAMMED GHANDOUR
 * Layer : APP
 */

#define F_CPU 8000000UL
#include <util/delay.h>

#include "../_LIB/STD_TYPES.h"
#include "../2_DIO/DIO_interface.h"
#include "../5_CLCD/CLCD_interface.h"
#include "../6_KPD/KPD_interface.h"

#define PASSWORD_LENGTH 4
#define MAX_FAILED_ATTEMPTS 3
#define UNLOCK_TIME_MS 5000
#define DENIED_TIME_MS 1000
#define ALARM_TIME_MS 10000

#define GREEN_LED_PORT DIO_PORTB
#define GREEN_LED_PIN  DIO_PIN0
#define BUZZER_PORT    DIO_PORTC
#define BUZZER_PIN     DIO_PIN0
#define RED_LED_PORT   DIO_PORTC
#define RED_LED_PIN    DIO_PIN1

typedef enum
{
    PASSWORD_ENTRY,
    ACCESS_GRANTED,
    ACCESS_DENIED,
    ALARM
} AppState_t;

static u8 g_password[PASSWORD_LENGTH] = {'2', '0', '0', '4'};
static u8 g_inputBuffer[PASSWORD_LENGTH] = {0};
static u8 g_digitCount = 0;
static u8 g_failedAttempts = 0;
static AppState_t g_state = PASSWORD_ENTRY;

static void App_VoidShowPasswordPrompt(void)
{
    CLCD_VoidClearScreen();
    CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
    CLCD_VoidSendString((const u8 *)"SECURE ACCESS");
    CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
    CLCD_VoidSendString((const u8 *)"Enter PIN:");
}

static void App_VoidResetInput(void)
{
    u8 i;
    for (i = 0; i < PASSWORD_LENGTH; i++)
    {
        g_inputBuffer[i] = 0;
    }
    g_digitCount = 0;
}

static void App_VoidShowPasswordMask(void)
{
    u8 i;
    CLCD_VoidClearScreen();
    CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
    CLCD_VoidSendString((const u8 *)"ENTER PIN");
    CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
    CLCD_VoidSendString((const u8 *)"PIN: ");
    for (i = 0; i < g_digitCount; i++)
    {
        CLCD_VoidSendData('*');
    }
}

int main(void)
{
    u8 key;
    u8 i;
    u8 passwordMatched;

    CLCD_VoidInit();
    KPD_Init();

    DIO_EnumSetPinDirection(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_OUTPUT);
    DIO_EnumSetPinDirection(BUZZER_PORT, BUZZER_PIN, DIO_PIN_OUTPUT);
    DIO_EnumSetPinDirection(RED_LED_PORT, RED_LED_PIN, DIO_PIN_OUTPUT);

    DIO_EnumSetPinValue(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_LOW);
    DIO_EnumSetPinValue(BUZZER_PORT, BUZZER_PIN, DIO_PIN_LOW);
    DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_LOW);

    g_failedAttempts = 0;
    App_VoidResetInput();
    g_state = PASSWORD_ENTRY;

    while (1)
    {
        switch (g_state)
        {
        case PASSWORD_ENTRY:
            App_VoidShowPasswordPrompt();
            while (g_state == PASSWORD_ENTRY)
            {
                key = KPD_u8GetPressed();

                if (key == NOTPRESSED)
                {
                    continue;
                }

                if ((key < '0') || (key > '9'))
                {
                    continue;
                }

                if (g_digitCount < PASSWORD_LENGTH)
                {
                    g_inputBuffer[g_digitCount] = key;
                    g_digitCount++;
                    App_VoidShowPasswordMask();
                }

                if (g_digitCount == PASSWORD_LENGTH)
                {
                    passwordMatched = 1;
                    for (i = 0; i < PASSWORD_LENGTH; i++)
                    {
                        if (g_inputBuffer[i] != g_password[i])
                        {
                            passwordMatched = 0;
                            break;
                        }
                    }

                    if (passwordMatched == 1)
                    {
                        g_state = ACCESS_GRANTED;
                    }
                    else
                    {
                        g_state = ACCESS_DENIED;
                    }
                }
            }
            break;

        case ACCESS_GRANTED:
            CLCD_VoidClearScreen();
            CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"ACCESS GRANTED");
            CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"WELCOME!");

            DIO_EnumSetPinValue(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_HIGH);
            DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_LOW);
            DIO_EnumSetPinValue(BUZZER_PORT, BUZZER_PIN, DIO_PIN_LOW);
            _delay_ms(UNLOCK_TIME_MS);
            DIO_EnumSetPinValue(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_LOW);

            g_failedAttempts = 0;
            App_VoidResetInput();
            g_state = PASSWORD_ENTRY;
            break;

        case ACCESS_DENIED:
            CLCD_VoidClearScreen();
            CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"ACCESS DENIED");
            CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"TRY AGAIN...");

            g_failedAttempts++;
            DIO_EnumSetPinValue(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_LOW);
            DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_HIGH);
            DIO_EnumSetPinValue(BUZZER_PORT, BUZZER_PIN, DIO_PIN_LOW);
            _delay_ms(DENIED_TIME_MS);

            DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_LOW);

            App_VoidResetInput();
            if (g_failedAttempts >= MAX_FAILED_ATTEMPTS)
            {
                g_state = ALARM;
            }
            else
            {
                g_state = PASSWORD_ENTRY;
            }
            break;

        case ALARM:
            CLCD_VoidClearScreen();
            CLCD_VoidSetPosition(CLCD_ROW_1, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"!! SECURITY !!");
            CLCD_VoidSetPosition(CLCD_ROW_2, CLCD_COL_1);
            CLCD_VoidSendString((const u8 *)"SYSTEM LOCKED");

            DIO_EnumSetPinValue(GREEN_LED_PORT, GREEN_LED_PIN, DIO_PIN_LOW);
            DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_HIGH);
            DIO_EnumSetPinValue(BUZZER_PORT, BUZZER_PIN, DIO_PIN_HIGH);
            _delay_ms(ALARM_TIME_MS);
            DIO_EnumSetPinValue(RED_LED_PORT, RED_LED_PIN, DIO_PIN_LOW);
            DIO_EnumSetPinValue(BUZZER_PORT, BUZZER_PIN, DIO_PIN_LOW);

            g_failedAttempts = 0;
            App_VoidResetInput();
            g_state = PASSWORD_ENTRY;
            break;

        default:
            g_state = PASSWORD_ENTRY;
            break;
        }
    }

    return 0;
}

