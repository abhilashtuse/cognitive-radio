
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "lisa_sync.h"
#include "gpio.h"

#define RECEIVE_COUNT 200

int payload_len;

void transmit(uchar_t rxBuffer[], int buf_len, int order) {
	GPIOinitOut(2, 11); // Set RED LED as output
	GPIOinitOut(2, 12); // Set GREEN LED as output
	GPIOinitOut(2, 7); // Set Tx as output
	GPIOinitIn(2, 10); // Set switch as input

	bool bit_buf[buf_len*8];
	int cnt = 0;
	for (int i = 0; i < buf_len; i++) {
		for (int j = 0; j < 8; j++) {
			//if ((rxBuffer[i] >> j) & 1) // LSB-MSB
			if (rxBuffer[i] >> (7-j) & 1) // MSB-LSB
				bit_buf[cnt] = 1;
			else
				bit_buf[cnt] = 0;
			cnt++;
		}
	}
	//scramble bit_buf[32*8+1 to cnt]
	payload_len = cnt/8 - SYNC_BYTES;
	//printf("\npayload len:%d\nbefore_payload:", payload_len);
	bool data[payload_len * 8], sout[payload_len * 8];

	for (int i = SYNC_BYTES*8, j = 0; i < cnt; i++, j++) {
		data[j] = bit_buf[i];
	}
	scrambling(order, data, sout, sizeof(data)/sizeof(bool));

	// store scrambled data back in buffer
	for (int i = SYNC_BYTES*8, j = 0; i < cnt; i++, j++) {
		bit_buf[i] = sout[j];
	}

	for (int i = 0; i < cnt; i++) {
		if (bit_buf[i]) {
			setGPIO(2, 7);
		} else {
			clearGPIO(2, 7);
		}
		delay(1);
	}
	printf("\nTx complete..");
}

void bufferManipulator(bool buffer[], unsigned char bufferNew[] ,int n){
	int count=0;
	for(int i=0; i<n; i++){
		unsigned char byte=0;
		for(int j=0; j<8; j++){
			if(buffer[count++])
				byte |=(1<<j);
		}
		bufferNew[i]=byte;
	}
}

void receive(int conf, int order, int payload_len) {
	GPIOinitIn(2, 8); // Set Rx as Input

	int count = 0;
//	bool buffer[RECEIVE_COUNT*8] = {0,0,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,0,1,1,0,0,1,0,1,1,1,1,0,0,1,0,1,0,0,0,1,0,1,0,1,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,1,1,0,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,0,0,0,1,1,0,1,0,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,0,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,0,1,1,1,1,0,1,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,0,0,1,0,1,0,1,1,0,1,0,1,0,0,1,1,1,0,1,0,1,0,0,0,0,0,0,0,1,1,0,1,0,1,1,0,0,0,1,0,0,1,1,1,0,0,0,1,0,0,0,0,0,0,1,0,1,1,1,0,0,1,0,0,0,0,0,0,1,1,0,1,1,1,1,1,1,0,1,1,1,0,0,0,1,1,0,1,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,1,0,0,1,0,1,1,1,0,0,1,1,1,0,1,1,1,1,0,0,0,0,0,1,1,1,1,0,0,1,1,1,1,1,0,1,0,1,1,0,0,1,0,0,1,0,1,1,1,0,0,1,1,0,1,1,0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,0,1,1,1,0,1,0,1,0,1,0,1,1,0,1,0,0,1,1,1,0,1,1,1,0,0,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,0,0,1,1,1,1,0,1,0,0,1,0,1};
//	count=sizeof(buffer)/sizeof(bool);
	bool buffer[RECEIVE_COUNT*8];
	uchar_t bufferNew[RECEIVE_COUNT];

	while(count < (RECEIVE_COUNT*8)){
		if(LPC_GPIO2-> FIOPIN & (1<<8))
			buffer[count]=1;
		else
			buffer[count]=0;
		count++;
		delay(10);
	}

	printf("\nreceived:\n");
	for (int i = 0; i < count; i++) {
		printf("%d", buffer[i]);
	}
	bufferManipulator(buffer, bufferNew, count/8);

	int payload_index = 0;

	bool result=  parseData(conf, bufferNew, count/8, &payload_index);
	if(result) {
	  bool data[50*8], sout[50*8];
	  for (int i = payload_index, j = 0; i < count; i++, j++) {
		  data[j] = buffer[i];
	  }

	  descrambling(order, data, sout, sizeof(data)/sizeof(bool));
	  uchar_t payload[50] = "";
	  bufferManipulator(sout, payload, payload_len);
	  printf("\n final payload: %s", payload);
	}
	else
	  printf("\n fail");
}

int main(void) {
    printf("Hello from LPC 1769\n");
    int file_len = createFile();
	uchar_t buffer[FILE_SIZE];
	FILE *fp;
	fp = fopen(file_path, "rb");
	assert(fp != NULL);
	int read_bytes = fread(buffer, 1, file_len, fp);
	printf("\nread_bytes:%d", read_bytes);
	fclose(fp);

	bool flag = true;
	while (flag) {
		int order = 5, conf = 5;
		int ch;
		printf("\nEnter scrambling order:");
		scanf("%d", &order);
		printf("\n1. Transmit\n2. Receive\nEnter choice:");
		scanf("%d", &ch);
		switch(ch) {
			case 1:
				transmit(buffer, file_len, order);
				break;
			case 2:
				printf("\nEnter confidence level:");
				scanf("%d", &conf);
				receive(conf, order, file_len - SYNC_BYTES);
				break;
			case 3:
				flag = false;
				break;
			default:
				printf("\nwrong choice");
		}
    }
    printf("\nDone");
    return 0 ;
}
