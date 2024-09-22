/*
 * main.c
 *
 *  Created on: 2024Äê9ÔÂ21ÈÕ
 *      Author: Administrator
 */


#include <stdio.h>
#include <stdlib.h>
#include "xil_io.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xscugic.h"
#include "sleep.h"

#define INTC_DEVICE_ID 			XPAR_PS7_SCUGIC_0_DEVICE_ID

#define SW_INTR_ID_0			0
#define SW_INTR_ID_1			1

#define SW_INTR_PRI_0			0x8
#define SW_INTR_PRI_1			0x0

/* XScuGic interrupt manager */
XScuGic IntcInstance;
XScuGic_Config* IntcConfig;

int NestSwitch = 0;
int Nested = 0;

void SwIntrHandler0(void* callbackRef)
{
	XScuGic* ptrIntc = (XScuGic*)callbackRef;
	xil_printf("Low Priority Interrupt In \r\n");
	if(NestSwitch)
	{
		Xil_EnableNestedInterrupts();
		Nested = 1;
	}
	usleep(1);
	/* Trigger high priority interrupt */
	XScuGic_SoftwareIntr(ptrIntc,
					SW_INTR_ID_1,
					XSCUGIC_SPI_CPU0_MASK);
	usleep(1);
	if(NestSwitch)
	{
		Xil_DisableNestedInterrupts();
		Nested = 0;
	}
	xil_printf("Low Priority Interrupt Out \r\n\r\n");
	NestSwitch = !NestSwitch;
}

void SwIntrHandler1(void* par)
{
	xil_printf("High Priority Interrupt In\r\n");
	if(Nested)
	{
		xil_printf("\t Nested \r\n");
	}
	else
	{
		xil_printf("Not Nested\r\n");
	}
	xil_printf("High Priority Interrupt Out\r\n\r\n");
}

int main(void)
{
	u8 prio, trig;
	xil_printf("Standalone Nested Interrupt Test Start\r\n");

	/* Setup ScuGic Device */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	XScuGic_CfgInitialize( 	/*InstancePtr*/		&IntcInstance,
							/*ConfigPtr*/		IntcConfig,
							/*EffectiveAddr*/	IntcConfig->CpuBaseAddress);
	/* Setup Exception and link ScuGic device to exception */
	Xil_ExceptionRegisterHandler(	/*Exception_id*/	XIL_EXCEPTION_ID_INT,
									/*Handler*/			(Xil_ExceptionHandler)XScuGic_InterruptHandler,
									/*Data*/			&IntcInstance);
	Xil_ExceptionEnable();

	/* Setup software interrupt 0, with lower priority */
	XScuGic_GetPriorityTriggerType(&IntcInstance, SW_INTR_ID_0, &prio, &trig);
	prio = SW_INTR_PRI_0;
	XScuGic_SetPriorityTriggerType(&IntcInstance, SW_INTR_ID_0, prio, trig);
	XScuGic_Connect(&IntcInstance,
			SW_INTR_ID_0,
			(Xil_ExceptionHandler)SwIntrHandler0,
			(void*)&IntcInstance);
	XScuGic_Enable(&IntcInstance, SW_INTR_ID_0);

	/* Setup software interrupt 1, with higher priority */
	XScuGic_GetPriorityTriggerType(&IntcInstance, SW_INTR_ID_1, &prio, &trig);
	prio = SW_INTR_PRI_1;
	XScuGic_SetPriorityTriggerType(&IntcInstance, SW_INTR_ID_1, prio, trig);
	XScuGic_Connect(&IntcInstance,
			SW_INTR_ID_1,
			(Xil_ExceptionHandler)SwIntrHandler1,
			(void*)&IntcInstance);
	XScuGic_Enable(&IntcInstance, SW_INTR_ID_1);

	while(1)
	{
		/* Trigger software interrupt 0 */
		XScuGic_SoftwareIntr(&IntcInstance,
				SW_INTR_ID_0,
				XSCUGIC_SPI_CPU0_MASK);
		sleep(1);
	}
}


