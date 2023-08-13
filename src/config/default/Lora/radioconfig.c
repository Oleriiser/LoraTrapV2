/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include "../definitions.h"
#include "radioconfig.h"
/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */
/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */
int global_data;


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */

void ConfigureRadioTx(void)
{
    uint8_t crcEnabled,iqInverted;
    crcEnabled = 1;//ENABLED;
    iqInverted = 0;//DISABLED;

    ConfigureRadio();

//    if(radioConfig.ecrConfig.override == true)
//    {
//        loRa.ecrConfig.override = true;
//        RADIO_GetAttr(ERROR_CODING_RATE,&(loRa.ecrConfig.ecr));
//        RADIO_SetAttr(ERROR_CODING_RATE,(void *)&(radioConfig.ecrConfig.ecr));
//    }

    //RADIO_SetAttr(OUTPUT_POWER,(void *)&radioConfiguration.txPower);
    RADIO_SetAttr(CRC_ON,(void *)&crcEnabled);
    RADIO_SetAttr(IQINVERTED,(void *)&iqInverted);
}

void ConfigureRadio(void)
{
    uint8_t loraModulation =1;
    RADIO_SetAttr(MODULATION,(void *)&(loraModulation));
    RADIO_SetAttr(CHANNEL_FREQUENCY,(void *)&(radioConfiguration.frequency));
    RADIO_SetAttr(FREQUENCY_HOP_PERIOD,(void *)&(radioConfiguration.frequencyHopPeriod));

    //LoRa modulation
    RADIO_SetAttr(SPREADING_FACTOR,(void *)&(radioConfiguration.dataRate));
    RADIO_SetAttr(BANDWIDTH,(void *)(&(radioConfiguration.bandWidth)));
    RADIO_SetAttr(LORA_SYNC_WORD,(void *)&(radioConfiguration.syncWordLoRa));
   

}


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
int ExampleInterfaceFunction(int param1, int param2) {
    return 0;
}


/* *****************************************************************************
 End of File
 */
