#include "gpio.h"

void GPIOinitOut(uint8_t portNum, uint32_t pinNum)
{
	if (portNum == 0)
	{
		LPC_GPIO0->FIODIR |= (1 << pinNum);
	}
	else if (portNum == 1)
	{
		LPC_GPIO1->FIODIR |= (1 << pinNum);
	}
	else if (portNum == 2)
	{
		LPC_GPIO2->FIODIR |= (1 << pinNum);
	}
	else
	{
		printf("Not a valid port!\n");
	}
}

// Initialize the port and pin as input.
void GPIOinitIn(uint8_t portNum, uint32_t pinNum)
{
	if (portNum == 0)
	{
		LPC_GPIO0->FIODIR &= ~(1 << pinNum);
	}
	else if (portNum == 1)
	{
		LPC_GPIO1->FIODIR &= ~(1 << pinNum);
	}
	else if (portNum == 2)
	{
		LPC_GPIO2->FIODIR &= ~(1 << pinNum);
	}
	else
	{
		printf("Not a valid port!\n");
	}
}

// Activate the pin
void setGPIO(uint8_t portNum, uint32_t pinNum)
{
	if (portNum == 0)
	{
		LPC_GPIO0->FIOSET = (1 << pinNum);
	}
	else if (portNum == 1)
	{
		LPC_GPIO1->FIOSET = (1 << pinNum);
	}
	else if (portNum == 2)
	{
		LPC_GPIO2->FIOSET = (1 << pinNum);
	}
	else
	{
		printf("Not Valid port to set!\n");
	}
}

// Deactivate the pin
void clearGPIO(uint8_t portNum, uint32_t pinNum)
{
	if (portNum == 0)
	{
		LPC_GPIO0->FIOCLR = (1 << pinNum);
	}
	else if (portNum == 1)
	{
		LPC_GPIO1->FIOCLR = (1 << pinNum);
	}
	else if (portNum == 2)
	{
		LPC_GPIO2->FIOCLR = (1 << pinNum);
	}
	else
	{
		printf("Not Valid port to clear!\n");
	}
}

void delay(uint32_t delay_ms)
{
	LPC_TIM0->TCR = 0x02;
	LPC_TIM0->PR = 0x00;
	LPC_TIM0->MR0 = delay_ms*(9000000 / 1000-1);
	LPC_TIM0->IR = 0xff;
	LPC_TIM0->MCR = 0x04;
	LPC_TIM0->TCR = 0x01;
	while (LPC_TIM0->TCR & 0x01);
	return;
}


