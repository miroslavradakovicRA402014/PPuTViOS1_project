#include "graphic_controller.h"

static timer_t timerId;
static IDirectFBSurface *primary = NULL;
static DFBSurfaceDescription surfaceDesc;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;
static bool isInitialized = false;
static uint8_t threadExit = 0;
static ScreenState state = {false, false, false, false};

static int32_t programNumberRender = 0;
static int32_t volumeLevelRender = 0;
static int32_t audioPidRender = 0;
static int32_t videoPidRender = 0;

static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static pthread_t gcThread;

static void* graphicControllerTask();
static void wipeScreen(union sigval signalArg);
static void drawProgram(int32_t keycode);
static void drawVolumeSymbol(int32_t volumeLevel);
static void drawBanner(int32_t audioPid, int32_t videoPid);


GraphicControllerError graphicControllerInit()
{
    if (pthread_create(&gcThread, NULL, &graphicControllerTask, NULL))
    {
        printf("Error creating input event task!\n");
        return GC_THREAD_ERROR;
    }

    return GC_NO_ERROR;
}

GraphicControllerError drawCnannel(int32_t channelNumber)
{
	programNumberRender = channelNumber;
	state.drawChannel = true;
	state.drawInfo = true;	
}

GraphicControllerError drawVolumeLevel(int32_t volumeLevel)
{
	volumeLevelRender = volumeLevel;
	state.drawVolumeChange = true;
}

GraphicControllerError drawInfoBanner(int32_t audioPid, int32_t videoPid)
{
	audioPidRender = audioPid;
	videoPidRender = videoPid;
	state.drawInfo = true;	
}

static void* graphicControllerTask()
{
	int32_t ret;

	/* structure for timer specification */
    struct sigevent signalEvent;

    /* initialize DirectFB */    
	DFBCHECK(DirectFBInit(NULL, NULL));
	DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	
    /* create primary surface with double buffering enabled */    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    /* fetch the screen size */
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));


	signalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEvent.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
    signalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
    signalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEvent,
                       /*where to store the ID of the newly created timer*/&timerId);
    if(ret == -1)
	{
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return 0;
    }

    /* set isInitialized flag */
    isInitialized = true;

	while (!threadExit)
	{	
		if (state.drawChannel)
		{
			drawProgram(programNumberRender);
			state.drawChannel = false;
		}

		if (state.drawVolumeChange)
		{
			drawVolumeSymbol(volumeLevelRender);
			state.drawVolumeChange = false;
		}

		if (state.drawInfo)
		{
			drawBanner(audioPidRender,videoPidRender);
			state.drawInfo = false;
		}
	}

}

GraphicControllerError graphicControllerDeinit()
{
    if (!isInitialized) 
    {
        printf("\n%s : ERROR graphicControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
        return GC_ERROR;
    }
    
    threadExit = 1;
    if (pthread_join(gcThread, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return GC_THREAD_ERROR;
    }

	/* clean up */
	timer_delete(timerId);
	primary->Release(primary);
	dfbInterface->Release(dfbInterface);

	/* set isInitialized flag */
    isInitialized = false;

    return GC_NO_ERROR;
}

void drawProgram(int32_t keycode)
{
    int32_t ret;
    IDirectFBFont *fontInterface = NULL;
    DFBFontDescription fontDesc;
    char keycodeString[4];
        
    /*  draw the frame */
    
    DFBCHECK(primary->SetColor(primary, 0x40, 0x10, 0x80, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 50, 50, screenWidth/8, screenHeight/8));
    
    DFBCHECK(primary->SetColor(primary, 0x80, 0x40, 0x10, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 50+FRAME_THICKNESS, 50+FRAME_THICKNESS, screenWidth/8-2*FRAME_THICKNESS, screenHeight/8-2*FRAME_THICKNESS));
    
    
    /* draw keycode */
    
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT_CHANNEL;
	
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
    
    /* generate keycode string */
    sprintf(keycodeString,"%d",keycode);
    
    /* draw the string */
    DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x40, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, 50+screenWidth/16, 50+screenHeight/16 + FONT_HEIGHT_CHANNEL/2, DSTF_CENTER));
    
    
    /* update screen */
    DFBCHECK(primary->Flip(primary, NULL, 0));
    
    
    /* set the timer for clearing the screen */
    
    memset(&timerSpec,0,sizeof(timerSpec));
    
    /* specify the timer timeout time */
    timerSpec.it_value.tv_sec = 3;
    timerSpec.it_value.tv_nsec = 0;
    
    /* set the new timer specs */
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
	{
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}

void drawVolumeSymbol(int32_t volumeLevel)
{
    int32_t ret;
	IDirectFBImageProvider *provider;
	IDirectFBSurface *volumeSurface = NULL;
	int32_t volumeHeight, volumeWidth;
	
    /* create the image provider for the specified file */
	switch (volumeLevel)
	{
		case 0:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_0.png", &provider));	
			break;
		case 1:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_1.png", &provider));	
			break;
		case 2:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_2.png", &provider));	
			break;
		case 3:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_3.png", &provider));	
			break;
		case 4:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_4.png", &provider));	
			break;
		case 5:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_5.png", &provider));	
			break;
		case 6:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_6.png", &provider));	
			break;
		case 7:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_7.png", &provider));	
			break;
		case 8:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_8.png", &provider));	
			break;
		case 9:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_9.png", &provider));	
			break;
		case 10:
			DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_10.png", &provider));	
			break;
	}

    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &volumeSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, volumeSurface, NULL));
	
    /* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);
	
    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(volumeSurface->GetSize(volumeSurface, &volumeWidth, &volumeHeight));
	DFBCHECK(primary->Blit(primary,
                           /*source surface*/ volumeSurface,
                           /*source region, NULL to blit the whole surface*/ NULL,
                           /*destination x coordinate of the upper left corner of the image*/50,
                           /*destination y coordinate of the upper left corner of the image*/screenHeight - volumeHeight -50));
    
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));
    
    
    /* set the timer for clearing the screen */
    
    memset(&timerSpec,0,sizeof(timerSpec));
    
    /* specify the timer timeout time */
    timerSpec.it_value.tv_sec = 3;
    timerSpec.it_value.tv_nsec = 0;
    
    /* set the new timer specs */
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
	{
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}


void drawBanner(int32_t audioPid, int32_t videoPid)
{
    int32_t ret;

    IDirectFBFont *fontInterface = NULL;
    DFBFontDescription fontDesc;
	char audioInfo[20];     
	char videoInfo[20];
	char audioPidStr[5];
	char videoPidStr[5];

	strcpy(audioInfo, "Audio PID: ");
	strcpy(videoInfo, "Video PID: ");

    /*  draw the frame */
        
    DFBCHECK(primary->SetColor(primary, 0x80, 0x40, 0x10, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 50, (screenHeight/3)*2, screenWidth-100, screenHeight/4));
      
    /* draw info */
    
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT_CHANNEL;
	
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));


	/* generate audioPid string */
    sprintf(audioPidStr,"%d",audioPid);
	strcat(audioInfo, audioPidStr);
	/* generate videoPid string */
    sprintf(videoPidStr,"%d",videoPid);
	strcat(videoInfo, videoPidStr);
    
    /* draw the string */

    DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x40, 0xff));
	DFBCHECK(primary->DrawString(primary, audioInfo, -1, 50 + (screenWidth/2) - 50, (screenHeight/3)*2 + FONT_HEIGHT_CHANNEL, DSTF_CENTER));
	DFBCHECK(primary->DrawString(primary, videoInfo, -1, 50 + (screenWidth/2) - 50, (screenHeight/3)*2 + FONT_HEIGHT_CHANNEL*3, DSTF_CENTER));    
    
    /* update screen */
    DFBCHECK(primary->Flip(primary, NULL, 0));
    
    
    /* set the timer for clearing the screen */
    
    memset(&timerSpec,0,sizeof(timerSpec));
    
    /* specify the timer timeout time */
    timerSpec.it_value.tv_sec = 3;
    timerSpec.it_value.tv_nsec = 0;
    
    /* set the new timer specs */
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
	{
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}



void wipeScreen(union sigval signalArg)
{
    int32_t ret;

    /* clear screen */
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
    /* update screen */
    DFBCHECK(primary->Flip(primary, NULL, 0));
    
    /* stop the timer */
    memset(&timerSpec,0,sizeof(timerSpec));
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
	{
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}
