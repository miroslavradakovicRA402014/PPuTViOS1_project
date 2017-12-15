#include "remote_controller.h"
#include "stream_controller.h"
#include "graphic_controller.h"

#define ARG_NUM 2

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

/* macro function for error checking */
#define ERRORCHECK(x)                                                       \
{                                                                           \
if (x != 0)                                                                 \
 {                                                                          \
    textColor(1,1,0);                                                       \
    printf(" Error!\n File: %s \t Line: <%d>\n", __FILE__, __LINE__);       \
    textColor(0,7,0);                                                       \
    return -1;                                                              \
 }                                                                          \
}

static void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value);
static pthread_cond_t deinitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t deinitMutex = PTHREAD_MUTEX_INITIALIZER;


extern ChannelInfo currentChannel;
extern int16_t volumeLevel;

int main(int argc, char* argv[])
{
	/* read config file */
	if (argc != ARG_NUM)
	{
		printf("\nERROR Don't enough input arguments!\n");
		return 0;
	}

    /* initialize remote controller module */
    ERRORCHECK(remoteControllerInit());
    
    /* register remote controller callback */
    ERRORCHECK(registerRemoteControllerCallback(remoteControllerCallback));
    
    /* initialize stream controller module */
    ERRORCHECK(streamControllerInit(argv[1]));

	/* initialize graphic controller module */
    ERRORCHECK(graphicControllerInit());

    /* wait for a EXIT remote controller key press event */
    pthread_mutex_lock(&deinitMutex);
	if (ETIMEDOUT == pthread_cond_wait(&deinitCond, &deinitMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&deinitMutex);

	/* deinitialize graphic controller module */
    ERRORCHECK(graphicControllerDeinit());
    
    /* unregister remote controller callback */
    ERRORCHECK(unregisterRemoteControllerCallback(remoteControllerCallback));

    /* deinitialize remote controller module */
    ERRORCHECK(remoteControllerDeinit());

    /* deinitialize stream controller module */
    ERRORCHECK(streamControllerDeinit());
  
    return 0;
}

void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value)
{
	//printf("Callback with code %d \n", code);
    switch(code)
	{
		case KEYCODE_INFO:
            printf("\nInfo pressed\n");  
		        
            if (getChannelInfo(&currentChannel) == SC_NO_ERROR)
            {
                printf("\n********************* Channel info *********************\n");
                printf("Program number: %d\n", currentChannel.programNumber);
                printf("Audio pid: %d\n", currentChannel.audioPid);
                printf("Video pid: %d\n", currentChannel.videoPid);
                printf("**********************************************************\n");
            }
			break;
		case KEYCODE_P_PLUS:
			printf("\nCH+ pressed\n");
            channelUp();
			drawCnannel(currentChannel.programNumber);
			break;
		case KEYCODE_P_MINUS:
		    printf("\nCH- pressed\n");
            channelDown();
			drawCnannel(currentChannel.programNumber);
			break;
		case KEYCODE_V_PLUS:
			printf("\nVOL+ pressed\n");
            volumeUp();
			drawVolumeLevel(volumeLevel);
			break;
		case KEYCODE_V_MINUS:
			printf("\nVOL- pressed\n");
            volumeDown();
			drawVolumeLevel(volumeLevel);
			break;
		case KEYCODE_MUTE:
			printf("\nMUTE pressed\n");
			mute();
			drawVolumeLevel(0);
			break;
		case KEYCODE_EXIT:
			printf("\nExit pressed\n");
            pthread_mutex_lock(&deinitMutex);
		    pthread_cond_signal(&deinitCond);
		    pthread_mutex_unlock(&deinitMutex);
			break;
		default:
			if (code >= KEYCODE_NUMBER_1 && code <= KEYCODE_NUMBER_0)
			{
				if (code != KEYCODE_NUMBER_0)
				{
					printf("\nNumber %d pressed\n", code - 1);
					channelSwitch(code - 1);
					drawCnannel(currentChannel.programNumber);
				}
				else
				{
					printf("\nNumber 0 pressed\n");
					channelSwitch(0);
					drawCnannel(currentChannel.programNumber);
				}
			}
			else
			{
				printf("\nPress P+, P-, info or exit! \n\n");
			}
	}
}
