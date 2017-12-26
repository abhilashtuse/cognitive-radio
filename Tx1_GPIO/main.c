/*
===============================================================================
 Name        : RF_Handshaking.c
 Description : RF and PWM code for LPC1769

 CTI One Corporation released for Dr. Harry Li for CMPE 245 Class use ONLY!
===============================================================================
*/

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "tx1_lora.h"

#define RF_Receive 1
#define TransmittACk 0
#define ack_start_stop 0
int rfInit(void);

char receiveData=0;
int packetSize;



/**************************************************************************************************
* @brief	wait for ms amount of milliseconds
* @param	ms : Time to wait in milliseconds
**************************************************************************************************/
static void delay_ms(unsigned int ms)
{
    unsigned int i,j;
    for(i=0;i<ms;i++)
        for(j=0;j<50000;j++);
}


/**************************************************************************************************
* main : Main program entry
**************************************************************************************************/
int main(void)
{
	long LUT[8]={	869000000,
				//724000000,
				750000000,
				790000000,
				800000000,
				845000000,
				850000000,
				869000000};
	// Working frequency range from 724 MHz to 1040 MHz.
	//LoRabegin(1040000000);
	//LoRabegin(1020000000);
	LoRabegin(724000000);
	//LoRabegin(750000000);
	//LoRabegin(790000000);
	//LoRabegin(800000000);
	//LoRabegin(845000000);
	//LoRabegin(850000000);
	//LoRabegin(910000000);
	//LoRabegin(868000000);
	
	// 11. PWR, 1. FBW, 2. WFR
	//LoRabegin(869000000);
	//LoRabegin(169000000); 
	
	printf("REG_MODEM_CONFIG_1: %x\n", readRegister(REG_MODEM_CONFIG_1)); // BWD=125kHz (Default)
	setSignalBandwidth(125E3); // 3. BWD: 125kHz
	printf("REG_MODEM_CONFIG_1: %x\n", readRegister(REG_MODEM_CONFIG_1)); // BWD=125kHz
	
	setCodingRate4(5); // 4.CDR: 4/5 (Default)
	
	printf("REG_MODEM_CONFIG_2: %x\n", readRegister(REG_MODEM_CONFIG_2)); // SPD=7 (Default)
	setSpreadingFactor(12); // 5. SPD = 12
	printf("REG_MODEM_CONFIG_2: %x\n", readRegister(REG_MODEM_CONFIG_2)); // SPD=12

	//6. FHSS
	set_freq_hop_period(100);
	
	printf("lora_mode Modem CFG: %x\n", readRegister(REG_MODEM_CONFIG_2)); // disable (Default)
	lora_mode_crc_enable(); // 7. CRC
	printf("lora_mode Modem CFG: %x\n", readRegister(REG_MODEM_CONFIG_2)); // enable
	
	writeRegister(REG_OP_MODE, readRegister(REG_OP_MODE) & ~(1 << 6)); // Share FSK address space (0x0D:0x3F)
	printf("REG_PKT_CFG1: %x\n", readRegister(REG_PKT_CFG1));
	enableScrambling(); // 9. Scrambling / whitening
	printf("REG_PKT_CFG1: %x\n", readRegister(REG_PKT_CFG1));

	//enableBeacon(); // 10.Beacon (Only for fixed length packet format)

	// 13. Channel Filter: RxBw = 10.4   Bit Rate 4.8 kbps    (Note: BitRate < 2 x RxBw)
	
	//printf("FSK_mode Pkt CFG: %x\n", readRegister(REG_PKT_CFG1));
	//fsk_mode_crc_enable(); // Its enabled by default
	//printf("FSK_mode Pkt CFG: %x\n", readRegister(REG_PKT_CFG1));

	//8. ADR: address based communication
//	writeRegister(REG_SYNC_CFG, readRegister(REG_SYNC_CFG) | (1 << 4)); // Set SYNC on
//	writeRegister(REG_PKT_CFG1, readRegister(REG_PKT_CFG1) | (1 << 2)); // Set Address filtering (Address field must match node address or broadcast address)
//	setNodeAddress(0xA1);
//	setBroadCastAddress(0x51);
	
	writeRegister(REG_OP_MODE, readRegister(REG_OP_MODE) | (1 << 6)); // Switch to LoRa mode
	
//	printf("bit rate: msb: %x lsb: %x\n", readRegister(REG_BITRATE_MSB), readRegister(REG_BITRATE_LSB));
	int is_tx = 0, freq_counter = 0, present_ch, received = 0, size;
	printf("Enter (1/0) for rx/tx\n");
	
	if (scanf("%d", &is_tx) && is_tx) {
		while(1)
		{
			if(received)
			{
				int channel = get_fhss_present_channel();
				printf("Channel: %d\n", channel);
				if(check_fhss_channel_change(LUT[freq_counter]))
				{
					freq_counter++;
					if(freq_counter == sizeof(LUT)/sizeof(LUT[0]))
						freq_counter = 0;
					printf("Channel: %d\n", channel);
					received = 0;
				}
				
			}
			/*if (received == 0)
			{
				const char buffer[] = "R";
				printf("Start Sending data \n");
				delay_ms(1000);
				LoRabeginPacket(0);
				//writebyte(Acknowledgement);
				size = lora_write((uint8_t*)buffer, 1);
				LoRaendPacket();
				printf("%d bytes sent \n", size);
			}*/
			while (received == 0) 
			{
				packetSize = parsePacket(0);
				if (packetSize)
				{
					//counter = 0;
					//NVIC_EnableIRQ(TIMER0_IRQn);
					//received a packet
					// read packet
					while (available())
					{
						//counter = 0;
						receiveData = lora_read();
						if (receiveData == '2')
						{
							receiveData = lora_read();						
							//printf("%c",receiveData);
							if(receiveData == 'R')
							{
								printf("Received packet '");
								printf("%c",receiveData);
								printf("' with RSSI ");
								printf("%d ",packetRssi());
								printf(" with SNR ");
								printf("%f\n",packetSnr());
								printf("Enter 1 to send data\n");
								scanf("%d", &is_tx);
								const char buffer[] = "5Data from Tx1";
								printf("Start Sending data \n");
								delay_ms(1000);
								LoRabeginPacket(0);
								//writebyte(Acknowledgement);
								size = lora_write((uint8_t*)buffer, sizeof(buffer));
								LoRaendPacket();
								printf("%d bytes sent \n", size);
							}
						}
					}
					received = 1;
					break;
				}
			}
		}
  	}
	else {
		const char buffer[] = "Data from NVIDIA Jetson TX1";
		//char Acknowledgement;
		int size = 0;
		//Acknowledgement = 'A';
		while(1)
		{
			printf("Start Sending data \n");
			delay_ms(1000);
			LoRabeginPacket(0);
			//writebyte(Acknowledgement);
			size = lora_write((uint8_t*)buffer, sizeof(buffer));
			LoRaendPacket();
			printf("%d bytes sent \n", size);

		}
	}
  finish(); //unexport gpio
}

