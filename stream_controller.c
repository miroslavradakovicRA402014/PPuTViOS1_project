#include "stream_controller.h"
#include "tables.h"
#include "config_parser.h"
#include "graphic_controller.h"
#include <string.h>

/* stream tables */
static PatTable *patTable;
static PmtTable *pmtTable;
static EitTable *eitTable;

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

static uint32_t playerHandle = 0;
static uint32_t sourceHandle = 0;
static uint32_t streamHandleA = 0;
static uint32_t streamHandleV = 0;
static uint32_t filterHandle = 0;
static uint8_t threadExit = 0;
static bool changeChannel = false;
static bool changeVolume = false;
static bool volumeMute = false;
static int16_t programNumber = 0;
static bool isInitialized = false;
static InitConfig config; 
static char configPathname[CONFIG_NAME_LEN];


ChannelInfo currentChannel;
int16_t volumeLevel = 0;

static struct timespec lockStatusWaitTime;
static struct timeval now;
static pthread_t scThread;
static pthread_cond_t demuxCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t demuxMutex = PTHREAD_MUTEX_INITIALIZER;


static void* streamControllerTask();
static StreamControllerError startChannel(int32_t channelNumber);
static void getEvent();


StreamControllerError streamControllerInit(char* configFile)
{
	/* handle stream events in background process */
    if (pthread_create(&scThread, NULL, &streamControllerTask, NULL))
    {
        printf("Error creating input event task!\n");
        return SC_THREAD_ERROR;
    }

	strcpy(configPathname, configFile);

    return SC_NO_ERROR;
}

StreamControllerError streamControllerDeinit()
{
    if (!isInitialized) 
    {
        printf("\n%s : ERROR streamControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    threadExit = 1;
    if (pthread_join(scThread, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return SC_THREAD_ERROR;
    }
    
    /* free demux filter */  
    Demux_Free_Filter(playerHandle, filterHandle);

	/* remove audio stream */
	Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
    
    /* remove video stream */
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
    
    /* close player source */
    Player_Source_Close(playerHandle, sourceHandle);
    
    /* deinitialize player */
    Player_Deinit(playerHandle);
    
    /* deinitialize tuner device */
    Tuner_Deinit();
    
    /* free allocated memory */  
    free(patTable);
    free(pmtTable);
	free(eitTable);    

    /* set isInitialized flag */
    isInitialized = false;

    return SC_NO_ERROR;
}

StreamControllerError channelUp()
{   
    if (programNumber >= patTable->serviceInfoCount - 2)
    {
        programNumber = 0;
    } 
    else
    {
        programNumber++;
    }

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError channelDown()
{
    if (programNumber <= 0)
    {
        programNumber = patTable->serviceInfoCount - 2;
    } 
    else
    {
        programNumber--;
    }
   
    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}


StreamControllerError channelSwitch(int16_t ch)
{
	/* check that channel exists */
    if (ch > patTable->serviceInfoCount)
    {
        return SC_ERROR;
    } 
    /* set program to channel */  
    programNumber = ch;
 	
	printf("\nSwitch to channel %d \n", ch);
   
    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError volumeUp()
{   
    if (volumeLevel != MAX_VOL_LEVEL)
    {
        volumeLevel++;
    } 

	printf("\nChange volume %d \n", volumeLevel);
 
    /* set flag to start volume up */
    changeVolume = true;
	volumeMute = false;

    return SC_NO_ERROR;
}

StreamControllerError volumeDown()
{   
    if (volumeLevel != 0)
    {
        volumeLevel--;
    } 

	printf("\nChange volume %d \n", volumeLevel);
 
    /* set flag to start volume down */
    changeVolume = true;
	volumeMute = false;

    return SC_NO_ERROR;
}

StreamControllerError mute()
{   
    if (!volumeMute)
    {
        volumeMute = true;
    }
	else
	{
		volumeMute = false;		
	} 

	printf("\nMute volume %d \n", 0);
 
    /* set flag to start volume mute */
    changeVolume = true;

    return SC_NO_ERROR;
}

StreamControllerError getChannelInfo(ChannelInfo* channelInfo)
{
    if (channelInfo == NULL)
    {
        printf("\n Error wrong parameter\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    channelInfo->programNumber = currentChannel.programNumber;
    channelInfo->audioPid = currentChannel.audioPid;
    channelInfo->videoPid = currentChannel.videoPid;
    
    return SC_NO_ERROR;
}

/* Sets filter to receive current channel PMT table
 * Parses current channel PMT table when it arrives
 * Creates streams with current channel audio and video pids
 */
StreamControllerError startChannel(int32_t channelNumber)
{
    /* free PAT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);
    
    /* set demux filter for receive PMT table of program */
    if(Demux_Set_Filter(playerHandle, patTable->patServiceInfoArray[channelNumber + 1].pid, 0x02, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return SC_ERROR;
	}
    
    /* wait for a PMT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
	}
	pthread_mutex_unlock(&demuxMutex);

    /* free EIT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);

    /* set demux filter for receive EIT table of program */
    if(Demux_Set_Filter(playerHandle, 0x12, 0x4E, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return SC_ERROR;
	}
    
    /* get audio and video pids */
    int16_t audioPid = -1;
    int16_t videoPid = -1;
    uint8_t i = 0;
    bool hasTeletext = false; 
    
    
    for (i = 0; i < pmtTable->elementaryInfoCount; i++)
    {
        if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x1) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x2) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x1b))
            && (videoPid == -1))
        {
            videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
			/* check video config pids*/
			if (config.configVideoPid != videoPid && !isInitialized)
			{
				printf("\nERROR Incompatabile video pid!\n"); 
				return SC_ERROR;  	
			}
        } 
        else if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x3) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x4))
            && (audioPid == -1))
        {
            audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
			/* check audio config pids*/
			if (config.configAudioPid != audioPid && !isInitialized)
			{
				printf("\nERROR Incompatabile audio pid!\n");
  				return SC_ERROR; 	
			}
        }
        else if ((pmtTable->pmtElementaryInfoArray[i].streamType == 0x6))
        {
        	hasTeletext = true; 
        }
    }

    if (videoPid != -1) 
    {
        /* remove previous video stream */
        if (streamHandleV != 0)
        {
		    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
        }

        /* create video stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, videoPid, config.configVideoType, &streamHandleV))
        {
            printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }
	else
	{
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
	}

    if (audioPid != -1)
    {   
        /* remove previos audio stream */
        if (streamHandleA != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
            streamHandleA = 0;
        }

	    /* create audio stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, audioPid, config.configAudioType, &streamHandleA))
        {
            printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }
    
    /* store current channel info */
    currentChannel.programNumber = channelNumber;
    currentChannel.audioPid = audioPid;
    currentChannel.videoPid = videoPid;
	currentChannel.teletext = hasTeletext; 
	/* sleep until video isn't visible */
	sleep(3.9);
	/* check that radio exists */
	if (videoPid != -1)
	{
		drawRadio(false);
	}
	else
	{
		drawRadio(true);
	}
	/* draw banner */
	drawCnannel(currentChannel.programNumber);
	drawInfoBanner(currentChannel.programNumber, currentChannel.audioPid, currentChannel.videoPid, currentChannel.teletext,  currentChannel.eventTime, currentChannel.eventName);
	

	return SC_NO_ERROR;
}

void* streamControllerTask()
{
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    /* allocate memory for PAT table section */
    patTable=(PatTable*)malloc(sizeof(PatTable));
    if(patTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(patTable, 0x0, sizeof(PatTable));

    /* allocate memory for PMT table section */
    pmtTable=(PmtTable*)malloc(sizeof(PmtTable));
    if(pmtTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(pmtTable, 0x0, sizeof(PmtTable));

    /* allocate memory for EIT table section */
    eitTable=(EitTable*)malloc(sizeof(EitTable));
    if(eitTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(eitTable, 0x0, sizeof(EitTable));
      
    /* initialize tuner device */
    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
		free(eitTable);
        return (void*) SC_ERROR;
    }
    
    /* register tuner status callback */
    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}
    
	/* parse init config file */
	if (!parseConfigFile(configPathname, &config))
	{
	   printf("\nERROR parseConfigFile() fail\n");		
	   return (void*) SC_ERROR;		
	}
	
	if (config.configFreq != 754000000)
	{
	   printf("\nERROR Frequency doesn't exist!\n");		
	   return (void*) SC_ERROR;		
	}
	
    /* lock to frequency */
    if(!Tuner_Lock_To_Frequency(config.configFreq, config.configBandwidth, config.configModule))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,config.configFreq);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,config.configFreq);
        free(patTable);
        free(pmtTable);
		free(eitTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    
    /* wait for tuner to lock */
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n",__FUNCTION__);
        free(patTable);
        free(pmtTable);
		free(eitTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    pthread_mutex_unlock(&statusMutex);
   
    /* initialize player */
    if(Player_Init(&playerHandle))
    {
		printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
		free(patTable);
        free(pmtTable);
		free(eitTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	
	/* open source */
	if(Player_Source_Open(playerHandle, &sourceHandle))
    {
		printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
		free(patTable);
        free(pmtTable);
        free(eitTable);	
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;	
	}

	/* set PAT pid and tableID to demultiplexer */
	if(Demux_Set_Filter(playerHandle, 0x00, 0x00, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
	}
	
	/* register section filter callback */
    if(Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
    {
		printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
	}

    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s:ERROR Lock timeout exceeded!\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
		free(eitTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	pthread_mutex_unlock(&demuxMutex);


	/* set program number to config program number */
	if (config.configProgramNumber > patTable->serviceInfoCount - 2)
	{
		printf("\nERROR Config channel doesn't exist!\n");		
		return (void*) SC_ERROR;				
 	}
	programNumber = config.configProgramNumber;    

    /* start current channel if pid correct */
    startChannel(programNumber);
	        
    /* set isInitialized flag */
    isInitialized = true;

	/* stream controller loop */
    while(!threadExit)
    {
    	/* check change channel event */
        if (changeChannel)
        {
            changeChannel = false;
            startChannel(programNumber);
			printf("\nSwitched to channel %d\n", programNumber);
        }
    	/* check change volume event */
		if (changeVolume)
        {
            changeVolume = false;
			if (!volumeMute)
			{
				if (Player_Volume_Set(playerHandle, volumeLevel * 165400000))
				{
					printf("\n%s : ERROR Player_Volume_Set() fail\n", __FUNCTION__);
				}
			}
			else
			{
				if (Player_Volume_Set(playerHandle, 0))
				{
					printf("\n%s : ERROR Player_Volume_Set() fail\n", __FUNCTION__);
				}
			}
			printf("\nVolume changed to %d\n", volumeLevel);
        }
    }
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
    uint8_t tableId = *buffer; 
    /* check table id to identify table who has arrived and parse it */ 
    if(tableId==0x00)
    {
    	/* pat table arrived */
        if(parsePatTable(buffer,patTable)==TABLES_PARSE_OK)
        {
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
        }
    } 
    else if (tableId==0x02)
    {        
    	/* pmt table arrived */
        if(parsePmtTable(buffer,pmtTable)==TABLES_PARSE_OK)
        {
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
        }
    }
	else if (tableId==0x4e)
	{	
		/* eit table arrived */	
		if(parseEitTable(buffer,eitTable)==TABLES_PARSE_OK)
        {
			if ((pmtTable->pmtHeader).programNumber == (eitTable->eitHeader).serviceId && eitTable->eitInfoArray[0].runningStatus == 0x4)
			{	
				/* if arrived current event for current channel get event name and time */		
				getEvent();
			} 
        }

	}

    return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
	/* check tuner callback to check when tuner is locked to frequency */
    if(status == STATUS_LOCKED)
    {
    	/* notify that tuner is locked to frequency */
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n%s -----TUNER LOCKED-----\n",__FUNCTION__);
    }
    else
    {
        printf("\n%s -----TUNER NOT LOCKED-----\n",__FUNCTION__);
    }
    return 0;
}

/* parse time and name from eit table */
void getEvent()
{
	uint32_t time = eitTable->eitInfoArray[0].startTime;
	uint32_t mask = 0x0000f000; 
	uint16_t num;
	char parsed_time[MAX_EVENT_LEN]; 
	uint8_t i;
	uint8_t j;
	uint8_t k = 0;
	/* get time from time in 32 bit hex format */
	for (i = 0; i <  2; i++)
	{
		for (j = 0; j < 2; j++,k++)
		{
			/* get time digit */
			num = time & mask;
			/* shift mask at next position to get next digit */
			mask >>= 4;
			num >>= (12 - 4 * (i*2 + j));
			if (k == 2)
			{
				parsed_time[k] = ':';
				parsed_time[++k] = num + '0';
			}
			else
			{
				parsed_time[k] = num + '0';
			}
		}
	}
	parsed_time[5] = '\0';
	/* copy to event event channel */
	strncpy(currentChannel.eventTime,parsed_time,6);
	strncpy(currentChannel.eventName,eitTable->eitInfoArray[0].eventName,50);	
}







