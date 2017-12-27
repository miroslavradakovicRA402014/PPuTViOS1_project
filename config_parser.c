#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "stream_controller.h"
#include "config_parser.h"

static FILE* configOpen(char* configFile);
static ConfigErrorCode parseAttribute(char* tag, char* value, InitConfig* config);
static int32_t getAttributeValue(char* value);

ConfigErrorCode parseConfigFile(char* configFile, InitConfig* config)
{
	FILE* fp = configOpen(configFile);
	if (fp == NULL)
	{
		printf("\nConfig file parse failed!\n");	
		return CONFIG_PARSE_ERROR;
	}

	uint8_t i = 0;
	/* strings for attribute parse */
	char configFileTag[CONFIG_LINE_LEN];
	char configFileSeparator[CONFIG_LINE_LEN];	
	char configFileValue[CONFIG_LINE_LEN];
	/* read file line by line and parse attributes */
	while (!feof(fp))
	{
		/* parse attribute tag */
		if (i == 0)
		{
			fscanf(fp, "%s", configFileTag);
			if (strcmp(configFileTag,"\n") != 0)
			{
				i++;
			}
		}
		/* parse separator */
		else if (i == 1)
		{
			i++;
			fscanf(fp, "%s", configFileSeparator);		
		}
		/* parse attribute value */
		else if (i == 2)
		{
			i = 0;
			fscanf(fp, "%s", configFileValue);
			parseAttribute(configFileTag, configFileValue, config);			
		}
	}
	
	printf("\nFrequency :%d", config->configFreq);
	printf("\nBandwidht :%d", config->configBandwidth);
	printf("\nAudio PID :%d", config->configAudioPid);		
	printf("\nVideo PID :%d", config->configVideoPid);	
	printf("\nProgram number :%d", config->configProgramNumber);	
	printf("\nTV module :%d", config->configModule);
	printf("\nAudio type :%d", config->configAudioType);
	printf("\nVideo type :%d", config->configVideoType);

	fclose(fp);

	return CONFIG_PARSE_OK;
}

ConfigErrorCode parseAttribute(char* tag, char* value, InitConfig* config)
{
	/* match tag with attribute and write parsed atribute at config struct */
	
	if (!strcmp(tag,"FREQUENCY"))
	{
		config->configFreq = getAttributeValue(value);
	}
	
	if (!strcmp(tag,"BANDWIDTH"))
	{
		config->configBandwidth = getAttributeValue(value);
	}
	
	if (!strcmp(tag,"AUDIO_PID"))
	{
		config->configAudioPid = getAttributeValue(value);
	}
	
	if (!strcmp(tag,"VIDEO_PID"))
	{
		config->configVideoPid = getAttributeValue(value);
	}
	
	if (!strcmp(tag,"PROGRAM_NUMBER"))
	{
		config->configProgramNumber = getAttributeValue(value);
	}		
	
	if (!strcmp(tag,"TV_MODULE"))
	{
		if (!strcmp(value,"DVB_T2"))
		{
			config->configModule = DVB_T2;
		}
		else if (!strcmp(value,"DVB_T"))
		{
			config->configModule = DVB_T;
		}
		else
		{
			printf("\nModule %s doesn't exist!\n", value);	
			return CONFIG_PARSE_ERROR;
		}
	}	
  
	if (!strcmp(tag,"AUDIO_TYPE"))
	{
		if (!strcmp(value,"AUDIO_TYPE_MPEG_AUDIO"))
		{
			config->configAudioType = AUDIO_TYPE_MPEG_AUDIO;
		}
		else
		{
			printf("\nModule %s doesn't exist!\n", value);	
			return CONFIG_PARSE_ERROR;
		}	
	}  
  
	if (!strcmp(tag,"VIDEO_TYPE"))
	{ 		
	 	if (!strcmp(value,"VIDEO_TYPE_MPEG2"))
		{
			config->configVideoType = VIDEO_TYPE_MPEG2;
		}
		else if (!strcmp(value,"VIDEO_TYPE_MPEG4"))
		{
			config->configVideoType = VIDEO_TYPE_MPEG4;
		}	
		else if (!strcmp(value,"VIDEO_TYPE_MPEG1"))
		{
			config->configVideoType = VIDEO_TYPE_MPEG1;
		}	
		else if (!strcmp(value,"VIDEO_TYPE_JPEG"))
		{
			config->configVideoType = VIDEO_TYPE_JPEG;
		}				
		else
		{
			printf("\nModule %s doesn't exist!\n", value);	
			return CONFIG_PARSE_ERROR;
		}	
	}
	
	return CONFIG_PARSE_OK;		
}

/* convert string attribute at integer value */
int32_t getAttributeValue(char* value)
{
	int32_t attr;
	uint8_t n = 0;
	char val[CONFIG_VAL_LEN]; 
	
	while ((value[n] >= 48 && value[n] <= 57))
	{
		n++;
	}
	val[n] = '\0';

	strncpy(val, value, n);
	attr = atoi(val);
	
	return attr;
}

FILE* configOpen(char* configFile)
{
	/* open config file */
	FILE* fp = fopen(configFile, "r");
	
	if (fp == NULL)
	{
		printf("\nCan't open config file!\n");
		return NULL;
	}

	return fp;
}



