
/*
 *****************************************************************************
 *                                                                           *
 *                 IMPINJ CONFIDENTIAL AND PROPRIETARY                       *
 *                                                                           *
 * This source code is the sole property of Impinj, Inc.  Reproduction or    *
 * utilization of this source code in whole or in part is forbidden without  *
 * the prior written consent of Impinj, Inc.                                 *
 *                                                                           *
 * (c) Copyright Impinj, Inc. 2007,2010. All rights reserved.                *
 *                                                                           *
 *****************************************************************************/

/**
 *****************************************************************************
 **
 ** @file  docsample5.cpp
 **
 ** @brief LLRP Examples Implementing Monza QT features
 **
 ** This shows a simple command line utility to demonstrate how to use
 ** the Impinj QT feature set.  You must have an Octane 4.4 or later
 ** reader and Monza 4 QT tags to run this code
 **
 ** The following options are available for QT
 ** 
 ** usage: 
 **     -p <password> -- specify an optional password for operations
 **     -n <password> -- specify a new password 
 **     -t  -- specify to automatically backscatter the TID
 **     -s  -- if setting QT config, -s will short range the tag
 **     -q <n>  -- run QT scenario n where n is defined as 
 **         0 -- Read standard TID memory
 **         1 -- set tag password (uses -p, -n)
 **         2 -- Read private memory data without QT commands
 **         3 -- read QT status of tag (uses -p)
 **         4 -- set QT status of tag to private (uses -p, -s)
 **         5 -- set QT status of tag to public (uses -p, -s)
 **         6 -- Peek at private memory data with temporary QT command (uses -p)
 **         7 -- Write 32 words of user data to random values
 **         8 -- Write 6 words of public EPC data to random values
 **         9 -- Read Reserved memory
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
private:

    unsigned int m_PowerLevelIndex;
    unsigned int m_messageID;

  public:
    /* Store the command line parsing here */ 
    /* Verbose level, incremented by each -v on command line */
    int                         m_Verbose;
    unsigned int                m_password;
    unsigned short int          m_qtmode;
    unsigned short int          m_tid;
    EImpinjQTAccessRange        m_shortRange;
    unsigned int                m_newPassword;

    /** Connection to the LLRP reader */
    CConnection *               m_pConnectionToReader;

    inline
    CMyApplication (void)
     : m_Verbose(0), m_pConnectionToReader(NULL)
    {
        m_messageID = 0;
        m_shortRange=ImpinjQTAccessRange_Normal_Range;
        m_password = 0;
        m_qtmode = 0;
        m_tid = 0;
        m_Verbose = 0;        
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
    getReaderCapabilities(void);

    int
    setImpinjReaderConfig(void);

    int
    addROSpec (void);

    int
    enableROSpec (void);

    int
    startROSpec (void);

    int
    stopROSpec (void);

    int 
    addAccessSpec(void);

    int
    enableAccessSpec(void);

    int
    awaitAndPrintReport (int timeoutSec);

    void
    printTagReportData (
      CRO_ACCESS_REPORT *       pRO_ACCESS_REPORT);

    void
    printOneTagReportData (
      CTagReportData *          pTagReportData);

    int
    formatOneEPC (
      CParameter           *pEPCParameter, 
      char *                buf, 
      int                   buflen,
      char *                startStr);

    int
    formatOneWriteResult (
      CParameter *          pWriteResilt,
      char *                buf,
      int                   buflen,
      char *                startStr);
    int
    formatOneSetQTConfigResult (
      CParameter *          pQTResilt,
      char *                buf,
      int                   buflen,
      char *                startStr);

    int
    formatOneGetQTConfigResult (
      CParameter *          pQTResilt,
      char *                buf,
      int                   buflen,
      char *                startStr);

    int
    formatOneSerializedTID (
      CParameter *          pTID,
      char *                buf,
      int                   buflen,
      char *                startStr);

    int
    formatOneReadResult (
      CParameter *          pReadResult,
      char *                buf,
      int                   buflen,
      char *                startStr);

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
 **     example1 [-v 0 -q 1 -p 23432] READERHOSTNAME
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
    int                         i;

    /*
     * Process comand arguments, determine reader name
     * and verbosity level.
     */

    if( ac < 2)
    {
        usage(av[0]);
        return -1;
    }

    srand((unsigned int) time(NULL));


    /* get the options. Skip the last one as its the hostname */
    for( i = 1; i < ac-1; i++)
    {
        if((0 == strcmp(av[i],"-p")) && (i < (ac-1)))
        {
            i++;
            myApp.m_password = atoi(av[i]);    
        }
        else if((0 == strcmp(av[i],"-n")) && (i < (ac-1)))
        {
            i++;
            myApp.m_newPassword = atoi(av[i]);    
        }        
        else if(0==strcmp(av[i], "-t"))
        {
            myApp.m_tid=1;
        }
        else if(0==strcmp(av[i], "-s"))
        {
            myApp.m_shortRange=ImpinjQTAccessRange_Short_Range ;
        }
        else if((0 == strcmp(av[i],"-v")) && (i < (ac-1)))
        {
            i++;
            myApp.m_Verbose = atoi(av[i]);    
        }
        else if((0 == strcmp(av[i],"-q")) && (i < (ac-1)))
        {
            i++;
            myApp.m_qtmode = atoi(av[i]);  
        }
        else
        {
            usage(av[0]);
            return -1;
        }
    }

    pReaderHostName = av[i];

    /*
     * Run application, capture return value for exit status
     */
    rc = myApp.run(pReaderHostName );

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
    printf("Usage: %s [options] READERHOSTNAME\n", pProgName);
    printf("     -p <password> -- specify an optional password for operations\n");
    printf("     -n <password> -- specifies a new password for the set password command\n");
    printf("     -t  -- specify to automatically backscatter the TID\n");
    printf("     -s  -- if setting QT config, -s will short range the tag\n");
    printf("     -q <n>  -- run QT scenario n where n is defined as \n");
    printf("         0 -- Read standard TID memory\n");
    printf("         1 -- set tag password (uses -p, -n )\n");
    printf("         2 -- Read private memory data without QT commands\n");
    printf("         3 -- read QT status of tag (uses -p)\n");
    printf("         4 -- set QT status of tag to private (uses -p, -s)\n");
    printf("         5 -- set QT status of tag to public (uses -p, -s)\n");
    printf("         6 -- Peek at private memory data with temporary QT command (uses -p)\n");   
    printf("         7 -- Write 32 words of user data to random values\n");
    printf("         8 -- Write 6 words of public EPC data to random values\n");
    printf("         9 -- Read Reserved memory\n");
    printf("\n");

    exit(1);
}


/**
 *****************************************************************************
 **
 ** @brief  Run the application
 **
 ** The steps:
 ** 1.  Initialize Library
 ** 2.  Connect to Reader
 ** 3.  Enable Impinj Extensions
 ** 4.  Factory Default LLRP configuration to ensure that the reader is in a 
 **     known state (since we are relying on the default reader configuration 
 **     for this simple example)
 ** 5.  SET_READER_CONFIG with the appropriate settings for report generation 
 **     as well as Impinj Low Duty Cycle mode to reduce interference.
 ** 6.  ADD_ROSPEC to tell the reader to perform an inventory. Include tag 
 **     filters to reduce unwanted reads from other RFID applications
 ** 7.  ADD_ACCESSSPEC to tell the reader to read user memory for personnel tags.
 ** 8.  ENABLE_ROSPEC
 ** 9.  ENABLE_ACCESSSPEC
 ** 10. START_ROSPEC start the inventory operation
 ** 11  Use GET_REPORT to for RFID data and Process RFID Data (EPC, and Timestamp)
 **
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
 **              4              Get Reader Capabilities failed
 **              5              Setting new Reader config failed
 **              6              Adding ROSpec failed
 **              7              Adding AccessSpec failed
 **              8              Enabling AccessSpec failed
 **              9              Enabling ROSpec failed
 **              10              Start ROSpec failed
 **              11             Something went wrong running the ROSpec
 **              12             Stopping ROSpec failed
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
                if(0 == getReaderCapabilities())
				{
					rc = 5;
					if(0 == setImpinjReaderConfig())
					{
						rc = 6;
						if(0 == addROSpec())
						{
							rc = 7;
							if(0 == addAccessSpec())
							{
								rc = 8;
								if(0 == enableAccessSpec())
								{
									rc = 9;
									if(0 == enableROSpec())
									{
										rc = 10;
										if(0 == startROSpec())
										{
											rc = 11;
											if(0 == awaitAndPrintReport(1))
											{
												rc = 12;
												if(0 == stopROSpec())
												{
													rc = 0;
												}
											}
										}
									}
								}
                            }
                        }
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
 ** @brief  Sends a CGET_READER_CAPABILITIES message and parses reply
 **
 ** Gets the capabilities from the reader and looks for the 
 ** reader manufacturer and model number.  
 **
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/
int
CMyApplication::getReaderCapabilities(void)
{
    CGET_READER_CAPABILITIES          *pCmd;
    CMessage *                         pRspMsg;
    CGET_READER_CAPABILITIES_RESPONSE *pRsp;
	  CGeneralDeviceCapabilities		  *pDevCap;
    unsigned int bMajorVersion, bMinorVersion, bDevVersion, bBuildVersion = 0;


    /*
     * Compose the command message
     */
    pCmd = new CGET_READER_CAPABILITIES();
    pCmd->setMessageID(m_messageID++);
    pCmd->setRequestedData(GetReaderCapabilitiesRequestedData_All);

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
     * Cast to a CGET_READER_CAPABILITIES_RESPONSE message.
     */
    pRsp = (CGET_READER_CAPABILITIES_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(),
                        "getReaderCapabilities"))
    {
        /* checkLLRPStatus already tattled */
        delete pRspMsg;
        return -1;
    }

    /*
    ** Get out the general device capabilities
    */
	if(NULL == (pDevCap = pRsp->getGeneralDeviceCapabilities()))
    {
        delete pRspMsg;
        return -1;
    }


	/* if this parameter is missing, or if this is not an Impinj
	** reader, we can't determine its capabilities so we exit
	** Impinj Private Enterprise NUmber is 25882 */
	if( (NULL == (pDevCap = pRsp->getGeneralDeviceCapabilities())) || 
		(25882 != pDevCap->getDeviceManufacturerName()))
	{
        delete pRspMsg;
        return -1;
	}


  /*
   * Get the version information from the reader and make sure we are 4.4 or better.
   */
  if  ( pDevCap->getReaderFirmwareVersion().m_nValue < 3)
  {
        printf("ERROR: Must have Firmware 4.4 or later for low level data example \n");
        delete pRspMsg;
        return -1;
  }

  /*
   * Parse to make sure it is really 4.4 or better
   */
  sscanf((char *) pDevCap->getReaderFirmwareVersion().m_pValue, "%u.%u.%u.%u", &bMajorVersion, &bMinorVersion, &bDevVersion, &bBuildVersion);

    if( (bMajorVersion < 4) && (bMinorVersion < 4) )
    {
        printf("ERROR: Must have Firmware 4.4 or later for low level data example \n");
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
        printf("INFO: Found LLRP Capabilities \n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Sends a CSET_READER_CONFIG message 
 **
 ** Sets up the impinj configuration to match the use case defined in the
 ** LTK Programmers Guide.  This could have been combined with the factory
 ** default setting, but we 
 **
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/
int
CMyApplication::setImpinjReaderConfig(void)
{
    CSET_READER_CONFIG          *pCmd;
    CMessage *                  pRspMsg;
    CSET_READER_CONFIG_RESPONSE *pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CSET_READER_CONFIG();
    pCmd->setMessageID(m_messageID++);

    CAntennaConfiguration *pAnt = new(CAntennaConfiguration);

    /*
    ** Apply this configuration to all antennas 
    */
    pAnt->setAntennaID(0);

    /*
    ** Create the container Inventory command to hold all the parameters 
    */
    CC1G2InventoryCommand *pC1G2Inv = new CC1G2InventoryCommand();
    
    /* set the mode to auto-set max throughput */
    CC1G2RFControl *pC1G2Rf = new CC1G2RFControl();
    pC1G2Rf->setModeIndex(1002);
    pC1G2Rf->setTari(0);        /* tari is ignored by the reader */
    pC1G2Inv->setC1G2RFControl(pC1G2Rf);

    CC1G2SingulationControl *pC1G2Sing = new CC1G2SingulationControl();    
    pC1G2Sing->setSession(1);
    pC1G2Sing->setTagPopulation(1);
    pC1G2Sing->setTagTransitTime(0);
    pC1G2Inv->setC1G2SingulationControl(pC1G2Sing);

    pC1G2Inv->setTagInventoryStateAware(false);

    /* set the Impinj Inventory search mode as per the use case */
    CImpinjInventorySearchMode *pImpIsm = new CImpinjInventorySearchMode();
    pImpIsm->setInventorySearchMode(ImpinjInventorySearchType_Single_Target);
    pC1G2Inv->addCustom(pImpIsm);

    /* 
    ** set the Impinj Low Duty Cycle mode as per the use case 
    */
    CImpinjLowDutyCycle *pImpLdc = new CImpinjLowDutyCycle();
    pImpLdc->setEmptyFieldTimeout(10000);
    pImpLdc->setFieldPingInterval(200);
    pImpLdc->setLowDutyCycleMode(ImpinjLowDutyCycleMode_Enabled);
    pC1G2Inv->addCustom(pImpLdc);

    /*
    ** Don't forget to add the InventoryCommand to the antenna
    ** configuration, and then add the antenna configuration to the
    ** config message
    */
    pAnt->addAirProtocolInventoryCommandSettings(pC1G2Inv);
    pCmd->addAntennaConfiguration(pAnt);

    /* 
    ** report every tag (N=1) since that is required for tag direction 
    */
    CROReportSpec *pROrs = new CROReportSpec();
    pROrs->setROReportTrigger(ROReportTriggerType_Upon_N_Tags_Or_End_Of_ROSpec);
    pROrs->setN(1);

    /* 
    ** Turn off off report data that we don't need since our use 
    ** case suggests we are bandwidth constrained 
    */
    CTagReportContentSelector *pROcontent = new CTagReportContentSelector();
    pROcontent->setEnableAccessSpecID(false);
    pROcontent->setEnableAntennaID(false);
    pROcontent->setEnableChannelIndex(false);
    pROcontent->setEnableFirstSeenTimestamp(true);
    pROcontent->setEnableInventoryParameterSpecID(false);
    pROcontent->setEnableLastSeenTimestamp(false);
    pROcontent->setEnablePeakRSSI(false);
    pROcontent->setEnableROSpecID(false);
    pROcontent->setEnableSpecIndex(false);
    pROcontent->setEnableTagSeenCount(false);
    CC1G2EPCMemorySelector *pC1G2Mem = new CC1G2EPCMemorySelector();
    pC1G2Mem->setEnableCRC(false);
    pC1G2Mem->setEnablePCBits(false);
    pROcontent->addAirProtocolEPCMemorySelector(pC1G2Mem);
    pROrs->setTagReportContentSelector(pROcontent);

    /* Optionally turn on EPC/TID backscatter. leave the others off as
    ** low level data is another example */
    CImpinjTagReportContentSelector * pImpTagCnt = new CImpinjTagReportContentSelector();
    
    CImpinjEnableRFPhaseAngle       * pEnableRfPhase = new CImpinjEnableRFPhaseAngle();
    pEnableRfPhase->setRFPhaseAngleMode(ImpinjRFPhaseAngleMode_Disabled);
    pImpTagCnt->setImpinjEnableRFPhaseAngle(pEnableRfPhase);

    CImpinjEnablePeakRSSI       * pEnablePeakRssi = new CImpinjEnablePeakRSSI();
    pEnablePeakRssi->setPeakRSSIMode(ImpinjPeakRSSIMode_Disabled);
    pImpTagCnt->setImpinjEnablePeakRSSI(pEnablePeakRssi);

    CImpinjEnableSerializedTID  * pEnableSerializedTID = new CImpinjEnableSerializedTID();

    /* Here's where we set the backscatter of the TID */
    if(m_tid)
	    pEnableSerializedTID->setSerializedTIDMode(ImpinjSerializedTIDMode_Enabled);
    else
	    pEnableSerializedTID->setSerializedTIDMode(ImpinjSerializedTIDMode_Disabled);

    pImpTagCnt->setImpinjEnableSerializedTID(pEnableSerializedTID);   

    pROrs->addCustom(pImpTagCnt);    

    pCmd->setROReportSpec(pROrs);

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
     * Cast to a CSET_READER_CONFIG_RESPONSE message.
     */
    pRsp = (CSET_READER_CONFIG_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(),
                        "setImpinjReaderConfig"))
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
        printf("INFO: Set Impinj Reader Configuration \n");
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
 **
 ** This example creates the simplex ROSpec.  It starts and stops
 ** based on user command.  It enables all antennas and uses all 
 ** the default values set by the SET_READER_CONFIG message.
 **
 ** The simplest ROSpec is augmented with the filters necessary
 ** to 
 **
 ** The message is
 **
.** <ROSpec>
.**     <ROSpecID>1111</ROSpecID>
.**     <Priority>0</Priority>
.**     <CurrentState>Disabled</CurrentState>
.**     <ROBoundarySpec>
.**         <ROSpecStartTrigger>
.**             <ROSpecStartTriggerType>Null</ROSpecStartTriggerType>
.**         </ROSpecStartTrigger>
.**         <ROSpecStopTrigger>
.**             <ROSpecStopTriggerType>Null</ROSpecStopTriggerType>
.**             <DurationTriggerValue>0</DurationTriggerValue>
.**         </ROSpecStopTrigger>
.**     </ROBoundarySpec>
.**     <AISpec>
.**         <AntennaIDs>0</AntennaIDs>
.**         <AISpecStopTrigger>
.**             <AISpecStopTriggerType>Null</AISpecStopTriggerType>
.**             <DurationTrigger>0</DurationTrigger>
.**         </AISpecStopTrigger>
.**         <InventoryParameterSpec>
.**             <InventoryParameterSpecID>1234</InventoryParameterSpecID>
.**             <ProtocolID>EPCGlobalClass1Gen2</ProtocolID>
.**              <AntennaConfiguration>
.**              <AntennaID>0</AntennaID>
.**              <C1G2InventoryCommand>
 **                  <TagInventoryStateAware>false</TagInventoryStateAware>
 **                  <C1G2Filter>
 **                      <T>Do_Not_Truncate</T>
 **                      <C1G2TagInventoryMask>
 **                          <MB>1</MB>
 **                          <Pointer>32</Pointer>
 **                          <!-- pointer is in decimal -->
 **                          <TagMask Count="8">33</TagMask>
 **                      </C1G2TagInventoryMask>
 **                      <C1G2TagInventoryStateUnawareFilterAction>
 **                          <Action>Select_Unselect</Action>
 **                      </C1G2TagInventoryStateUnawareFilterAction>
 **                  </C1G2Filter>
 **                  <C1G2Filter>
 **                      <T>Do_Not_Truncate</T>
 **                      <C1G2TagInventoryMask>
 **                          <MB>1</MB>
 **                          <Pointer>32</Pointer>
 **                          <!-- pointer is in decimal -->
 **                          <TagMask Count="8">35</TagMask>
 **                      </C1G2TagInventoryMask>
 **                      <C1G2TagInventoryStateUnawareFilterAction>
 **                          <Action>Select_DoNothing</Action>
 **                      </C1G2TagInventoryStateUnawareFilterAction>
 **                  </C1G2Filter>
 **              </C1G2InventoryCommand>
 **             </AntennaConfiguration>
.**         </InventoryParameterSpec>
.**     </AISpec>
.** </ROSpec>
.**</ADD_ROSPEC>
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
                                ROSpecStartTriggerType_Null);

    CROSpecStopTrigger *        pROSpecStopTrigger = new CROSpecStopTrigger();
    pROSpecStopTrigger->setROSpecStopTriggerType(ROSpecStopTriggerType_Null);
    pROSpecStopTrigger->setDurationTriggerValue(0);     /* n/a */

    CROBoundarySpec *           pROBoundarySpec = new CROBoundarySpec();
    pROBoundarySpec->setROSpecStartTrigger(pROSpecStartTrigger);
    pROBoundarySpec->setROSpecStopTrigger(pROSpecStopTrigger);

    CAISpecStopTrigger *        pAISpecStopTrigger = new CAISpecStopTrigger();
    pAISpecStopTrigger->setAISpecStopTriggerType(
            AISpecStopTriggerType_Null);
    pAISpecStopTrigger->setDurationTrigger(0);

    CInventoryParameterSpec *   pInventoryParameterSpec =
                                    new CInventoryParameterSpec();
    pInventoryParameterSpec->setInventoryParameterSpecID(1234);
    pInventoryParameterSpec->setProtocolID(AirProtocols_EPCGlobalClass1Gen2);

    /* Build the antennaConfiguration to Contain this */
    CAntennaConfiguration * pAntennaConfiguration = 
                                    new CAntennaConfiguration();
    pAntennaConfiguration->setAntennaID(0);

    /* don't forget to add this to the INventory Parameter Spec above */
    pInventoryParameterSpec->addAntennaConfiguration(pAntennaConfiguration);

    /* 
    ** Use all Antennas
    */
    llrp_u16v_t                 AntennaIDs = llrp_u16v_t(1);
    AntennaIDs.m_pValue[0] = 0;

    CAISpec *                   pAISpec = new CAISpec();
    pAISpec->setAntennaIDs(AntennaIDs);
    pAISpec->setAISpecStopTrigger(pAISpecStopTrigger);
    pAISpec->addInventoryParameterSpec(pInventoryParameterSpec);

    CROSpec *                   pROSpec = new CROSpec();
    pROSpec->setROSpecID(1111);
    pROSpec->setPriority(0);
    pROSpec->setCurrentState(ROSpecState_Disabled);
    pROSpec->setROBoundarySpec(pROBoundarySpec);
    pROSpec->addSpecParameter(pAISpec);

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
    pCmd->setROSpecID(1111);

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
    pCmd->setROSpecID(1111);

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
    pCmd->setROSpecID(1111);

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
 ** @brief Add an AccessSpec using ADD_ACCESSSPEC message
 **
 ** Adds an access spec to perform the appropriate QT operation
 **
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/
int
CMyApplication::addAccessSpec (void)
{
    CADD_ACCESSSPEC *            pCmd;
    CMessage *                   pRspMsg;
    CADD_ACCESSSPEC_RESPONSE *   pRsp;

    pCmd = new CADD_ACCESSSPEC();
    pCmd->setMessageID(m_messageID++);

    /* build the C1G2Target Tag with the AccessSpec filter */
    CC1G2TargetTag *ptargetTag = new CC1G2TargetTag();
    ptargetTag->setMatch(true);
    ptargetTag->setMB(1);
    ptargetTag->setPointer(16);
    
    /* match any tag */
    llrp_u1v_t tagData = llrp_u1v_t(0);
    tagData.m_nBit = 0;
    ptargetTag->setTagData(tagData);
    llrp_u1v_t tagMask = llrp_u1v_t(0);
    tagMask.m_nBit = 0;
    ptargetTag->setTagMask(tagMask);

    /* build the AirProtocolTagSpec Add the filter */
    CC1G2TagSpec *ptagSpec = new CC1G2TagSpec();
    ptagSpec->addC1G2TargetTag(ptargetTag);

    /* Create the AccessCommand.  Add the TagSpec and the OpSpec */
    CAccessCommand *pAccessCommand = new CAccessCommand();
    pAccessCommand->setAirProtocolTagSpec(ptagSpec);

    /* Build the read Op Spec and add to the access command */
    /*
     **     -q <n>  -- run QT scenario n where n is defined as 
     **         0 -- Read standard TID memory
     **         1 -- set tag password (uses -p, -n)
     **         2 -- Read private memory data without QT commands
     **         3 -- read QT status of tag (uses -p)
     **         4 -- set QT status of tag to private (uses -p, -s)
     **         5 -- set QT status of tag to public (uses -p, -s)
     **         6 -- Peek at private memory data with temporary QT command (uses -p)
     **         7 -- write random data to user memory
     **         8 -- write random data to the public EPC space
    */

    switch(m_qtmode)
    {
    case 0:
        {
            CC1G2Read *preadStdTID = new CC1G2Read();
            preadStdTID->setAccessPassword(0);     /* no access required to read */
            preadStdTID->setMB(2);
            preadStdTID->setOpSpecID(1);
            preadStdTID->setWordCount(2);      /* standard TID */
            preadStdTID->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadStdTID);
        }
        break;
    case 1:
        {
            llrp_u16v_t Data = llrp_u16v_t(2);
            Data.m_pValue[0] = ((m_newPassword >> 16) & 0x0000ffff);
            Data.m_pValue[1] = (m_newPassword & 0x0000ffff);

            CC1G2Write *pwrite = new CC1G2Write();
            pwrite->setOpSpecID(10);
            pwrite->setMB(0);
            pwrite->setAccessPassword(m_password);
            pwrite->setWordPointer(2);
            pwrite->setWriteData(Data);
            pAccessCommand->addAccessCommandOpSpec(pwrite);
        }
        break;
    case 2:
        {
            CC1G2Read *preadSTID = new CC1G2Read();
            preadSTID->setAccessPassword(0);     /* no access required to read */
            preadSTID->setMB(2);
            preadSTID->setOpSpecID(2);
            preadSTID->setWordCount(6);      /* standard TID plus 48 bit STID */
            preadSTID->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadSTID);

            CC1G2Read *preadPEPC = new CC1G2Read();
            preadPEPC->setAccessPassword(0);     /* no access required to read */
            preadPEPC->setMB(2);
            preadPEPC->setOpSpecID(2);
            preadPEPC->setWordCount(6);      /* Public EPC memory */
            preadPEPC->setWordPointer(6);
            pAccessCommand->addAccessCommandOpSpec(preadPEPC);

            CC1G2Read *preadUser = new CC1G2Read();
            preadUser->setAccessPassword(0);     /* no access required to read */
            preadUser->setMB(3);
            preadUser->setOpSpecID(3);
            preadUser->setWordCount(32);        /* 512 bits of user memory */
            preadUser->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadUser);
        }
        break;
    default:
        /* for unknown, just try to read QT status */
    case 3:
        {
            CImpinjGetQTConfig *pgetQT = new CImpinjGetQTConfig();
            pgetQT->setAccessPassword(m_password);  /* use password if provided */
            pgetQT->setOpSpecID(4);
            pAccessCommand->addAccessCommandOpSpec(pgetQT);
        }
        break;
    case 4:
        {
            CImpinjSetQTConfig *psetQT = new CImpinjSetQTConfig();
            psetQT->setAccessPassword(m_password);
            psetQT->setOpSpecID(5);
            psetQT->setAccessRange(m_shortRange);
            psetQT->setDataProfile(ImpinjQTDataProfile_Private);
            psetQT->setPersistence(ImpinjQTPersistence_Permanent);
            pAccessCommand->addAccessCommandOpSpec(psetQT);
        }
        break;
    case 5:
        {
            CImpinjSetQTConfig *psetQT = new CImpinjSetQTConfig();
            psetQT->setAccessPassword(m_password);
            psetQT->setOpSpecID(6);
            psetQT->setAccessRange(m_shortRange);
            psetQT->setDataProfile(ImpinjQTDataProfile_Public);
            psetQT->setPersistence(ImpinjQTPersistence_Permanent);
            pAccessCommand->addAccessCommandOpSpec(psetQT);
        }
        break;
    case 6:
        {
            CImpinjSetQTConfig *psetQT = new CImpinjSetQTConfig();
            psetQT->setAccessPassword(m_password);
            psetQT->setOpSpecID(6);
            psetQT->setAccessRange(ImpinjQTAccessRange_Normal_Range );
            psetQT->setDataProfile(ImpinjQTDataProfile_Private);
            psetQT->setPersistence(ImpinjQTPersistence_Temporary );
            pAccessCommand->addAccessCommandOpSpec(psetQT);

            CC1G2Read *preadPrivEPC = new CC1G2Read();
            preadPrivEPC->setAccessPassword(0);     /* no access required to read */
            preadPrivEPC->setMB(1);
            preadPrivEPC->setOpSpecID(7);
            preadPrivEPC->setWordCount(8);      /* assume 128-bit */
            preadPrivEPC->setWordPointer(2);
            pAccessCommand->addAccessCommandOpSpec(preadPrivEPC);

            CC1G2Read *preadSTID = new CC1G2Read();
            preadSTID->setAccessPassword(0);     /* no access required to read */
            preadSTID->setMB(2);
            preadSTID->setOpSpecID(8);
            preadSTID->setWordCount(6);      /* standard TID plus 48 bit STID */
            preadSTID->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadSTID);

            CC1G2Read *preadUser = new CC1G2Read();
            preadUser->setAccessPassword(0);     /* no access required to read */
            preadUser->setMB(3);
            preadUser->setOpSpecID(9);
            preadUser->setWordCount(32);
            preadUser->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadUser);
        }
        break;
    case 7:
        {
            llrp_u16v_t Data = llrp_u16v_t(32);

            for(int x = 0; x < 32; x++)
            {
                Data.m_pValue[x] = rand();
            }
            
            CC1G2Write *pwrite = new CC1G2Write();
            pwrite->setOpSpecID(10);
            pwrite->setMB(3);
            pwrite->setAccessPassword(m_password);
            pwrite->setWordPointer(0);
            pwrite->setWriteData(Data);
            pAccessCommand->addAccessCommandOpSpec(pwrite);
        }
        break;
    case 8:
        {
            llrp_u16v_t Data = llrp_u16v_t(6);

            for(int x = 0; x < 6; x++)
            {
                Data.m_pValue[x] = rand();
            }
            
            CC1G2Write *pwrite = new CC1G2Write();
            pwrite->setOpSpecID(11);
            pwrite->setMB(2);
            pwrite->setAccessPassword(m_password);
            pwrite->setWordPointer(6);
            pwrite->setWriteData(Data);
            pAccessCommand->addAccessCommandOpSpec(pwrite);
        }
        break;
    case 9:
        {
            CC1G2Read *preadRsvd = new CC1G2Read();
            preadRsvd->setAccessPassword(0);     /* no access required to read */
            preadRsvd->setMB(0);
            preadRsvd->setOpSpecID(12);
            preadRsvd->setWordCount(4);      /* Access and Kill */
            preadRsvd->setWordPointer(0);
            pAccessCommand->addAccessCommandOpSpec(preadRsvd);
        }
        break;
    }
    
    /* set up the Access Report Spec rule to report only with ROSpecs */
    CAccessReportSpec *pAccessReportSpec = new CAccessReportSpec();
    pAccessReportSpec->setAccessReportTrigger(
            AccessReportTriggerType_Whenever_ROReport_Is_Generated);

    /* set up the stop trigger for the access spec. Do not stop */
    CAccessSpecStopTrigger *pAccessStopTrigger = new CAccessSpecStopTrigger();
    pAccessStopTrigger->setAccessSpecStopTrigger(
        AccessSpecStopTriggerType_Null);
    pAccessStopTrigger->setOperationCountValue(0);      /* ignored */

    /* Create and configure the AccessSpec */
    CAccessSpec *pAccessSpec = new CAccessSpec();
    pAccessSpec->setAccessSpecID(23);
    pAccessSpec->setAntennaID(0);       /* valid for all antennas */
    pAccessSpec->setCurrentState(AccessSpecState_Disabled);
    pAccessSpec->setProtocolID(AirProtocols_EPCGlobalClass1Gen2);
    pAccessSpec->setROSpecID(0);        /* valid for All RoSpecs */
    pAccessSpec->setAccessSpecStopTrigger(pAccessStopTrigger);
    pAccessSpec->setAccessReportSpec(pAccessReportSpec);
    pAccessSpec->setAccessCommand(pAccessCommand);

    /* Add the AccessSpec to the ADD_ACCESS_SPEC message */
    pCmd->setAccessSpec(pAccessSpec);

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
     * Cast to a ADD_ACCESSSPEC_RESPONSE message.
     */
    pRsp = (CADD_ACCESSSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "addAccessSpec"))
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
        printf("INFO: AccessSpec added\n");
    }

    /*
     * Victory.
     */
    return 0;
}


/**
 *****************************************************************************
 **
 ** @brief  Enable our AccessSpec using ENABLE_ACCESSSPEC message
 **
 ** Enable the AccessSpec that was added above.
 **
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

int
CMyApplication::enableAccessSpec (void)
{
    CENABLE_ACCESSSPEC *            pCmd;
    CMessage *                      pRspMsg;
    CENABLE_ACCESSSPEC_RESPONSE *   pRsp;

    /*
     * Compose the command message
     */
    pCmd = new CENABLE_ACCESSSPEC();
    pCmd->setMessageID(m_messageID++);
    pCmd->setAccessSpecID(23);

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
     * Cast to a ENABLE_ACCESSSPEC_RESPONSE message.
     */
    pRsp = (CENABLE_ACCESSSPEC_RESPONSE *) pRspMsg;

    /*
     * Check the LLRPStatus parameter.
     */
    if(0 != checkLLRPStatus(pRsp->getLLRPStatus(), "enableAccessSpec"))
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
        printf("INFO: AccessSpec enabled\n");
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
 ** @return   number of bytes written
 **
 *****************************************************************************/
int
CMyApplication::formatOneEPC (
  CParameter *pEPCParameter, 
  char *buf, 
  int buflen,
  char * startStr)
{
    char *              p = buf;
    int                 bufsize = buflen;
    int                 written = 0;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

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

    return buflen - bufsize;
}

/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one Read OpSpec Result
 **
 ** @return  void
 **
 *****************************************************************************/
int
CMyApplication::formatOneReadResult (
  CParameter *pOpSpecReadResult, 
  char *buf, 
  int buflen,
  char * startStr)
{
    EC1G2ReadResultType result;
    char *              p = buf;
    int written = 0; 
    int bufsize = buflen;
    int i;
    llrp_u16v_t  readData;
    CC1G2ReadOpSpecResult *pread = (CC1G2ReadOpSpecResult*) pOpSpecReadResult;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

    result = pread->getResult();
    written = snprintf(p, bufsize, "result=%d", result);
    p+= written;
    bufsize -= written;

    if(result == C1G2ReadResultType_Success)
    {
        readData = pread->getReadData();
        
        written = snprintf(p, bufsize, " Data=");
        p+= written;
        bufsize -= written;

        for(i = 0; i < readData.m_nValue - 1 ; i++)
        {
            written =snprintf(p, bufsize, "%04x-", readData.m_pValue[i]);
            p+= written;
            bufsize -= written;
        }
        if(readData.m_nValue)
        {
            written =snprintf(p, bufsize, "%04x", readData.m_pValue[i]);
            p+= written;
            bufsize -= written;
        }
    }
    buf[buflen-1] = '\0';
    return buflen - bufsize;
}

/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one RWriteead OpSpec Result
 **
 ** @return  void
 **
 *****************************************************************************/
int
CMyApplication::formatOneWriteResult (
  CParameter *pOpSpecReadResult, 
  char *buf, 
  int buflen,
  char * startStr)
{
    EC1G2WriteResultType result;
    char *              p = buf;
    int written = 0; 
    int bufsize = buflen;
    llrp_u16v_t  readData;
    CC1G2WriteOpSpecResult *pwrite = (CC1G2WriteOpSpecResult*) pOpSpecReadResult;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

    result = pwrite->getResult();
    written = snprintf(p, bufsize, "result=%d", result);
    p+= written;
    bufsize -= written;

    buf[buflen-1] = '\0';
    return buflen - bufsize;
}

/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one SetQTConfig OpSpec Result
 **
 ** @return  void
 **
 *****************************************************************************/
int
CMyApplication::formatOneSetQTConfigResult (
  CParameter *pSetQTConfig, 
  char *buf, 
  int buflen,
  char * startStr)
{
    EImpinjSetQTConfigResultType  result;
    char *              p = buf;

    int written = 0; 
    int bufsize = buflen;
    llrp_u16v_t  readData;
    CImpinjSetQTConfigOpSpecResult *pset = (CImpinjSetQTConfigOpSpecResult*) pSetQTConfig;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

    result = pset->getResult();

    written = snprintf(p, bufsize, "result=%d", result);
    p+= written;
    bufsize -= written;

    buf[buflen-1] = '\0';
    return buflen - bufsize;
}

/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one GetQTConfig OpSpec Result
 **
 ** @return  void
 **
 *****************************************************************************/
int
CMyApplication::formatOneGetQTConfigResult (
  CParameter *pSetQTConfig, 
  char *buf, 
  int buflen,
  char * startStr)
{
    EImpinjGetQTConfigResultType  result;
    char *              p = buf;
    int written = 0; 
    int bufsize = buflen;
    llrp_u16v_t  readData;
    const char * dataStrings[3] = {  "Unknown",    
                                     "Private",   
                                     "Public"};  

    const char * rangeStrings[3] = { "Unknown",
                                     "Normal",
                                     "Short"};

    CImpinjGetQTConfigOpSpecResult *pset = (CImpinjGetQTConfigOpSpecResult*) pSetQTConfig;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

    result = pset->getResult();
    written = snprintf(p, bufsize, "result=%d ", result);
    p+= written;
    bufsize -= written;

    if(ImpinjGetQTConfigResultType_Success  == result)
    {
        written = snprintf(p, bufsize, "data=%s range=%s\n",
            dataStrings[pset->getDataProfile()],
            rangeStrings[pset->getAccessRange()]);
        p+= written;
        bufsize -= written;
    }

    buf[buflen-1] = '\0';
    return buflen - bufsize;
}


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one serializedTID Result
 **
 ** @return  void
 **
 *****************************************************************************/
int
CMyApplication::formatOneSerializedTID (
  CParameter *pSerializedTID, 
  char *buf, 
  int buflen,
  char * startStr)
{
    char *              p = buf;
    int written = 0; 
    int bufsize = buflen;

    CImpinjSerializedTID *pTID = (CImpinjSerializedTID*) pSerializedTID;

    written = snprintf(p, bufsize, "%s", startStr); 
    bufsize -= written;
    p += written;

    llrp_u16v_t tid = pTID->getTID();

    for(int i = 0; i < tid.m_nValue; i++)
    {
        if(0 < i && i%2 == 0 && 1 < bufsize)
        {
            *p++ = '-';
            bufsize--;
        }
        if(bufsize > 2)
        {
            written = snprintf(p, bufsize, "%02X", tid.m_pValue[i]);
            bufsize -= written;
            p+= written;
        }
    }

    buf[buflen-1] = '\0';
    return buflen - bufsize;
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
    const int                   bufSize = 1024;
    char                        aBuf[bufSize];
    char                       *ptr = aBuf;
    int                         written = 0;
    int                         bufLimit = bufSize;

    std::list<CParameter *>::iterator OpSpecResults;
    std::list<CParameter *>::iterator Cur;

    /*
     * Print the EPC. It could be an 96-bit EPC_96 parameter
     * or an variable length EPCData parameter.
     */

    CParameter *                pEPCParameter =
                                    pTagReportData->getEPCParameter();

    written = formatOneEPC(pEPCParameter, ptr, bufLimit, "epc=");
    ptr += written;
    bufLimit-=written;
    
    aBuf[bufSize-1] = '\0';
    /*
    ** Handle a few of the op Spec result types 
    */
    for (
        OpSpecResults = pTagReportData->beginAccessCommandOpSpecResult();
        OpSpecResults != pTagReportData->endAccessCommandOpSpecResult();
        OpSpecResults++)
        {
            if( (*OpSpecResults)->m_pType == &CC1G2ReadOpSpecResult::s_typeDescriptor)
            {
                written = formatOneReadResult(*OpSpecResults, ptr, bufLimit, "\n    READ ");
                ptr += written;
                bufLimit-=written;
            }
            else if( (*OpSpecResults)->m_pType == &CC1G2WriteOpSpecResult::s_typeDescriptor)
            {
                written = formatOneWriteResult(*OpSpecResults, ptr, bufLimit, "\n    WRITE ");
                ptr += written;
                bufLimit-=written;
            }
            else if( (*OpSpecResults)->m_pType == &CImpinjSetQTConfigOpSpecResult::s_typeDescriptor)
            {
                written = formatOneSetQTConfigResult(*OpSpecResults, ptr, bufLimit, "\n    SETQT ");
                ptr += written;
                bufLimit-=written;
            }
            else if( (*OpSpecResults)->m_pType == &CImpinjGetQTConfigOpSpecResult::s_typeDescriptor)
            {
                written = formatOneGetQTConfigResult(*OpSpecResults, ptr, bufLimit, "\n    GETQT ");
                ptr += written;
                bufLimit-=written;
            } 
        }

    /* look for custom parameters like TID */
    for(
        Cur = pTagReportData->beginCustom();
        Cur != pTagReportData->endCustom();
        Cur++)
    {
        /* look for our special Impinj Tag Report Data */
        if(&CImpinjSerializedTID ::s_typeDescriptor == (*Cur)->m_pType)
        {
            written = formatOneSerializedTID(*Cur, ptr, bufLimit, "\n    SERIAL-TID ");
            ptr += written;
            bufLimit -= written;
        } 
    }

    /*
     * End of line
     */
    printf("%s\n", aBuf);
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
