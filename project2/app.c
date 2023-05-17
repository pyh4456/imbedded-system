/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                       IAR Development Kits
*                                              on the
*
*                                    STM32F429II-SK KICKSTART KIT
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : YS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <includes.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_TASK_EQ_0_ITERATION_NBR              16u
/*
*********************************************************************************************************
*                                            TYPES DEFINITIONS
*********************************************************************************************************
*/

typedef enum {
   TASK_BUTTON,
   TASK_LED,
   TASK_USART,

   TASK_N
}task_e;


typedef struct
{
   CPU_CHAR* name;
   OS_TASK_PTR func;
   OS_PRIO prio;
   CPU_STK* pStack;
   OS_TCB* pTcb;
}task_t;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  AppTaskStart          (void     *p_arg);
static  void  AppTaskCreate         (void);
static  void  AppObjCreate          (void);

static void AppTask_Button(void *p_arg);		//task determine if button is pressed
static void AppTask_LED(void *p_arg);			//control LEDs
static void AppTask_USART(void *p_arg);			//print string when button is pressed

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
/* ----------------- APPLICATION GLOBALS -------------- */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB   Task_Button_TCB;
static  CPU_STK  Task_Button_Stack[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       Task_LED_TCB;

static  OS_TCB       Task_USART_TCB;

static  CPU_STK  Task_LED_Stack[APP_CFG_TASK_START_STK_SIZE];

static  CPU_STK  Task_USART_Stack[APP_CFG_TASK_START_STK_SIZE];

task_t cyclic_tasks[TASK_N] = {
   {"Task_BUTTON" , AppTask_Button,  0, &Task_Button_Stack[0] , &Task_Button_TCB},
   {"Task_LED" , AppTask_LED,  1, &Task_LED_Stack[0] , &Task_LED_TCB},
   {"Task_USART", AppTask_USART, 2, &Task_USART_Stack[0], &Task_USART_TCB},
};

unsigned  short button_pressed = 0;		// 0: button isn't pressed, 1: button is pressed

OS_SEM MySem;							// Semaphore

/* ------------ FLOATING POINT TEST TASK -------------- */
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    /* Basic Init */
    RCC_DeInit();
//    SystemCoreClockUpdate();

    /* BSP Init */
    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    CPU_Init();                                                 /* Initialize the uC/CPU Services                       */
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */


    /* OS Init */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    //create semaphore
    OSSemCreate(&MySem,
             "My Semaphore",
            1,
            &err);

    OSTaskCreate((OS_TCB       *)&AppTaskStartTCB,              /* Create the start task                                */
                 (CPU_CHAR     *)"App Task Start",
                 (OS_TASK_PTR   )AppTaskStart,
                 (void         *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK      *)&AppTaskStartStk[0u],
                 (CPU_STK_SIZE  )AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void         *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);

   OSStart(&err);   /* Start multitasking (i.e. give control to uC/OS-III). */

   (void)&err;

   return (0u);
}
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/
static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;

   (void)p_arg;

    BSP_Init();                                                 /* Initialize BSP functions                             */
    CPU_Init();
    BSP_Tick_Init();                                            /* Initialize Tick Services.                            */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

   APP_TRACE_DBG(("Creating Application Kernel Objects\n\r"));
   AppObjCreate();                                             /* Create Applicaiton kernel objects                    */

   APP_TRACE_DBG(("Creating Application Tasks\n\r"));
   AppTaskCreate();                                            /* Create Application tasks                             */
}

/*
*********************************************************************************************************



*********************************************************************************************************
*/
static void AppTask_Button(void *p_arg){
    OS_ERR  err;
    CPU_TS ts;
    BSP_LED_On(2);

    int button = 0;

    while (DEF_TRUE) {

    	while(!button){
    		button = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);
    		OSTimeDlyHMSM(0u, 0u, 0u, 50u,OS_OPT_TIME_HMSM_STRICT, &err);
    	}

    	OSSemPend(&MySem, 0, OS_OPT_PEND_BLOCKING, &ts,   &err);

    	switch(err){
    	case OS_ERR_NONE:
    		if(button){
    			button_pressed = 1;
    		}

    		OSSemPost(&MySem, OS_OPT_POST_1, &err);

    	case OS_ERR_PEND_ABORT:
    		break;
    	case OS_ERR_OBJ_DEL:
    		break;
    	}

    	button = 0;

    	OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}


static void AppTask_LED(void *p_arg){

	CPU_TS ts;
	OS_ERR  err;

	int LED_mode = 0;		// 0: LED rolling, 1: show the result of button press
	int LED_turn = 0;		//determine which LED to be turn on

 	 while (DEF_TRUE) {
 		 OSSemPend(&MySem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);

 		 switch(err){
 		 case OS_ERR_NONE:
 			 if(button_pressed){
 				 LED_mode = 1;
 			 }

 			 OSSemPost(&MySem, OS_OPT_POST_1, &err);

 		 case OS_ERR_PEND_ABORT:
 			 break;
 		 case OS_ERR_OBJ_DEL:
 			 break;
 		 }

 		 if(LED_mode){
    		 if(LED_turn == 2){
    			 LED_turn = 4;
    		 }
    		 else {
    			 LED_turn = 0;
    		 }
    		 LED_mode = 0;
 		 }
 		 else{
 			 LED_turn++;
 			 if(LED_turn >= 4){
        	 	LED_turn = 1;
 			 }
 		 }

 		 switch(LED_turn){
 		 case 0:
 			 BSP_LED_Off(1);
 			 BSP_LED_Off(2);
 			 BSP_LED_Off(3);
 			 break;
 		 case 1:
 			 BSP_LED_On(1);
 			 BSP_LED_Off(2);
 			 BSP_LED_Off(3);
 			 break;
 		 case 2:
 			 BSP_LED_Off(1);
 			 BSP_LED_On(2);
 			 BSP_LED_Off(3);
 			 break;
 		 case 3:
 			 BSP_LED_Off(1);
 			 BSP_LED_Off(2);
 			 BSP_LED_On(3);
 			 break;
 		 case 4:
 			 BSP_LED_On(1);
          	 BSP_LED_On(2);
          	 BSP_LED_On(3);
          	 break;
 		 }

 		 OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &err);
 	 }
}


static void AppTask_USART(void *p_arg){

    OS_ERR  err;
    CPU_TS ts;

    send_string("Press user button when blue LED is on\n\r");

    while (DEF_TRUE) {

       OSSemPend(&MySem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);


       switch(err){
              case OS_ERR_NONE:
                 if(button_pressed){
                     send_string("button pressed\n\r");
                 }

                 button_pressed = 0;
                 OSSemPost(&MySem, OS_OPT_POST_1, &err);

              case OS_ERR_PEND_ABORT:
                 break;
              case OS_ERR_OBJ_DEL:
                 break;
       }
       OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
   OS_ERR  err;

   u8_t idx = 0;
   task_t* pTask_Cfg;
   for(idx = 0; idx < TASK_N; idx++)
   {
      pTask_Cfg = &cyclic_tasks[idx];

      OSTaskCreate(
            pTask_Cfg->pTcb,
            pTask_Cfg->name,
            pTask_Cfg->func,
            (void         *)0u,
            pTask_Cfg->prio,
            pTask_Cfg->pStack,
            pTask_Cfg->pStack[APP_CFG_TASK_START_STK_SIZE / 10u],
            APP_CFG_TASK_START_STK_SIZE,
            (OS_MSG_QTY    )0u,
            (OS_TICK       )0u,
            (void         *)0u,
            (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR       *)&err
      );
   }
}

/*
*********************************************************************************************************
*                                          AppObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppObjCreate (void)
{

}
