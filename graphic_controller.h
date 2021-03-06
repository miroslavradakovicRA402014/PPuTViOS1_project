#ifndef __GRAPHIC_CONTROLLER_H__
#define __GRAPHIC_CONTROLLER_H__

#include <stdio.h>
#include <directfb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include "pthread.h"
#include <stdbool.h>

#define FRAME_THICKNESS 5
#define FONT_HEIGHT_CHANNEL 50

/* helper macro for error checking */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}


/**
 * @brief Structure that defines graphic controller error
 */
typedef enum _GraphicControllerError
{
    GC_NO_ERROR = 0,
    GC_ERROR,
    GC_THREAD_ERROR
}GraphicControllerError;


/**
 * @brief Structure that defines current screen state
 */
typedef struct _ScreenState
{
	bool drawChannel;
	bool drawVolumeChange; 
	bool drawInfo;
	bool drawBlackScreen;
	bool refreshScreen;
}ScreenState;


/**
 * @brief Initializes graphic controller module
 *	
 * @return graphic controller error code
 */
GraphicControllerError graphicControllerInit();

/**
 * @brief Deinitializes graphic controller module
 *
 * @return graphic controller error code
 */
GraphicControllerError graphicControllerDeinit();
/**
 * @brief Draw cnannel number
 *
 * @return graphic controller error code
 */
GraphicControllerError drawCnannel(int32_t channelNumber);
/**
 * @brief Draw volume level symbol
 *
 * @return graphic controller error code
 */
GraphicControllerError drawVolumeLevel(int32_t volumeLevel);
/**
 * @brief Draw info banner
 *
 * @return graphic controller error code
 */
GraphicControllerError drawInfoBanner(int32_t channelNumber, int32_t audioPid, int32_t videoPid, bool teletext, char* time, char* name);


#endif /* __GRAPHIC_CONTROLLER_H__ */
