/*
 * config.h
 *
 *  Created on: 10.07.2015
 *      Author: Internship
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* Frequency of RTC (COMP0) pulses on PRS channel 2. */
#define RTC_PULSE_FREQUENCY    (LS013B7DH03_POLARITY_INVERSION_FREQUENCY)

#define GLIB_FONT_WIDTH   (glibContext.font.fontWidth + glibContext.font.charSpacing)
#define GLIB_FONT_HEIGHT  (glibContext.font.fontHeight)

/* Center of display */
#define CENTER_X (glibContext.pDisplayGeometry->xSize / 2)
#define CENTER_Y (glibContext.pDisplayGeometry->ySize / 2)

#define MAX_X (glibContext.pDisplayGeometry->xSize - 1)
#define MAX_Y (glibContext.pDisplayGeometry->ySize - 1)

#define MIN_X           0
#define MIN_Y           0

#define MIN_TEMP 0
#define MAX_TEMP 50
#define HISTEREZA 1
#define THERM_X 10
#define THERM_Y 100

/* The GLIB context */
static GLIB_Context_t   glibContext;

/* The current time reference. Number of seconds since midnight*/
static time_t curTime = 0;


unsigned char setPoint =(MIN_TEMP + MAX_TEMP)/3;
int ctemp=0;
int BSP_LedSet(uint32_t leds);
int BSP_LedsInit(void);
int BSP_LedClear(int ledNo);
EMSTATUS GLIB_drawCircleFilled(GLIB_Context_t *pContext, int32_t xCenter, int32_t yCenter, uint32_t radius);
EMSTATUS GLIB_drawRect(GLIB_Context_t *pContext, GLIB_Rectangle_t *pRect);
EMSTATUS GLIB_drawRectFilled(GLIB_Context_t *pContext, GLIB_Rectangle_t *pRect);

#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
/* sniprintf does not process floats, but occupy less falsh memory ! */
#define snprintf sniprintf
#endif

/* Forward static function declararations */

static void GlibDrawTemp(GLIB_Context_t *pContext);

//graph settings
#define MAX_POINTS 20
struct graphPoints{
	short x,y;
}p[MAX_POINTS];	//memorizes the points in the graph
unsigned char k=0;
#define GRAPH_X 25
#define GRAPH_Y 105
#define GRAPH_HEIGHT 60
#define GRAPH_WIDTH 100
void drawGraph();
int past_x=GRAPH_X;
int	past_y=GRAPH_Y;

#endif /* CONFIG_H_ */
