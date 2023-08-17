/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "definitions.h"
#include "app.h"
//#include "pmm.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
//uint8_t macBuffer[16]={'A','B','C','D','A','B','C','D','A','B','C','D','A','B','C','D'};
uint8_t macBuffer[5]={22,33,44,55,22};
//uint8_t macBuffer[16]={22,33,44,55,22,33,44,55,22,33,44,55,22,33,44,55};

bool radio_transmission_active=false;
//bool DIOStatus = 0;
SleepCallback_t mlsAppSleepWakeupCallback;
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
PMM_SleepReq_t defaultSleep;


// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/
static void trapTriggerCallback(void);

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    defaultSleep.sleepTimeMs =5000;
    defaultSleep.sleep_mode = SLEEP_MODE_STANDBY;
    defaultSleep.pmmWakeupCallback = MlsAppSleepCallback;
    appData.trappedMice=0;
    appData.batteryVoltage=0;
    
    EIC_CallbackRegister(EIC_PIN_6, (EIC_CALLBACK)trapTriggerCallback, (uintptr_t)NULL);
    EIC_InterruptDisable(EIC_PIN_6);
    
    EIC_CallbackRegister(EIC_PIN_7, (EIC_CALLBACK)trapTriggerCallback, (uintptr_t)NULL);
    EIC_InterruptDisable(EIC_PIN_7);
    //EIC_InterruptEnable(EIC_PIN_7);
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            RADIO_Init();
            
            PORT_PinOutputEnable(PORT_PIN_PB23);
            if (appInitialized)
            {
                
                
                /* Do other things */
                appData.state = APP_STATE_GET_MICE_COUNT;
            }
            break;
        }
        case APP_STATE_GET_MICE_COUNT:
        {   
            //delay_ms(500);
            
            appData.trappedMice=readMouseTraps();    
            appData.state = APP_STATE_GET_BATTERY_VOLTAGE;
            break;
        }

        case APP_STATE_GET_BATTERY_VOLTAGE: //This should be skipped 49/50 times we run the events, no need to check often.
        {
            ADC_Initialize();
            ADC_Enable();
            ADC_ConversionStart();
            while(!ADC_ConversionSequenceIsFinished())
            {
                
            }
            appData.batteryVoltage=ADC_ConversionResultGet()>>8;
            appData.state = APP_STATE_REPORT;
            break;
        }
        case APP_STATE_REPORT:
        {          
            buildLoraMessage(macBuffer);
            RadioTransmitParam_t RadioTransmitParam;
            ConfigureRadioTx();
            RadioTransmitParam.bufferLen = 5;
            RadioTransmitParam.bufferPtr = macBuffer;
            //resend the last packet
            if (RADIO_Transmit(&RadioTransmitParam) == ERR_NONE)
            {
                radio_transmission_active=true;
                PORT_PinWrite(PORT_PIN_PB23, true);
            }
            else
            {
                
                delay_ms(1000);
                PORT_PinWrite(PORT_PIN_PB23, false);
                delay_ms(100);
                PORT_PinWrite(PORT_PIN_PB23, true);
                delay_ms(100);
                PORT_PinWrite(PORT_PIN_PB23, false);
                delay_ms(100);
                PORT_PinWrite(PORT_PIN_PB23, true);
                delay_ms(100);
            }
            
            appData.state = APP_STATE_ENTER_SLEEP;
            
            break;
            }
        case APP_STATE_ENTER_SLEEP:
        {   
            EIC_InterruptEnable(EIC_PIN_6);
            EIC_InterruptEnable(EIC_PIN_7);
            if (MlsAppSleep() == PMM_SLEEP_REQ_DENIED) {
                
            }
            else{
                PORT_PinWrite(PORT_PIN_PB23, false);
            }
            
            break;
        }

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

//We wake up and go through the app states to see if there is any trapped mice.
static void trapTriggerCallback(void)
{
//  if (interruptHandlerDio0)
//  {

    PMM_Wakeup(0xFFFFFFFF, (uintptr_t)NULL);
//    interruptHandlerDio0();
//  }
}

uint8_t readMouseTraps(void)
{
    uint8_t trapped =0;
    uint8_t sw1=0;
        uint8_t sw2=0;

    SW1_COM_OutputEnable();
    SW1_COM_Set();
    SW2_COM_OutputEnable();
    SW2_COM_Set();
    sw1=NC_SW1_Get();
    sw2=NC_SW2_Get();
    trapped = sw1+sw2;
    return trapped;
}

void buildLoraMessage(uint8_t *message)
{
    message[0]=0x70;//appData.unitID;
    message[1]=MESSAGE_ID;
    message[2]=appData.trappedMice;
    message[3]=appData.batteryVoltage;
    message[4]=222;//appData.sleepTime;
}

PMM_Status_t MlsAppSleep(void)
{
    PMM_Status_t stat = PMM_SLEEP_REQ_DENIED;
//    PMM_SleepReq_t sleepReq;
//
//    /* Put the application to sleep */
//    sleepReq.sleepTimeMs = 2000;
//    sleepReq.pmmWakeupCallback = MlsAppSleepCallback;
//    sleepReq.sleep_mode = SLEEP_MODE_STANDBY;

    if(RADIO_STATE_IDLE== RADIO_GetState())
    {
                //delay_ms(500);
                //radio_transmission_active=false;
                appData.state = APP_STATE_GET_MICE_COUNT;
                PORT_PinWrite(PORT_PIN_PB23, false);
        MlsAppResourceDeinitialize();
        HAL_DisbleDIO0Interrupt();
        if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&defaultSleep))
        {
            MlsAppResourceInitialize();
            
        }
        else
        {
            stat = PMM_SLEEP_REQ_PROCESSED;
        }
        delay_ms(1);
    }

    return stat;
}
//------------------------------------------------------------------------------

void MlsAppSleepCallback(uint32_t sleptDuration)
{
    MlsAppResourceInitialize();
    appData.state = APP_STATE_INIT;
    if (mlsAppSleepWakeupCallback)
    {
        mlsAppSleepWakeupCallback(sleptDuration);
    }
}
//------------------------------------------------------------------------------

void MlsAppSleepCallbackNotifySet(SleepCallback_t func)
{
    mlsAppSleepWakeupCallback = func;
}
//------------------------------------------------------------------------------

void MlsAppResourceInitialize(void)
{
    INTERRUPT_GlobalInterruptDisable();
    PORT_Initialize();

    SERCOM4_SPI_Initialize();

    //SERCOM0_USART_Initialize();

    HAL_RadioInit();

    HAL_Radio_resources_init();

    //MlsAppSerialInitialize();
    INTERRUPT_GlobalInterruptEnable();
}
//------------------------------------------------------------------------------

void MlsAppResourceDeinitialize(void)
{
//    while (SERCOM0_USART_WriteIsBusy());
//
//    SERCOM0_USART_TransmitterDisable();
//    SERCOM0_USART_ReceiverDisable();
//
//    SERCOM0_REGS->USART_INT.SERCOM_CTRLA &= ~(SERCOM_USART_INT_CTRLA_ENABLE_Msk);
//
//    while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY);

    /* Disable Transceiver SPI Module */
    HAL_RadioDeInit();
}

//void MlsAppSleepCallbackNotifySet(SleepCallback_t func)
//{
//    mlsAppSleepWakeupCallback = func;
//}


/*******************************************************************************
 End of File
 */
