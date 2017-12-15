#include "graphic_controller.h"

static timer_t timerId;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;
static bool isInitialized = false;
static uint8_t threadExit = 0;
static ScreenState state = {false, false, false, false, false};
static int32_t programNumber = 0;


static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static pthread_t gcThread;

static void* graphicControllerTask();
static void wipeScreen(union sigval signalArg);
static void drawKeycode(int32_t keycode);


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
	programNumber = channelNumber;
	state.drawChannel = true;
}


static void* graphicControllerTask()
{
	DFBSurfaceDescription surfaceDesc;
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
			drawKeycode(programNumber);
			state.drawChannel = false;
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

void drawKeycode(int32_t keycode){
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
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}

void wipeScreen(union sigval signalArg){
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
