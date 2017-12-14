#ifndef __STREAM_CONTROLLER_H__
#define __STREAM_CONTROLLER_H__

#include <stdio.h>
#include "tables.h"
#include "tdp_api.h"
#include "pthread.h"
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#define DESIRED_FREQUENCY 754000000	        /* Tune frequency in Hz */
#define BANDWIDTH 8    				        /* Bandwidth in Mhz */

#define CONFIG_NAME_LEN 20

#define MAX_VOL_LEVEL 10

/**
 * @brief Structure that defines stream controller error
 */
typedef enum _StreamControllerError
{
    SC_NO_ERROR = 0,
    SC_ERROR,
    SC_THREAD_ERROR
}StreamControllerError;

/**
 * @brief Structure that defines channel info
 */
typedef struct _ChannelInfo
{
    int16_t programNumber;
    int16_t audioPid;
    int16_t videoPid;
}ChannelInfo;

/**
 * @brief Structure that defines initial config
 */
typedef struct _InitConfig 
{
	int32_t configFreq;
	int16_t configBandwidth;	
    int16_t configAudioPid;
    int16_t configVideoPid;	
	int16_t configProgramNumber;
	t_Module configModule;
    tStreamType configAudioType;
	tStreamType configVideoType;	
}InitConfig;

/**
 * @brief Initializes stream controller module
 *	
 * @param [in] configFile - config file path 
 * @return stream controller error code
 */
StreamControllerError streamControllerInit(char* configFile);

/**
 * @brief Deinitializes stream controller module
 *
 * @return stream controller error code
 */
StreamControllerError streamControllerDeinit();

/**
 * @brief Channel up
 *
 * @return stream controller error
 */
StreamControllerError channelUp();

/**
 * @brief Channel down
 *
 * @return stream controller error
 */
StreamControllerError channelDown();

/**
 * @brief Channel switch to channel number
 *
 * @return stream controller error
 */
StreamControllerError channelSwitch(int16_t ch);

/**
 * @brief Volume up
 *
 * @return stream controller error
 */
StreamControllerError volumeUp();

/**
 * @brief Volume down
 *
 * @return stream controller error
 */
StreamControllerError volumeDown();

/**
 * @brief Volume mute
 *
 * @return stream controller error
 */
StreamControllerError mute();

/**
 * @brief Returns current channel info
 *
 * @param [out] channelInfo - channel info structure with current channel info
 * @return stream controller error code
 */
StreamControllerError getChannelInfo(ChannelInfo* channelInfo);

#endif /* __STREAM_CONTROLLER_H__ */

