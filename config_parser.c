#include <string.h>
#include "stream_controller.h"
#include "config_parser.h"

ConfigErrorCode parseConfigFile(char* configFile, InitConfig* config)
{
	FILE* fp = configOpen(configFile);
	if (fp == NULL)
	{
		printf("\nConfig file parse failed!\n");	
		return CONFIG_PARSE_ERROR;
	}

	char configModule[CONFIG_NAME_LEN];
	char configAudioType[CONFIG_NAME_LEN];
	char configVideoType[CONFIG_NAME_LEN];	

	fscanf(fp, "%d %d %d %d %d %s %s %s", 
				&(config->configFreq), 
				   &(config->configBandwidth), 	
				      &(config->configAudioPid), 
					     &(config->configVideoPid), 
					        &(config->configProgramNumber),
							   configModule,	 
							      configAudioType, 
								   	 configVideoType);


	if (!strcmp(configModule,"DVB_T2") || !strcmp(configModule,"dvb_t2"))
	{
		config->configModule = DVB_T2;
	}
	else if (!strcmp(configModule,"DVB_T") || !strcmp(configModule,"dvb_t"))
	{
		config->configModule = DVB_T;
	}
	else
	{
		printf("\nModule %s doesn't exist!\n", configModule);	
		return CONFIG_PARSE_ERROR;
	}

	if (!strcmp(configAudioType,"AUDIO_TYPE_MPEG_AUDIO") || !strcmp(configAudioType,"audio_type_mpeg_audio"))
	{
		config->configAudioType = AUDIO_TYPE_MPEG_AUDIO;
	}
	else
	{
		printf("\nModule %s doesn't exist!\n", configAudioType);	
		return CONFIG_PARSE_ERROR;
	}

	if (!strcmp(configVideoType,"VIDEO_TYPE_MPEG2") || !strcmp(configVideoType,"video_type_mpeg2"))
	{
		config->configVideoType = VIDEO_TYPE_MPEG2;
	}
	else
	{
		printf("\nModule %s doesn't exist!\n", configVideoType);	
		return CONFIG_PARSE_ERROR;
	}
	
	printf("\nFrequency :%d", config->configFreq);
	printf("\nBandwidht :%d", config->configBandwidth);
	printf("\nAudio PID :%d", config->configAudioPid);		
	printf("\nVideo PID :%d", config->configVideoPid);	
	printf("\nProgram number :%d", config->configProgramNumber);	
	printf("\nTV module :%s", configModule);
	printf("\nAudio type :%s", configAudioType);
	printf("\nVideo type :%s", configVideoType);

	fclose(fp);

	return CONFIG_PARSE_OK;
}


FILE* configOpen(char* configFile)
{
	FILE* fp = fopen(configFile, "r");
	
	if (fp == NULL)
	{
		printf("\nCan't open config file!\n");
		return NULL;
	}

	return fp;
}



