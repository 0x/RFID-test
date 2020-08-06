
/*
 *****************************************************************************
 *                                                                           *
 *                 IMPINJ CONFIDENTIAL AND PROPRIETARY                       *
 *                                                                           *
 * This source code is the sole property of Impinj, Inc.  Reproduction or    *
 * utilization of this source code in whole or in part is forbidden without  *
 * the prior written consent of Impinj, Inc.                                 *
 *                                                                           *
 * (c) Copyright Impinj, Inc. 2007,2009. All rights reserved.                *
 *                                                                           *
 *****************************************************************************/

/**
 *****************************************************************************
 **
 ** @file  example1.cpp
 **
 ** @brief Simple example of LTKCPP that talks to an actual reader
 **
 ** This is sometimes called the "LLRP Hello World" program
 **
 ** The steps:
 ** 1.  Initialize Library
 ** 2.  Connect to Reader
 ** 3.  Enable Impinj Extensions
 ** 4.  Factory Default LLRP configuration to ensure that the reader is in 
        a known state (since we are relying on the default reader 
        configuration for this simple example)
 ** 5.  ADD_ROSPEC to tell the reader to perform an inventory 
 ** 6.  ENABLE_ROSPEC
 ** 7.  START_ROSPEC start the inventory operation
 ** 8.  Process RFID Data (EPC, RSSI, Timestamp)
 **
 ** This program can be run with zero, one, or two verbose options (-v).
 **     no -v -- Only prints the tag report and errors
 **     -v    -- Also prints one line progress messages
 **     -vv   -- Also prints all LLRP messages as XML text
 **
 *****************************************************************************/


#include <stdio.h>
#include "ltkcpp.h"
#include "impinj_ltkcpp.h"
#include "time.h"

using namespace LLRP;

/*
** Sorry, we use this linux safe method
** to print buffers.  WIndows has the same
** method, but by a different name
*/
#if (WIN32)
#define snprintf _snprintf
#endif

class CMyApplication
{
    unsigned int m_messageID;

  public:
    /** Verbose level, incremented by each -v on command line */
    int                         m_Verbose;

    /** Connection to the LLRP reader */
    CConnection *               m_pConnectionToReader;

    inline
    CMyApplication (void)
     : m_Verbose(0), m_pConnectionToReader(NULL)
    {
        m_messageID = 0;
    }

    int
    run (
      char *                    pReaderHostName);

    int
    checkConnectionStatus (void);

    int
    enableImpinjExtensions (void);

    int
    resetConfigurationToFactoryDefaults (void);

    int
    addROSpec (void);

    int
    enableROSpec (void);

    int
    startROSpec (void);

    int
    stopROSpec (void);

    int
    awaitAndPrintReport (int timeoutSec);

    void
    printTagReportData (
      CRO_ACCESS_REPORT *       pRO_ACCESS_REPORT);

    void
    printOneTagReportData (
      CTagReportData *          pTagReportData);

    void
    formatOneEPC (
      CParameter *          pEpcParameter,
      char *                buf,
      int                   buflen);
      
    void
    handleReaderEventNotification (
      CReaderEventNotificationData *pNtfData);

    void
    handleAntennaEvent (
      CAntennaEvent *           pAntennaEvent);

    void
    handleReaderExceptionEvent (
      CReaderExceptionEvent *   pReaderExceptionEvent);

    int
    checkLLRPStatus (
      CLLRPStatus *             pLLRPStatus,
      char *                    pWhatStr);

    CMessage *
    transact (
      CMessage *                pSendMsg);

    CMessage *
    recvMessage (
      int                       nMaxMS);

    int
    sendMessage (
      CMessage *                pSendMsg);

    void
    printXMLMessage (
      CMessage *                pMessage);
};


/* BEGIN forward declarations */
int
main (
  int                           ac,
  char *                        av[]);

void
usage (
  char *                        pProgName);
/* END forward declarations */


/**
 *****************************************************************************
 **
 ** @brief  Command main routine
 **
 ** Command synopsis:
 **
 **     example1 [-v[v]] READERHOSTNAME
 **
 ** @exitcode   0               Everything *seemed* to work.
 **             1               Bad usage
 **             2               Run failed
 **
 *****************************************************************************/

int
main (
  int                           ac,
  char *                        av[])
{
    CMyApplication              myApp;
    char *                      pReaderHostName;
    int                         rc;

    /*
     * Process comand arguments, determine reader name
     * and verbosity level.
     */
    if(ac == 2)
    {
        pReaderHostName = av[1];
    }
    else if(ac == 3)
    {
        char *                  p = av[1];

        while(*p)
        {
            switch(*p++)
            {
            case '-':   /* linux conventional option warn char */
            case '/':   /* Windows/DOS conventional option warn char */
                break;

            case 'v':
            case 'V':
                myApp.m_Verbose++;
                break;

            default:
                usage(av[0]);
                /* no return */
                break;
            }
        }

        pReaderHostName = av[2];
    }
    else
    {
        usage(av[0]);
        /* no return */
    }

    /*
     * Run application, capture return value for exit status
     */
    rc = myApp.run(pReaderHostName);

    printf("INFO: Done\n");

    /*
     * Exit with the right status.
     */
    if(0 == rc)
    {
        exit(0);
    }
    else
    {
        exit(2);
    }
    /*NOTREACHED*/
}


/**
 *****************************************************************************
 **
 ** @brief  Print usage message and exit
 **
 ** @param[in]  nProgName       Program name string
 **
 ** @return     none, exits
 **
 *****************************************************************************/

void
usage (
  char *                        pProgName)
{
#ifdef linux
    printf("Usage: %s [-v[v]] READERHOSTNAME\n", pProgName);
    printf("\n");
    printf("Each -v increases verbosity level\n");
#endif /* linux */
#ifdef WIN32
    printf("Usage: %s [/v[v]] READERHOSTNAME\n", pProgName);
    printf("\n");
    printf("Each /v increases verbosity level\n");
#endif /* WIN32 */
    exit(1);
}


/**
 *****************************************************************************
 **
 ** @brief  Run the application
 **
 ** The steps:
 **     - Instantiate connection
 **     - Connect to LLRP reader (TCP)
 **     - Make sure the connection status is good
 **     - Clear (scrub) the reader configuration
 **     - Configure for what we want to do
 **     - Run inventory operation 5 times
 **     - Again, clear (scrub) the reader configuration
 **     - Disconnect from reader
 **     - Destruct connection
 **
 ** @param[in]  pReaderHostName String with reader name
 **
 ** @return      0              Everything worked.
 **             -1              Failed allocation of type registry
 **             -2              Failed construction of connection
 **             -3              Could not connect to reader
 **              1              Reader connection status bad
 **              2              Impinj Extension enable failed
 **              3              Cleaning reader config failed
 **              4              Adding ROSpec failed
 **              5              Enabling ROSpec failed
 **              6              Start ROSpec failed
 **              7              Something went wrong running the ROSpec
 **              8              Stopping ROSpec failed
 **
 *****************************************************************************/

int
CMyApplication::run (
  char *                        pReaderHostName)
{
    CTypeRegistry *             pTypeRegistry;
    CConnection *               pConn;
    int                         rc;

    /*
     * Allocate the type registry. This is needed
     * by the connection to decode.
     */
    pTypeRegistry = getTheTypeRegistry();
    if(NULL == pTypeRegistry)
    {
        printf("ERROR: getTheTypeRegistry failed\n");
        return -1;
    }

    /*
     * Enroll impinj extension types into the 
     * type registry, in preparation for using 
     * Impinj extension params.
     */
    LLRP::enrollImpinjTypesIntoRegistry(pTypeRegistry);

    /*
     * Construct a connection (LLRP::CConnection).
     * Using a 32kb max frame size for send/recv.
     * The connection object is ready for business
     * but not actually connected to the reader yet.
     */
    pConn = new CConnection(pTypeRegistry, 32u*1024u);
    if(NULL == pConn)
    {
        printf("ERROR: new CConnection failed\n");
        return -2;
    }

    /*
     * Open the connection to the reader
     */
    if(m_Verbose)
    {
        printf("INFO: Connecting to %s....\n", pReaderHostName);
    }

    rc = pConn->openConnectionToReader(pReaderHostName);
    if(0 != rc)
    {
        printf("ERROR: connect: %s (%d)\n", pConn->getConnectError(), rc);
        delete pConn;
        return -3;
    }

    /*
     * Record the pointer to the connection object so other
     * routines can use it.
     */
    m_pConnectionToReader = pConn;

    if(m_Verbose)
    {
        printf("INFO: Connected, checking status....\n");
    }

    /*
     * Commence the sequence and check for errors as we go.
     * See comments for each routine for details.
     * Each routine prints messages.
     */
    rc = 1;
    if(0 == checkConnectionStatus())
    {
        rc = 2;
        if(0 == enableImpinjExtensions())
        {
            rc = 3;
            if(0 == resetConfigurationToFactoryDefaults())
            {
                rc = 4;
                if(0 == addROSpec())
                {
                    rc = 5;
                    if(0 == enableROSpec())
                    {
                        //rc = 6;
                        //if(0 == startROSpec())
                        //{
                            rc = 7;
                            if(0 == awaitAndPrintReport(10))
                            {
                                rc = 8;
                                if(0 == stopROSpec())
                                {
                                    rc = 0;
                                }
                            }
                        //}
                    }
                }
            }

            /*
             * After we're done, try to leave the reader
             * in a clean state for next use. This is best
             * effort and no checking of the result is done.
             */
            if(m_Verbose)
            {
                printf("INFO: Clean up reader configuration...\n");
            }
            resetConfigurationToFactoryDefaults();
        }
    }

    if(m_Verbose)
    {
        printf("INFO: Finished\n");
    }

    /*
     * Close the connection and release its resources
     */
    pConn->closeConnectionToReader();
    delete pConn;

    /*
     * Done with the registry.
     */
    delete pTypeRegistry;

    /*
     * When we get here all allocated memory should have been deallocated.
     */

    return rc;
}


/**
 *****************************************************************************
 **
 ** @brief  Await and check the connection status message from the reader
 **
 ** We are expecting a READER_EVENT_NOTIFICATION message that
 ** tells us the connection is OK. The reader is suppose to
 ** send the message promptly upon connection.
 **
 ** If there is already another LLRP connection to the
 ** reader we'll get a bad Status.
 **
 ** The message should be something like:
 **
 **     <READER_EVENT_NOTIFICATION MessageID='0'>
 **       <ReaderEventNotificationData>
 **         <UTCTimestamp>
 **           <Microseconds>1184491439614224</Microseconds>
 **         </UTCTimestamp>
 **         <ConnectionAttemptEvent>
 **           <Status>Success</Status>
 **         </ConnectionAttemptEvent>
 **       </ReaderEventNotificationData>
 **     </READER_EVENT_NOTIFICATION>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::checkConnectionStatus (void)
{
    CMessage *                  pMessage;
    CREADER_EVENT_NOTIFICATION *pNtf;
    CReaderEventNotificationData *pNtfData;
    CConnectionAttemptEvent *   pEvent;

    /*
     * Expect the notification within 10 seconds.
     * It is suppose to be the very first message sent.
     */
    pMessage = recvMessage(10000);

    /*
     * recvMessage() returns NULL if something went wrong.
     */
    if(NULL == pMessage)
    {
        /* recvMessage already tattled */
        goto fail;
    }

    /*
     * Check to make sure the message is of the right type.
     * The type label (pointer) in the message should be
     * the type descriptor for READER_EVENT_NOTIFICATION.
     */
    if(&CREADER_EVENT_NOTIFICATION::s_typeDescriptor != pMessage->m_pType)
    {
        goto fail;
    }

    /*
     * Now that we are sure it is a READER_EVENT_NOTIFICATION,
     * traverse to the ReaderEventNotificationData parameter.
     */
    pNtf = (CREADER_EVENT_NOTIFICATION *) pMessage;
    pNtfData = pNtf->getReaderEventNotificationData();
    if(NULL == pNtfData)
    {
        goto fail;
    }

    /*
     * The ConnectionAttemptEvent parameter must be present.
     */
    pEvent = pNtfData->getConnectionAttemptEvent();
    if(NULL == pEvent)
    {
        goto fail;
    }

    /*
     * The status in the ConnectionAttemptEvent parameter
     * must indicate connection success.
     */
    if(ConnectionAttemptStatusType_Success != pEvent->getStatus())
    {
        goto fail;
    }

    /*
     * Done with the message
     */
    delete pMessage;

    if(m_Verbose)
    {
        printf("INFO: Connection status OK\n");
    }

    /*
     * Victory.
     */
    return 0;

  fail:
    /*
     * Something went wrong. Tattle. Clean up. Return error.
     */
    printf("ERROR: checkConnectionStatus failed\n");
    delete pMessage;
    return -1;
}

/**
 *****************************************************************************
 **
 ** @brief  Send an IMPINJ_ENABLE_EXTENSION_MESSAGE
 **
 ** NB: Send the message to enable the impinj extension.  This must
 ** be done every time we connect to the reader.
 **
 ** The message is:
 ** <Impinj:IMPINJ_ENABLE_EXTENSIONS MessageID="X">
 ** </Impinj:IMPINJ_ENABLE_EXTENSIONS >
 **
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/
int
CMyApplication::enableImpinjExtensions (void)
{
    CIMPINJ_ENABLE_EXTENSIONS *        pCmd;
    CMessage *                         pRspMsg;
    CIMPINJ_ENABLE_EXTENSIONS_RESPONSE *pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CIMPINJ_ENABLE_EXTENSIONS();
    pCmd->setMessageID(m_messageID++);
    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a CIMPINJ_ENABLE_EXTENSIONS_RESPONSE message.
     */
    pRsp = (CIMPINJ_ENABLE_EXTENSIONS_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(),
                        "enableImpinjExtensions"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress, maybe
     */
    if(m_Verbose)
    {
        printf("INFO: Impinj Extensions are enabled\n");
    }

    /*
     * Victory.
     */
    return 0;
}

/**
 *****************************************************************************
 **
 ** @brief  Send a SET_READER_CONFIG message that resets the
 **         reader to factory defaults.
 **
 ** NB: The ResetToFactoryDefault semantics vary between readers.
 **     It might have no effect because it is optional.
 **
 ** The message is:
 **
 **     <SET_READER_CONFIG MessageID='X'>
 **       <ResetToFactoryDefault>1</ResetToFactoryDefault>
 **     </SET_READER_CONFIG>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::resetConfigurationToFactoryDefaults (void)
{
    CSET_READER_CONFIG *        pCmd;
    CMessage *                  pRspMsg;
    CSET_READER_CONFIG_RESPONSE *pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CSET_READER_CONFIG();
    pCmd->setMessageID(m_messageID++);
    pCmd->setResetToFactoryDefault(1);

    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a SET_READER_CONFIG_RESPONSE message.
     */
    pRsp = (CSET_READER_CONFIG_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(),
                        "resetConfigurationToFactoryDefaults"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress, maybe
     */
    if(m_Verbose)
    {
        printf("INFO: Configuration reset to factory defaults\n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Add our ROSpec using ADD_ROSPEC message
 **
 ** This ROSPec is the simples ROSpec possible. It
 ** starts and stops based on application command,
 ** enables all antennas, and uses all the reader
 ** settings from the SET_READER_CONFIG message
 ** (or default values if SET_READER_CONFIG has not
 ** been called on the reader 
 **
 ** The message is
 **
 ** <ADD_ROSPEC MessageID="X">
 **  <ROSpec>
 **      <ROSpecID>1111</ROSpecID>
 **      <Priority>0</Priority>
 **      <CurrentState>Disabled</CurrentState>
 **      <ROBoundarySpec>
 **          <ROSpecStartTrigger>
 **              <ROSpecStartTriggerType>Null</ROSpecStartTriggerType>
 **          </ROSpecStartTrigger>
 **          <ROSpecStopTrigger>
 **              <ROSpecStopTriggerType>Null</ROSpecStopTriggerType>
 **              <DurationTriggerValue>0</DurationTriggerValue>
 **          </ROSpecStopTrigger>
 **      </ROBoundarySpec>
 **      <AISpec>
 **          <AntennaIDs>0</AntennaIDs>
 **          <AISpecStopTrigger>
 **              <AISpecStopTriggerType>Null</AISpecStopTriggerType>
 **              <DurationTrigger>0</DurationTrigger>
 **          </AISpecStopTrigger>
 **          <InventoryParameterSpec>
 **              <InventoryParameterSpecID>1234</InventoryParameterSpecID>
 **              <ProtocolID>EPCGlobalClass1Gen2</ProtocolID>
 **          </InventoryParameterSpec>
 **      </AISpec>
 **  </ROSpec>
 ** </ADD_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::addROSpec (void)
{
    CROSpecStartTrigger *       pROSpecStartTrigger =
                                    new CROSpecStartTrigger();
    pROSpecStartTrigger->setROSpecStartTriggerType(
                                ROSpecStartTriggerType_Immediate);

    CROSpecStopTrigger *        pROSpecStopTrigger = new CROSpecStopTrigger();
    pROSpecStopTrigger->setROSpecStopTriggerType(ROSpecStopTriggerType_Null);
    pROSpecStopTrigger->setDurationTriggerValue(0);     /* n/a */

    CROBoundarySpec *           pROBoundarySpec = new CROBoundarySpec();
    pROBoundarySpec->setROSpecStartTrigger(pROSpecStartTrigger);
    pROBoundarySpec->setROSpecStopTrigger(pROSpecStopTrigger);

    CAISpecStopTrigger *        pAISpecStopTrigger = new CAISpecStopTrigger();
    pAISpecStopTrigger->setAISpecStopTriggerType(
            AISpecStopTriggerType_Tag_Observation);
    pAISpecStopTrigger->setDurationTrigger(0);

    CTagObservationTrigger *    pAISpecTagObsvTrigger = new CTagObservationTrigger();
    pAISpecTagObsvTrigger->setTriggerType(TagObservationTriggerType_Upon_Seeing_N_Tags_Or_Timeout);
    pAISpecTagObsvTrigger->setNumberOfTags(100);
    pAISpecTagObsvTrigger->setNumberOfAttempts(0);
    pAISpecTagObsvTrigger->setT(0);
    pAISpecTagObsvTrigger->setTimeout(500);

    pAISpecStopTrigger->setTagObservationTrigger(pAISpecTagObsvTrigger);


/////


    CInventoryParameterSpec *   pInventoryParameterSpec =
                                    new CInventoryParameterSpec();
    pInventoryParameterSpec->setInventoryParameterSpecID(1234);
    pInventoryParameterSpec->setProtocolID(AirProtocols_EPCGlobalClass1Gen2);

    CAntennaConfiguration *     pAntennaConfig = new CAntennaConfiguration();
    pAntennaConfig->setAntennaID(1);

    CRFTransmitter *               pRFTransmitter = new CRFTransmitter();
    pRFTransmitter->setHopTableID(1);
    pRFTransmitter->setChannelIndex(0);
    pRFTransmitter->setTransmitPower(81);
    pAntennaConfig->setRFTransmitter(pRFTransmitter);

    CC1G2InventoryCommand *        pC1G2InventoryCmd = new CC1G2InventoryCommand();
    pC1G2InventoryCmd->setTagInventoryStateAware(1);

// Filter 1    
    CC1G2Filter *                  pC1G2Filter1 = new CC1G2Filter();
    pC1G2Filter1->setT(C1G2TruncateAction_Do_Not_Truncate);

    CC1G2TagInventoryMask *        pC1G2TagInvMask1 = new CC1G2TagInventoryMask();
    pC1G2TagInvMask1->setMB(2);
    pC1G2TagInvMask1->setPointer(0);
    
    llrp_u1v_t tagMask1 = llrp_u1v_t(15);
    tagMask1.m_nBit = 15;
    tagMask1.m_pValue[0] = 0xE2;
    tagMask1.m_pValue[1] = 0x00;
    pC1G2TagInvMask1->setTagMask(tagMask1);

    pC1G2Filter1->setC1G2TagInventoryMask(pC1G2TagInvMask1);
    
    CC1G2TagInventoryStateAwareFilterAction * pC1G2TagInvStateAwareFilterAction1 = new CC1G2TagInventoryStateAwareFilterAction();
    pC1G2TagInvStateAwareFilterAction1->setTarget(C1G2StateAwareTarget_Inventoried_State_For_Session_S3);
    pC1G2TagInvStateAwareFilterAction1->setAction(C1G2StateAwareAction_AssertSLOrA_DeassertSLOrB);

    pC1G2Filter1->setC1G2TagInventoryStateAwareFilterAction(pC1G2TagInvStateAwareFilterAction1);

    pC1G2InventoryCmd->addC1G2Filter(pC1G2Filter1);

 // Filter 2
    CC1G2Filter *                  pC1G2Filter2 = new CC1G2Filter();
    pC1G2Filter2->setT(C1G2TruncateAction_Do_Not_Truncate);

    CC1G2TagInventoryMask *        pC1G2TagInvMask2 = new CC1G2TagInventoryMask();
    pC1G2TagInvMask2->setMB(1);
    pC1G2TagInvMask2->setPointer(0);

    llrp_u1v_t tagMask2 = llrp_u1v_t(16);
    tagMask2.m_nBit = 16;
    tagMask2.m_pValue[0] = 0xE2;
    tagMask2.m_pValue[1] = 0x00;
    pC1G2TagInvMask2->setTagMask(tagMask2);

    pC1G2Filter2->setC1G2TagInventoryMask(pC1G2TagInvMask2);
    
    CC1G2TagInventoryStateAwareFilterAction * pC1G2TagInvStateAwareFilterAction2 = new CC1G2TagInventoryStateAwareFilterAction();
    pC1G2TagInvStateAwareFilterAction2->setTarget(C1G2StateAwareTarget_Inventoried_State_For_Session_S2);
    pC1G2TagInvStateAwareFilterAction2->setAction(C1G2StateAwareAction_AssertSLOrA_DeassertSLOrB);

    pC1G2Filter2->setC1G2TagInventoryStateAwareFilterAction(pC1G2TagInvStateAwareFilterAction2);

    pC1G2InventoryCmd->addC1G2Filter(pC1G2Filter2); 

// Filter 3
    CC1G2Filter *                  pC1G2Filter3 = new CC1G2Filter();
    pC1G2Filter3->setT(C1G2TruncateAction_Do_Not_Truncate);

    CC1G2TagInventoryMask *        pC1G2TagInvMask3 = new CC1G2TagInventoryMask();
    pC1G2TagInvMask3->setMB(3);
    pC1G2TagInvMask3->setPointer(0);

    llrp_u1v_t tagMask3 = llrp_u1v_t(32);
    tagMask3.m_nBit = 32;
    tagMask3.m_pValue[0] = 0xFF;
    tagMask3.m_pValue[1] = 0xFF;
    tagMask3.m_pValue[2] = 0xE2;
    tagMask3.m_pValue[3] = 0x00;
    pC1G2TagInvMask3->setTagMask(tagMask3);

    pC1G2Filter3->setC1G2TagInventoryMask(pC1G2TagInvMask3);
    
    CC1G2TagInventoryStateAwareFilterAction * pC1G2TagInvStateAwareFilterAction3 = new CC1G2TagInventoryStateAwareFilterAction();
    pC1G2TagInvStateAwareFilterAction3->setTarget(C1G2StateAwareTarget_Inventoried_State_For_Session_S1);
    pC1G2TagInvStateAwareFilterAction3->setAction(C1G2StateAwareAction_AssertSLOrA_DeassertSLOrB);

    pC1G2Filter3->setC1G2TagInventoryStateAwareFilterAction(pC1G2TagInvStateAwareFilterAction3);

    pC1G2InventoryCmd->addC1G2Filter(pC1G2Filter3); 

// Filter 4
    CC1G2Filter *                  pC1G2Filter4 = new CC1G2Filter();
    pC1G2Filter4->setT(C1G2TruncateAction_Do_Not_Truncate);

    CC1G2TagInventoryMask *        pC1G2TagInvMask4 = new CC1G2TagInventoryMask();
    pC1G2TagInvMask4->setMB(2);
    pC1G2TagInvMask4->setPointer(0);

    llrp_u1v_t tagMask4 = llrp_u1v_t(8);
    tagMask4.m_nBit = 8;
    tagMask4.m_pValue[0] = 0xAA;
    pC1G2TagInvMask4->setTagMask(tagMask4);

    pC1G2Filter4->setC1G2TagInventoryMask(pC1G2TagInvMask4);
    
    CC1G2TagInventoryStateAwareFilterAction * pC1G2TagInvStateAwareFilterAction4 = new CC1G2TagInventoryStateAwareFilterAction();
    pC1G2TagInvStateAwareFilterAction4->setTarget(C1G2StateAwareTarget_Inventoried_State_For_Session_S0);
    pC1G2TagInvStateAwareFilterAction4->setAction(C1G2StateAwareAction_AssertSLOrA_DeassertSLOrB);

    pC1G2Filter4->setC1G2TagInventoryStateAwareFilterAction(pC1G2TagInvStateAwareFilterAction4);

    pC1G2InventoryCmd->addC1G2Filter(pC1G2Filter4); 

// Filter 5
    CC1G2Filter *                  pC1G2Filter5 = new CC1G2Filter();
    pC1G2Filter5->setT(C1G2TruncateAction_Do_Not_Truncate);

    CC1G2TagInventoryMask *        pC1G2TagInvMask5 = new CC1G2TagInventoryMask();
    pC1G2TagInvMask5->setMB(1);
    pC1G2TagInvMask5->setPointer(0);

    llrp_u1v_t tagMask5 = llrp_u1v_t(16);
    tagMask5.m_nBit = 16;
    tagMask5.m_pValue[0] = 0xFF;
    tagMask5.m_pValue[1] = 0xFF;
    pC1G2TagInvMask5->setTagMask(tagMask5);

    pC1G2Filter5->setC1G2TagInventoryMask(pC1G2TagInvMask5);
    
    CC1G2TagInventoryStateAwareFilterAction * pC1G2TagInvStateAwareFilterAction5 = new CC1G2TagInventoryStateAwareFilterAction();
    pC1G2TagInvStateAwareFilterAction5->setTarget(C1G2StateAwareTarget_SL);
    pC1G2TagInvStateAwareFilterAction5->setAction(C1G2StateAwareAction_AssertSLOrA_DeassertSLOrB);

    pC1G2Filter5->setC1G2TagInventoryStateAwareFilterAction(pC1G2TagInvStateAwareFilterAction5);

    pC1G2InventoryCmd->addC1G2Filter(pC1G2Filter5); 

//


    CC1G2SingulationControl *               pC1G2SingulationControl = new CC1G2SingulationControl();
    pC1G2SingulationControl->setSession(1);
    pC1G2SingulationControl->setTagPopulation(32);
    pC1G2SingulationControl->setTagTransitTime(0);

    CC1G2TagInventoryStateAwareSingulationAction * pC1G2TagInvStateAwareSingAction = new CC1G2TagInventoryStateAwareSingulationAction();
    pC1G2TagInvStateAwareSingAction->setI(C1G2TagInventoryStateAwareI_State_B);
    pC1G2TagInvStateAwareSingAction->setS(C1G2TagInventoryStateAwareS_SL);

    pC1G2SingulationControl->setC1G2TagInventoryStateAwareSingulationAction(pC1G2TagInvStateAwareSingAction);

    pC1G2InventoryCmd->setC1G2SingulationControl(pC1G2SingulationControl);

    pAntennaConfig->addAirProtocolInventoryCommandSettings(pC1G2InventoryCmd);

    pInventoryParameterSpec->addAntennaConfiguration(pAntennaConfig);

/////
    llrp_u16v_t AntennaIDs = llrp_u16v_t(1);
    AntennaIDs.m_pValue[0] = 1;

    CAISpec *                   pAISpec = new CAISpec();
    pAISpec->setAntennaIDs(AntennaIDs);
    pAISpec->setAISpecStopTrigger(pAISpecStopTrigger);
    pAISpec->addInventoryParameterSpec(pInventoryParameterSpec);


////
    CROReportSpec *             pROReportSpec = new CROReportSpec();
    pROReportSpec->setROReportTrigger(ROReportTriggerType_Upon_N_Tags_Or_End_Of_AISpec);
    pROReportSpec->setN(1000);

    CTagReportContentSelector * pTagRptContentSelector = new CTagReportContentSelector();
    pTagRptContentSelector->setEnableROSpecID(1);
    pTagRptContentSelector->setEnableSpecIndex(1);
    pTagRptContentSelector->setEnableInventoryParameterSpecID(1);
    pTagRptContentSelector->setEnableAntennaID(1);
    pTagRptContentSelector->setEnableChannelIndex(1);
    pTagRptContentSelector->setEnablePeakRSSI(1);
    pTagRptContentSelector->setEnableFirstSeenTimestamp(1);
    pTagRptContentSelector->setEnableLastSeenTimestamp(1);
    pTagRptContentSelector->setEnableTagSeenCount(1);
    pTagRptContentSelector->setEnableAccessSpecID(1);

    pROReportSpec->setTagReportContentSelector(pTagRptContentSelector);    
////


    CROSpec *                   pROSpec = new CROSpec();
    pROSpec->setROSpecID(1);
    pROSpec->setPriority(0);
    pROSpec->setCurrentState(ROSpecState_Disabled);
    pROSpec->setROBoundarySpec(pROBoundarySpec);
    pROSpec->addSpecParameter(pAISpec);
    pROSpec->setROReportSpec(pROReportSpec);

    CADD_ROSPEC *               pCmd;
    CMessage *                  pRspMsg;
    CADD_ROSPEC_RESPONSE *      pRsp;

    /*
     * Compose the command message.
     * N.B.: After the message is composed, all the parameters
     *       constructed, immediately above, are considered "owned"
     *       by the command message. When it is destructed so
     *       too will the parameters be.
     */
    pCmd = new CADD_ROSPEC();
    pCmd->setMessageID(m_messageID++);
    pCmd->setROSpec(pROSpec);

    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message.
     * N.B.: And the parameters
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a ADD_ROSPEC_RESPONSE message.
     */
    pRsp = (CADD_ROSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "addROSpec"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress, maybe
     */
    if(m_Verbose)
    {
        printf("INFO: ROSpec added\n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Enable our ROSpec using ENABLE_ROSPEC message
 **
 ** Enable the ROSpec that was added above.
 **
 ** The message we send is:
 **     <ENABLE_ROSPEC MessageID='X'>
 **       <ROSpecID>123</ROSpecID>
 **     </ENABLE_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::enableROSpec (void)
{
    CENABLE_ROSPEC *            pCmd;
    CMessage *                  pRspMsg;
    CENABLE_ROSPEC_RESPONSE *   pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CENABLE_ROSPEC();
    pCmd->setMessageID(m_messageID++);
    pCmd->setROSpecID(1);

    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a ENABLE_ROSPEC_RESPONSE message.
     */
    pRsp = (CENABLE_ROSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "enableROSpec"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress, maybe
     */
    if(m_Verbose)
    {
        printf("INFO: ROSpec enabled\n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Start our ROSpec using START_ROSPEC message
 **
 ** Start the ROSpec that was added above.
 **
 ** The message we send is:
 **     <START_ROSPEC MessageID='X'>
 **       <ROSpecID>123</ROSpecID>
 **     </START_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::startROSpec (void)
{
    CSTART_ROSPEC *             pCmd;
    CMessage *                  pRspMsg;
    CSTART_ROSPEC_RESPONSE *    pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CSTART_ROSPEC();
    pCmd->setMessageID(m_messageID++);
    pCmd->setROSpecID(1);

    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a START_ROSPEC_RESPONSE message.
     */
    pRsp = (CSTART_ROSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "startROSpec"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress
     */
    if(m_Verbose)
    {
        printf("INFO: ROSpec started\n");
    }

    /*
     * Victory.
     */
    return 0;
}

/**
 *****************************************************************************
 **
 ** @brief  Stop our ROSpec using STOP_ROSPEC message
 **
 ** Stop the ROSpec that was added above.
 **
 ** The message we send is:
 **     <STOP_ROSPEC MessageID='203'>
 **       <ROSpecID>123</ROSpecID>
 **     </STOP_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::stopROSpec (void)
{
    CSTOP_ROSPEC *             pCmd;
    CMessage *                  pRspMsg;
    CSTOP_ROSPEC_RESPONSE *    pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CSTOP_ROSPEC();
    pCmd->setMessageID(m_messageID++);
    pCmd->setROSpecID(1);

    /*
     * Send the message, expect the response of certain type
     */
    pRspMsg = transact(pCmd);

    /*
     * Done with the command message
     */
    delete pCmd;

    /*
     * transact() returns NULL if something went wrong.
     */
    if(NULL == pRspMsg)
    {
        /* transact already tattled */
        return -1;
    }

    /*
     * Cast to a STOP_ROSPEC_RESPONSE message.
     */
    pRsp = (CSTOP_ROSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "stopROSpec"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
     * Done with the response message.
     */
    delete pRspMsg;

    /*
     * Tattle progress
     */
    if(m_Verbose)
    {
        printf("INFO: ROSpec stopped\n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Receive and print the RO_ACCESS_REPORT
 **
 ** Receive messages for timeout seconds and then stop. Typically
 ** for simple applications, this is sufficient.  For applications with
 ** asyncrhonous reporting or other asyncrhonous activity, it is recommended
 ** to create a thread to perform the report listening.
 **
 ** @param[in]                  timeout
 **
 ** This shows how to determine the type of a received message.
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::awaitAndPrintReport (int timeout)
{
    int                         bDone = 0;
    int                         retVal = 0;
    time_t                      startTime = time(NULL);
    time_t                      tempTime;
    /*
     * Keep receiving messages until done or until
     * something bad happens.
     */
    while(!bDone)
    {
        CMessage *              pMessage;
        const CTypeDescriptor * pType;

        /*
         * Wait up to 1 second for a report.  Check
         * That way, we can check the timestamp even if 
         * there are no reports coming in
         */
        pMessage = recvMessage(1000);

        /* validate the timestamp */
        tempTime = time(NULL);
        if(difftime(tempTime, startTime) > timeout)
        {
            bDone=1;
        }

        if(NULL == pMessage)
        {
            continue;
        }

        /*
         * What happens depends on what kind of message
         * received. Use the type label (m_pType) to
         * discriminate message types.
         */
        pType = pMessage->m_pType;

        /*
         * Is it a tag report? If so, print it out.
         */
        if(&CRO_ACCESS_REPORT::s_typeDescriptor == pType)
        {
            CRO_ACCESS_REPORT * pNtf;

            pNtf = (CRO_ACCESS_REPORT *) pMessage;

            printTagReportData(pNtf);
        }

        /*
         * Is it a reader event? This example only recognizes
         * AntennaEvents.
         */
        else if(&CREADER_EVENT_NOTIFICATION::s_typeDescriptor == pType)
        {
            CREADER_EVENT_NOTIFICATION *pNtf;
            CReaderEventNotificationData *pNtfData;

            pNtf = (CREADER_EVENT_NOTIFICATION *) pMessage;

            pNtfData = pNtf->getReaderEventNotificationData();
            if(NULL != pNtfData)
            {
                handleReaderEventNotification(pNtfData);
            }
            else
            {
                /*
                 * This should never happen. Using continue
                 * to keep indent depth down.
                 */
                printf("WARNING: READER_EVENT_NOTIFICATION without data\n");
            }
        }

        /*
         * Hmmm. Something unexpected. Just tattle and keep going.
         */
        else
        {
            printf("WARNING: Ignored unexpected message during monitor: %s\n",
                pType->m_pName);
        }

        /*
         * Done with the received message
         */
        delete pMessage;
    }

    return retVal;
}


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print a tag report
 **
 ** The report is printed in list order, which is arbitrary.
 **
 ** TODO: It would be cool to sort the list by EPC and antenna,
 **       then print it.
 **
 ** @return     void
 **
 *****************************************************************************/

void
CMyApplication::printTagReportData (
  CRO_ACCESS_REPORT *           pRO_ACCESS_REPORT)
{
    std::list<CTagReportData *>::iterator Cur;

    unsigned int                nEntry = 0;

    /*
     * Loop through and count the number of entries
     */
    for(
        Cur = pRO_ACCESS_REPORT->beginTagReportData();
        Cur != pRO_ACCESS_REPORT->endTagReportData();
        Cur++)
    {
        nEntry++;
    }

    if(m_Verbose)
    {
    printf("INFO: %u tag report entries\n", nEntry);
    }

    /*
     * Loop through again and print each entry.
     */
    for(
        Cur = pRO_ACCESS_REPORT->beginTagReportData();
        Cur != pRO_ACCESS_REPORT->endTagReportData();
        Cur++)
    {
        printOneTagReportData(*Cur);
    }
}


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one EPC data parameter
 **
 ** @return     void
 **
 *****************************************************************************/
void
CMyApplication::formatOneEPC (
  CParameter *pEPCParameter, 
  char *buf, 
  int buflen)
{
    char *              p = buf;
    int                 bufsize = buflen;
    int                 written = 0;

    if(NULL != pEPCParameter)
    {
        const CTypeDescriptor *     pType;
        llrp_u96_t          my_u96;
        llrp_u1v_t          my_u1v;
        llrp_u8_t *         pValue = NULL;
        unsigned int        n, i;

        pType = pEPCParameter->m_pType;
        if(&CEPC_96::s_typeDescriptor == pType)
        {
            CEPC_96             *pEPC_96;

            pEPC_96 = (CEPC_96 *) pEPCParameter;
            my_u96 = pEPC_96->getEPC();
            pValue = my_u96.m_aValue;
            n = 12u;
        }
        else if(&CEPCData::s_typeDescriptor == pType)
        {
            CEPCData *          pEPCData;

            pEPCData = (CEPCData *) pEPCParameter;
            my_u1v = pEPCData->getEPC();
            pValue = my_u1v.m_pValue;
            n = (my_u1v.m_nBit + 7u) / 8u;
        }

        if(NULL != pValue)
        {
            for(i = 0; i < n; i++)
            {
                if(0 < i && i%2 == 0 && 1 < bufsize)
                {
                    *p++ = '-';
                    bufsize--;
                }
                if(bufsize > 2)
                {
                    written = snprintf(p, bufsize, "%02X", pValue[i]);
                    bufsize -= written;
                    p+= written;
                }
            }
        }
        else
        {
            written = snprintf(p, bufsize, "%s", "---unknown-epc-data-type---");
            bufsize -= written;
            p += written;
        }
    }
    else
    {
        written = snprintf(p, bufsize, "%s", "--null epc---");
        bufsize -= written;
        p += written;
    }

    // null terminate this for good practice
    buf[buflen-1] = '\0';

}

/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one tag report entry on one line
 **
 ** @return     void
 **
 *****************************************************************************/
void
CMyApplication::printOneTagReportData (
  CTagReportData *              pTagReportData)
{
    char                        aBuf[64];

    /*
     * Print the EPC. It could be an 96-bit EPC_96 parameter
     * or an variable length EPCData parameter.
     */

    CParameter *                pEPCParameter =
                                    pTagReportData->getEPCParameter();

    formatOneEPC(pEPCParameter, aBuf, 64);
    
    /*
     * End of line
     */
    printf("EPC: %s\n", aBuf);
}


/**
 *****************************************************************************
 **
 ** @brief  Handle a ReaderEventNotification
 **
 ** Handle the payload of a READER_EVENT_NOTIFICATION message.
 ** This routine simply dispatches to handlers of specific
 ** event types.
 **
 ** @return     void
 **
 *****************************************************************************/

void
CMyApplication::handleReaderEventNotification (
  CReaderEventNotificationData *pNtfData)
{
    CAntennaEvent *             pAntennaEvent;
    CReaderExceptionEvent *     pReaderExceptionEvent;
    int                         nReported = 0;

    pAntennaEvent = pNtfData->getAntennaEvent();
    if(NULL != pAntennaEvent)
    {
        handleAntennaEvent(pAntennaEvent);
        nReported++;
    }

    pReaderExceptionEvent = pNtfData->getReaderExceptionEvent();
    if(NULL != pReaderExceptionEvent)
    {
        handleReaderExceptionEvent(pReaderExceptionEvent);
        nReported++;
    }

    /*
     * Similarly handle other events here:
     *      HoppingEvent
     *      GPIEvent
     *      ROSpecEvent
     *      ReportBufferLevelWarningEvent
     *      ReportBufferOverflowErrorEvent
     *      RFSurveyEvent
     *      AISpecEvent
     *      ConnectionAttemptEvent
     *      ConnectionCloseEvent
     *      Custom
     */

    if(0 == nReported)
    {
        printf("NOTICE: Unexpected (unhandled) ReaderEvent\n");
    }
}


/**
 *****************************************************************************
 **
 ** @brief  Handle an AntennaEvent
 **
 ** An antenna was disconnected or (re)connected. Tattle.
 **
 ** @return     void
 **
 *****************************************************************************/

void
CMyApplication::handleAntennaEvent (
  CAntennaEvent *               pAntennaEvent)
{
    EAntennaEventType           eEventType;
    llrp_u16_t                  AntennaID;
    char *                      pStateStr;

    eEventType = pAntennaEvent->getEventType();
    AntennaID = pAntennaEvent->getAntennaID();

    switch(eEventType)
    {
    case AntennaEventType_Antenna_Disconnected:
        pStateStr = "disconnected";
        break;

    case AntennaEventType_Antenna_Connected:
        pStateStr = "connected";
        break;

    default:
        pStateStr = "?unknown-event?";
        break;
    }

    printf("NOTICE: Antenna %d is %s\n", AntennaID, pStateStr);
}


/**
 *****************************************************************************
 **
 ** @brief  Handle a ReaderExceptionEvent
 **
 ** Something has gone wrong. There are lots of details but
 ** all this does is print the message, if one.
 **
 ** @return     void
 **
 *****************************************************************************/

void
CMyApplication::handleReaderExceptionEvent (
  CReaderExceptionEvent *       pReaderExceptionEvent)
{
    llrp_utf8v_t                Message;

    Message = pReaderExceptionEvent->getMessage();

    if(0 < Message.m_nValue && NULL != Message.m_pValue)
    {
        printf("NOTICE: ReaderException '%.*s'\n",
             Message.m_nValue, Message.m_pValue);
    }
    else
    {
        printf("NOTICE: ReaderException but no message\n");
    }
}


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to check an LLRPStatus parameter
 **         and tattle on errors
 **
 ** Helper routine to interpret the LLRPStatus subparameter
 ** that is in all responses. It tattles on an error, if one,
 ** and tries to safely provide details.
 **
 ** This simplifies the code, above, for common check/tattle
 ** sequences.
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong, already tattled
 **
 *****************************************************************************/

int
CMyApplication::checkLLRPStatus (
  CLLRPStatus *                 pLLRPStatus,
  char *                        pWhatStr)
{
    /*
     * The LLRPStatus parameter is mandatory in all responses.
     * If it is missing there should have been a decode error.
     * This just makes sure (remember, this program is a
     * diagnostic and suppose to catch LTKC mistakes).
     */
    if(NULL == pLLRPStatus)
    {
        printf("ERROR: %s missing LLRP status\n", pWhatStr);
        return -1;
    }

    /*
     * Make sure the status is M_Success.
     * If it isn't, print the error string if one.
     * This does not try to pretty-print the status
     * code. To get that, run this program with -vv
     * and examine the XML output.
     */
    if(StatusCode_M_Success != pLLRPStatus->getStatusCode())
    {
        llrp_utf8v_t            ErrorDesc;

        ErrorDesc = pLLRPStatus->getErrorDescription();

        if(0 == ErrorDesc.m_nValue)
        {
            printf("ERROR: %s failed, no error description given\n",
                pWhatStr);
        }
        else
        {
            printf("ERROR: %s failed, %.*s\n",
                pWhatStr, ErrorDesc.m_nValue, ErrorDesc.m_pValue);
        }
        return -2;
    }

    /*
     * Victory. Everything is fine.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to do an LLRP transaction
 **
 ** Wrapper to transact a request/resposne.
 **     - Print the outbound message in XML if verbose level is at least 2
 **     - Send it using the LLRP_Conn_transact()
 **     - LLRP_Conn_transact() receives the response or recognizes an error
 **     - Tattle on errors, if any
 **     - Print the received message in XML if verbose level is at least 2
 **     - If the response is ERROR_MESSAGE, the request was sufficiently
 **       misunderstood that the reader could not send a proper reply.
 **       Deem this an error, free the message.
 **
 ** The message returned resides in allocated memory. It is the
 ** caller's obligtation to free it.
 **
 ** @return     ==NULL          Something went wrong, already tattled
 **             !=NULL          Pointer to a message
 **
 *****************************************************************************/

CMessage *
CMyApplication::transact (
  CMessage *                    pSendMsg)
{
    CConnection *               pConn = m_pConnectionToReader;
    CMessage *                  pRspMsg;

    /*
     * Print the XML text for the outbound message if
     * verbosity is 2 or higher.
     */
    if(1 < m_Verbose)
    {
        printf("\n===================================\n");
        printf("INFO: Transact sending\n");
        printXMLMessage(pSendMsg);
    }

    /*
     * Send the message, expect the response of certain type.
     * If LLRP::CConnection::transact() returns NULL then there was
     * an error. In that case we try to print the error details.
     */
    pRspMsg = pConn->transact(pSendMsg, 5000);

    if(NULL == pRspMsg)
    {
        const CErrorDetails *   pError = pConn->getTransactError();

        printf("ERROR: %s transact failed, %s\n",
            pSendMsg->m_pType->m_pName,
            pError->m_pWhatStr ? pError->m_pWhatStr : "no reason given");

        if(NULL != pError->m_pRefType)
        {
            printf("ERROR: ... reference type %s\n",
                pError->m_pRefType->m_pName);
        }

        if(NULL != pError->m_pRefField)
        {
            printf("ERROR: ... reference field %s\n",
                pError->m_pRefField->m_pName);
        }

        return NULL;
    }

    /*
     * Print the XML text for the inbound message if
     * verbosity is 2 or higher.
     */
    if(1 < m_Verbose)
    {
        printf("\n- - - - - - - - - - - - - - - - - -\n");
        printf("INFO: Transact received response\n");
        printXMLMessage(pRspMsg);
    }

    /*
     * If it is an ERROR_MESSAGE (response from reader
     * when it can't understand the request), tattle
     * and declare defeat.
     */
    if(&CERROR_MESSAGE::s_typeDescriptor == pRspMsg->m_pType)
    {
        const CTypeDescriptor * pResponseType;

        pResponseType = pSendMsg->m_pType->m_pResponseType;

        printf("ERROR: Received ERROR_MESSAGE instead of %s\n",
            pResponseType->m_pName);
        delete pRspMsg;
        pRspMsg = NULL;
    }

    return pRspMsg;
}


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to receive a message
 **
 ** This can receive notifications as well as responses.
 **     - Recv a message using the LLRP_Conn_recvMessage()
 **     - Tattle on errors, if any
 **     - Print the message in XML if verbose level is at least 2
 **
 ** The message returned resides in allocated memory. It is the
 ** caller's obligtation to free it.
 **
 ** @param[in]  nMaxMS          -1 => block indefinitely
 **                              0 => just peek at input queue and
 **                                   socket queue, return immediately
 **                                   no matter what
 **                             >0 => ms to await complete frame
 **
 ** @return     ==NULL          Something went wrong, already tattled
 **             !=NULL          Pointer to a message
 **
 *****************************************************************************/

CMessage *
CMyApplication::recvMessage (
  int                           nMaxMS)
{
    CConnection *               pConn = m_pConnectionToReader;
    CMessage *                  pMessage;

    /*
     * Receive the message subject to a time limit
     */
    pMessage = pConn->recvMessage(nMaxMS);

    /*
     * If LLRP::CConnection::recvMessage() returns NULL then there was
     * an error. In that case we try to print the error details.
     */
    if(NULL == pMessage)
    {
        const CErrorDetails *   pError = pConn->getRecvError();

        /* don't warn on timeout since this is a polling example */
        if(pError->m_eResultCode != RC_RecvTimeout)
        {
        printf("ERROR: recvMessage failed, %s\n",
            pError->m_pWhatStr ? pError->m_pWhatStr : "no reason given");
        }

        if(NULL != pError->m_pRefType)
        {
            printf("ERROR: ... reference type %s\n",
                pError->m_pRefType->m_pName);
        }

        if(NULL != pError->m_pRefField)
        {
            printf("ERROR: ... reference field %s\n",
                pError->m_pRefField->m_pName);
        }

        return NULL;
    }

    /*
     * Print the XML text for the inbound message if
     * verbosity is 2 or higher.
     */
    if(1 < m_Verbose)
    {
        printf("\n===================================\n");
        printf("INFO: Message received\n");
        printXMLMessage(pMessage);
    }

    return pMessage;
}


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to send a message
 **
 ** Wrapper to send a message.
 **     - Print the message in XML if verbose level is at least 2
 **     - Send it using the LLRP_Conn_sendMessage()
 **     - Tattle on errors, if any
 **
 ** @param[in]  pSendMsg        Pointer to message to send
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong, already tattled
 **
 *****************************************************************************/

int
CMyApplication::sendMessage (
  CMessage *                    pSendMsg)
{
    CConnection *               pConn = m_pConnectionToReader;

    /*
     * Print the XML text for the outbound message if
     * verbosity is 2 or higher.
     */
    if(1 < m_Verbose)
    {
        printf("\n===================================\n");
        printf("INFO: Sending\n");
        printXMLMessage(pSendMsg);
    }

    /*
     * If LLRP::CConnection::sendMessage() returns other than RC_OK
     * then there was an error. In that case we try to print
     * the error details.
     */
    if(RC_OK != pConn->sendMessage(pSendMsg))
    {
        const CErrorDetails *   pError = pConn->getSendError();

        printf("ERROR: %s sendMessage failed, %s\n",
            pSendMsg->m_pType->m_pName,
            pError->m_pWhatStr ? pError->m_pWhatStr : "no reason given");

        if(NULL != pError->m_pRefType)
        {
            printf("ERROR: ... reference type %s\n",
                pError->m_pRefType->m_pName);
        }

        if(NULL != pError->m_pRefField)
        {
            printf("ERROR: ... reference field %s\n",
                pError->m_pRefField->m_pName);
        }

        return -1;
    }

    /*
     * Victory
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Helper to print a message as XML text
 **
 ** Print a LLRP message as XML text
 **
 ** @param[in]  pMessage        Pointer to message to print
 **
 ** @return     void
 **
 *****************************************************************************/

void
CMyApplication::printXMLMessage (
  CMessage *                    pMessage)
{
    char                        aBuf[100*1024];

    /*
     * Convert the message to an XML string.
     * This fills the buffer with either the XML string
     * or an error message. The return value could
     * be checked.
     */

    pMessage->toXMLString(aBuf, sizeof aBuf);

    /*
     * Print the XML Text to the standard output.
     */
    printf("%s", aBuf);
}
