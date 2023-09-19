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
    defaultSleep.sleepTimeMs =10000;
    defaultSleep.sleep_mode = SLEEP_MODE_STANDBY;
    defaultSleep.pmmWakeupCallback = MlsAppSleepCallback;
    appData.trappedMice=0;
    appData.batteryVoltage=0;
    appData.lastBatteryVoltageRead=0;
    appData.unitId=UNIT_ID;
    appData.radio_power=LORA_POWER;
    RADIO_SetAttr(OUTPUT_POWER,(void*)&appData.radio_power);
    EIC_CallbackRegister(EIC_PIN_2, (EIC_CALLBACK)trapTriggerCallback, (uintptr_t)NULL);
    EIC_InterruptDisable(EIC_PIN_2);
    
    EIC_CallbackRegister(EIC_PIN_3, (EIC_CALLBACK)trapTriggerCallback, (uintptr_t)NULL);
    EIC_InterruptDisable(EIC_PIN_3);
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
            EIC_InterruptDisable(EIC_PIN_2);
            EIC_InterruptDisable(EIC_PIN_3);
            PORT_PinWrite(PORT_PIN_PB23, true);
            if(appData.lastBatteryVoltageRead==0)
            {
                
                ADC_Initialize();
                ADC_Enable();
                ADC_ConversionStart();
                appData.state =APP_STATE_READ_BATTERY_VOLTAGE;
                appData.lastBatteryVoltageRead++;
                break;
            }
            else
            {
                appData.lastBatteryVoltageRead++;
                if(appData.lastBatteryVoltageRead>=WAKEUPS_BETWEEN_BATTERYVOLTAGE_READ)
                {
                    appData.lastBatteryVoltageRead=0;
                }
                appData.state = APP_STATE_GET_MICE_COUNT;
                break;
            }
            
        }
        case APP_STATE_READ_BATTERY_VOLTAGE:
        {
           
            if(ADC_ConversionSequenceIsFinished())
            {
                
                //delay_ms(1);
                if(ADC_ConversionResultGet()>0)
                {
                    /*the voltage can be calculated by dividing the ADC resolution(vbat reference to the internal 1V ref
                     the interesting range is from 3.3V to 1.8V. to save transmission time the voltage result will be formatted in this range
                     * (3.3V*100)-100=230
                     * (1.8V*100)-100=80
                     * 
                     * This has to be accounted for on the recieving end aswell.
                        */
                    
                    
                    
                uint16_t tempVoltage=(4095*100)/ADC_ConversionResultGet();
                appData.batteryVoltage=tempVoltage-100;
                appData.state = APP_STATE_GET_MICE_COUNT;
                ADC_Disable();
                ADC_InterruptsClear(ADC_INTFLAG_Msk);
                }
            }
            break;
        }
        case APP_STATE_GET_MICE_COUNT:
        {             
            appData.trappedMice=readMouseTraps();    
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
            if (RADIO_Transmit(&RadioTransmitParam) == ERR_NONE)
            {
                appData.state = APP_STATE_ENTER_SLEEP;
            }
            else
            {
               appData.state=APP_STATE_INIT;     
            }
            break;
            }
        case APP_STATE_ENTER_SLEEP:
        {   
            EIC_InterruptEnable(EIC_PIN_2);
            EIC_InterruptEnable(EIC_PIN_3);
            
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

//    SW1_COM_OutputEnable();
//    SW1_COM_Set();
//    SW2_COM_OutputEnable();
//    SW2_COM_Set();
    if(SW1_COM_Get())
    {   
        if(NC_SW1_Get())
        {
            sw1=1;
        }
        NC_SW1_Toggle();
        SW3_COM_Toggle();//This should be NO_SW1_Toggle
    }
    if(SW2_COM_Get())
    {
        if(NC_SW2_Get())
        {
        sw2=1;
        }
        NC_SW2_Toggle();
        SW4_COM_Toggle();//This should be NO_SW2_Toggle
    }
//    SW1_COM_Clear();
//    SW2_COM_Clear();
    trapped = sw1+sw2;
    return trapped;
}

void buildLoraMessage(uint8_t *message)
{
    message[0]=appData.unitId;
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
    SUPC_DisableBod();
    ADC_Disable();
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
