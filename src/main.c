

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_pcnt.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "display.h"
#include "dmd.h"
#include "glib.h"
#include "bspconfig.h"
#include "em_adc.h"
#include<stdlib.h>
#include"config.h"
#include"si7013.h"
#include<i2cspm.h>
#include "capsense.h"



/**************************************************************************//**
 * @brief Setup GPIO interrupt for pushbuttons.
 *****************************************************************************/
static void GpioSetup(void)
{
  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure PB0 as input and enable interrupt  */
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);

  /* Configure PB1 as input and enable interrupt */
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}


static void AdcSetup(void)
{
  /* Enable ADC clock */
  CMU_ClockEnable( cmuClock_ADC0, true);

  /* Base the ADC configuration on the default setup. */
  ADC_Init_TypeDef       init  = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef sInit = ADC_INITSINGLE_DEFAULT;

  /* Initialize timebases */
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(400000, 0);
  ADC_Init(ADC0, &init);

  /* Set input to temperature sensor. Reference must be 1.25V */
  sInit.reference = adcRef1V25;
  sInit.input     = adcSingleInpTemp;
  ADC_InitSingle(ADC0, &sInit);
}

/**************************************************************************//**
 * @brief  Do one ADC conversion
 * @return ADC conversion result
 *****************************************************************************/
static uint32_t AdcRead( void )
{
  ADC_Start(ADC0, adcStartSingle);
  while ( ( ADC0->STATUS & ADC_STATUS_SINGLEDV ) == 0 ){}
  return ADC_DataSingleGet(ADC0);
}
static float32_t ConvertToCelsius(int32_t adcSample)
{
  float32_t temp;
  /* Factory calibration temperature from device information page. */
  int32_t cal_temp_0 = ((DEVINFO->CAL & _DEVINFO_CAL_TEMP_MASK)
                             >> _DEVINFO_CAL_TEMP_SHIFT);
  /* Factory calibration value from device information page. */
  int32_t cal_value_0 = ((DEVINFO->ADC0CAL2 & _DEVINFO_ADC0CAL2_TEMP1V25_MASK)
                          >> _DEVINFO_ADC0CAL2_TEMP1V25_SHIFT);

  /* Temperature gradient (from datasheet) */
  float32_t t_grad = -6.27;
  temp = (cal_temp_0 - ((cal_value_0 - adcSample) / t_grad));
  return temp;
}

/**************************************************************************//**
* @brief Unified GPIO Interrupt handler (pushbuttons)
*        PB0 Reset to test zero
*        PB1 Next test
*****************************************************************************/
void GPIO_Unified_IRQ(void)
{
  /* Get and clear all pending GPIO interrupts */
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  /* Act on interrupts */
  if (interruptMask & (1 << BSP_GPIO_PB0_PIN))
  {
    /* PB0:  */
	  if(setPoint<MAX_TEMP)
		  setPoint ++;
  }

  if (interruptMask & (1 << BSP_GPIO_PB1_PIN))
  {
    /* PB1:  */
	  if(setPoint>MIN_TEMP)
		  setPoint--;
  }
}

/**************************************************************************//**
 * @brief GPIO Interrupt handler (PB0)
 *
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}


/**************************************************************************//**
 * @brief GPIO Interrupt handler (PB1)
 *
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}


/**************************************************************************//**
 * @brief   Set up PCNT to generate an interrupt every second.
 *
 *****************************************************************************/
void PcntInit(void)
{
  PCNT_Init_TypeDef pcntInit = PCNT_INIT_DEFAULT;

  /* Enable PCNT clock */
  CMU_ClockEnable(cmuClock_PCNT0, true);
  /* Set up the PCNT to count RTC_PULSE_FREQUENCY pulses -> one second */
  pcntInit.mode = pcntModeOvsSingle;
  pcntInit.top = RTC_PULSE_FREQUENCY / RTC_PULSE_FREQUENCY;
  pcntInit.s1CntDir = false;
  /* The PRS channel used depends on the configuration and which pin the
  LCD inversion toggle is connected to. So use the generic define here. */
  pcntInit.s0PRS = (PCNT_PRSSel_TypeDef)LCD_AUTO_TOGGLE_PRS_CH;

  PCNT_Init(PCNT0, &pcntInit);

  /* Select PRS as the input for the PCNT */
  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);

  /* Enable PCNT interrupt every second */
  NVIC_EnableIRQ(PCNT0_IRQn);
  PCNT_IntEnable(PCNT0, PCNT_IF_OF);
}

/**************************************************************************//**
 * @brief   This interrupt is triggered at every second by the PCNT
 *
 *****************************************************************************/
void PCNT0_IRQHandler(void)
{
  PCNT_IntClear(PCNT0, PCNT_IF_OF);
  if(curTime%40==0){
	  ctemp = (int)(ConvertToCelsius( AdcRead() ) * 10); //gets the temperature from sensor every 40 cycles
	  if(k!=0){
	  p[k].x=GRAPH_X+k*(GRAPH_WIDTH/MAX_POINTS);
	  p[k].y=GRAPH_Y+GRAPH_HEIGHT/2-(int)ctemp/5;//determine a point in the graph based on current temperature
	  }
	  else{
		  p[k].x=GRAPH_X;
		  	  p[k].y=GRAPH_Y-GRAPH_HEIGHT/2;
	  }
	  if(k==MAX_POINTS){	//if the max points are reached, i delete the first point from the vector(queue)
		 // k=-1;
		  int i=0;

		  for(i=0;i<MAX_POINTS;i++)
		  {  p[i]=p[i+1];
		  	  p[i].x=p[i].x-(GRAPH_WIDTH/MAX_POINTS);
		  }

		  k--;
	  }
	  k++;
  	    }
  curTime++;
}


static void GlibDrawTemp(GLIB_Context_t *pContext)
{
  #define STR_LEN 48
  char str[ STR_LEN ];

/* Print welcome message using NORMAL 8x8 font. */
  GLIB_setFont(pContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);



        /* Output measurement on display. */
  uint32_t         rhData=-1;
    int32_t          tData;
#define SI7013_ADDR      0x82
    (void)Si7013_MeasureRHAndTemp(I2C0, SI7013_ADDR, &rhData, &tData); //provides the humidity in the rhdata, tdata is ignored
  snprintf( str, STR_LEN, "CTemp: %2d.%1d C \nSetPoint: %d C \nHumidity:%d%%", (ctemp / 10), abs(ctemp % 10),setPoint,(int)rhData/1000);

  GLIB_drawString(&glibContext,
                  str,
                  strlen(str),
                  MIN_X,
                  5,
                  0);

  #undef STR_LEN
}
static void GRAPHICS_CreateString(char *string, int32_t value)
{
  if (value < 0)
  {
    value = -value;
    string[0] = '-';
  }
  else
  {
    string[0] = ' ';
  }
  string[5] = 0;
  string[4] = '0' + (value % 1000) / 100;
  string[3] = '.';
  string[2] = '0' + (value % 10000) / 1000;
  string[1] = '0' + (value % 100000) / 10000;

  if (string[1] == '0')
  {
    string[1] = ' ';
  }
}
static void GRAPHICS_DrawThermometerC(int xoffset, int yoffset, int32_t tempData)
{
  char string[10],max_temp[5],min_temp[5];
  int  dx;
  int  dy;
  int  height;

  dy = yoffset * 8;
  dx = xoffset * 8;

  /* Text in display */
  //GRAPHICS_CreateString(string, tempData);
  //GLIB_drawString(&glibContext, string, strlen(string), THERM_X-56 - dx, THERM_Y -51 - dy, 0);
  tempData /= 1000;

  if(tempData>MAX_TEMP)//make sure the bar doesn't go beyond the max point
	  tempData=MAX_TEMP;
  height =THERM_Y-tempData;

  /* Line in thermometer */
  GLIB_Rectangle_t rectf;
  rectf.xMax=THERM_X+4 - dx;
  rectf.xMin=THERM_X - dx;
  rectf.yMax=THERM_Y;
  rectf.yMin=height;
  GLIB_drawRectFilled(&glibContext, &rectf);

  GLIB_drawCircle(&glibContext, THERM_X+2,THERM_Y+5, 8);
  GLIB_drawCircleFilled(&glibContext, THERM_X+2,THERM_Y+5, 5);
  GLIB_Rectangle_t rect;
  rect.xMax=THERM_X+6;
  rect.xMin=THERM_X-2;
  rect.yMax=THERM_Y-1;
  rect.yMin=THERM_Y-52;
  GLIB_drawRect(&glibContext, &rect);//drawn without the bottom line

  /* Marks on the thermometer */
  snprintf(max_temp, 5, "%d", MAX_TEMP);
  snprintf(min_temp, 5, "%d", MIN_TEMP);
  //GLIB_drawString(&glibContext, min_temp, 1, THERM_X-14 - dx, THERM_Y-5 - dy, 0);
  //GLIB_drawString(&glibContext, max_temp, 2, THERM_X-22 - dx, THERM_Y-76 - dy, 0);
 // GLIB_drawString(&glibContext, "C", 1, THERM_X+18 - dx, THERM_Y-49 - dy, 0);
}
static void updateDisplay(void){
	GLIB_clear(&glibContext);
	GlibDrawTemp(&glibContext);//text information
	GRAPHICS_DrawThermometerC(0, -2,ctemp*100 );//graphic thermometer
	drawGraph();//graph
	DMD_updateDisplay();
}
void drawGraph(){
	GLIB_drawLineH(&glibContext,GRAPH_X,GRAPH_Y,GRAPH_X+GRAPH_WIDTH );
	GLIB_drawLineV(&glibContext,GRAPH_X,GRAPH_Y,GRAPH_Y-GRAPH_HEIGHT);
	int i=0;
//	static int ok=0;
//	static int past_x,past_y;
//	if(ok==0){
//		ok=1;
	past_x=GRAPH_X;
	past_y=GRAPH_Y;
//	}
	for(i=0;i<k;i++){
		GLIB_drawLine(&glibContext,past_x,past_y,p[i].x,p[i].y);//draws a line from the past point to the current point in the graph
		past_x=p[i].x;
		past_y=p[i].y;
	}
	char max_temp[5],min_temp[5];
	snprintf(max_temp, 5, "%d", MAX_TEMP);
	snprintf(min_temp, 5, "%d", MIN_TEMP);
	GLIB_drawString(&glibContext, min_temp, 2, GRAPH_X , GRAPH_Y+10, 0);//minimum temperature mark
	GLIB_drawString(&glibContext, max_temp, 2, GRAPH_X, GRAPH_Y-GRAPH_HEIGHT-5, 0);//maximum temperature mark


}
void manageLedOutput() {
	if (ctemp / 10 >= setPoint+HISTEREZA) {
		BSP_LedSet(0);
		BSP_LedSet(1);
	}
	if (ctemp / 10 < setPoint - HISTEREZA) {
		BSP_LedClear(0);
		BSP_LedClear(1);
	}
}

void manageTouchControlls() {	//AFTER A FEW MINUTES IT GOES CRAZY for some UNKNOWN reason
	//does the exact same thing as the hardware buttons
	CAPSENSE_Init();
	CAPSENSE_Sense();
	if (CAPSENSE_getPressed(BUTTON1_CHANNEL)
			&& !CAPSENSE_getPressed(BUTTON0_CHANNEL)) {
		if(setPoint<MAX_TEMP)
			setPoint++;
	} else if (CAPSENSE_getPressed(BUTTON0_CHANNEL)
			&& !CAPSENSE_getPressed(BUTTON1_CHANNEL)) {
		if (setPoint > MIN_TEMP)
			setPoint--;
	}
}

int main(void)
{

	 AdcSetup();
  EMSTATUS status;

  /* Chip errata */
  CHIP_Init();

  /* Setup GPIO for pushbuttons. */
  GpioSetup();

  /* Initialize LED driver */
    BSP_LedsInit();

  /* Initialize the display module. */
  status = DISPLAY_Init();
  if (DISPLAY_EMSTATUS_OK != status)
    while(1);

  /* Initialize the DMD module for the DISPLAY device driver. */
  status = DMD_init(0);
  if (DMD_OK != status)
    while(1);

  status = GLIB_contextInit(&glibContext);
  if (GLIB_OK != status)
    while(1);

  /* Set PCNT to generate interrupt every second */
  PcntInit();

  /* Initialize I2C driver, using standard rate. */
  	I2CSPM_Init_TypeDef i2cInit = I2CSPM_INIT_DEFAULT;
  	  I2CSPM_Init(&i2cInit);
  	  bool             si7013_status;	//humidity sensor
  	  si7013_status = Si7013_Detect(I2C0, SI7013_ADDR, NULL);



  /* Enter infinite loop */
  while (1)
  {

	  manageTouchControlls(); //has bugs, works for some time(around 5 minutes), after that it needs a reset
	  manageLedOutput();
	  updateDisplay();
  }
}

