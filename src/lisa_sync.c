/**
 * LISA Implementation
 * @Author: Abhilash Tuse
 * @Date:   2017-09-22T16:48:32-07:00
 * @Filename: lisa_sync.c
 * @version: Debug
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "lisa_sync.h"

#define DEBUG 0
static uchar_t message[] = "Hello, SJSU_CMPE245_Abhilash_9326";
//static uchar_t message[] = "Hello";
static uint8_t payload_len;
static uchar_t sync_field[SYNC_BYTES] ={0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                                        0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                                        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                        0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,0x5E, 0x5F };

int createFile() {
  int file_len = 0;
  FILE *fp;
  fp = fopen(file_path, "wb");
  assert(fp != NULL);

#if 0 //Skip random bytes
  uint8_t random_data_len = FILE_SIZE - SYNC_BYTES - payload_len;
  for (uint8_t i = 0; i < random_data_len; ++i) {
    fprintf(fp, "%c", rand() % 256);
  }
  file_len += random_data_len;
#endif

  file_len += fwrite(sync_field, sizeof(uchar_t), sizeof(sync_field), fp);
  payload_len = strlen((const char*)message);
  file_len += fwrite(message, sizeof(char), payload_len, fp);
  fclose(fp);
  return file_len;
}

void printFile() {
  FILE *fp;
  fp = fopen(file_path, "rb");
  assert(fp != NULL);
  while (!feof(fp)) {
    int c = fgetc(fp);
    printf("%x %c\t", c, c);
  }
  fclose(fp);
}

void corruptSyncField(uint8_t corrupt_per) {
  uint8_t sync_field_start = FILE_SIZE - SYNC_BYTES - payload_len;
  FILE *fp;
  fp = fopen(file_path, "r+b");
  assert(fp != NULL);
  int corr_start = rand() % SYNC_BYTES;
  int corr_count = SYNC_BYTES * (corrupt_per / 100.0);
  if (corr_count > 0) {
    int rem = SYNC_BYTES - corr_start - corr_count;
    if (rem < 0)
      corr_start += rem;
    printf("\nCorrupted sync field from %d to %d", corr_start + 1, corr_start + corr_count);
    fseek(fp, sync_field_start + corr_start, SEEK_SET);
    while (corr_count-- > 0)
      fputc(0xff, fp);
  }

  fclose(fp);
}

void scrambling(int order, bool data[], bool out[], int len) {
	bool dx, dy;
	bool debug_dx[len], debug_dy[len];
	for (int i = 0; i < len; i++) {
		if (i >= ((order / 2) + 1))
			dx = out[i - ((order / 2) + 1)];
		else
			dx = 0;
		if (i >= order)
			dy = out[i - order];
		else
			dy = 0;
		debug_dx[i] = dx;
		debug_dy[i] = dy;
		out[i] = data[i] ^ dx ^ dy;
	}
#if DEBUG
    printf("\nScrambling:");
    printf("\nInput: ");
    for (int i = 0; i < len; i++) {
        printf("%d", data[i]);
    }
    printf("\nD%dT1:  ", (order / 2) + 1);
    for (int i = 0; i < len; i++) {
        printf("%d", debug_dx[i]);
    }
    printf("\nD%dT1:  ", order);
    for (int i = 0; i < len; i++) {
        printf("%d", debug_dy[i]);
    }
    printf("\nOutput:");
    for (int i = 0; i < len; i++) {
        printf("%d", out[i]);
    }
#endif
}

void descrambling(int order, bool data[], bool out[], int len) {
    bool dx, dy;
    bool debug_dx[len], debug_dy[len];
    for (int i = 0; i < len; i++) {
        if (i >= ((order / 2) + 1))
            dx = data[i - ((order / 2) + 1)];
        else
            dx = 0;
        if (i >= order)
            dy = data[i - order];
        else
            dy = 0;
        debug_dx[i] = dx;
        debug_dy[i] = dy;
        out[i] = data[i] ^ dx ^ dy;
    }
#if DEBUG
    printf("\nDescrambling:");
    printf("\ndata:  ");
    for (int i = 0; i < len; i++) {
        printf("%d", data[i]);
    }
    printf("\nD%dT1:  ", (order / 2) + 1);
    for (int i = 0; i < len; i++) {
        printf("%d", debug_dx[i]);
    }
    printf("\nD%dT1:  ", order);
    for (int i = 0; i < len; i++) {
        printf("%d", debug_dy[i]);
    }
    printf("\nOutput:");
    for (int i = 0; i < len; i++) {
        printf("%d", out[i]);
    }
#endif
}


int matchPattern(uchar_t data) {
  for(int i = 0; i < SYNC_BYTES; i++) {
    //printf("\nmatch_index: %d\n", match_index);
    if (data == sync_field[i]) {
      return i;
    }
  }
  return -1;
}

bool sequenceMatch(uchar_t rxBuf[], uchar_t c2, int shift, int ind, int seq_ind, int conf, int buf_len) {
  while (conf > 0 && seq_ind < SYNC_BYTES && ind < buf_len) {
    seq_ind++;
    uchar_t c1 = c2;
    c2 = rxBuf[ind];
    ind++;
    c1 |= c2 >> (8 - shift);
    if (c1 != sync_field[seq_ind])
      break;
    c2 <<= shift;
    conf--;
  }

  if (conf == 0)
    return true;
  return false;
}

void printMessage(uchar_t buf[], int ind, int buflen, int shift) {
  uchar_t c1 = buf[ind];
  uchar_t c2 = buf[ind+1];
  ind += 2;
  printf("\nPayload:\n");
  while(ind < buflen) {
    c1 |= (c2 >> (8-shift));
    c2 <<= shift;
    printf("%c", c1);
    c1 = c2;
    c2 = buf[ind];
    ind++;
  }
}

bool parseData(int conf, uchar_t rxBuffer[], int buf_len, int *final_index) {
  bool sync_match_complete = false;
  uchar_t c1, c2;
  int j, match_index = -1;
  c1 = rxBuffer[0];
  c2 = rxBuffer[1];

  for (j = 2; j < buf_len; j++) {
    for (int i = 0; i < 8; i++) {
      int res = matchPattern(c1);
      if (res == -1) {
        c1 = (c1 << 1) | (c2 >> 7);
        c2 <<= 1;
      } else {
        match_index = res;
        // One sync field is matched, check if next 'conf-1' number of sync fields are present in the 'rxBuffer'
        sync_match_complete = sequenceMatch(rxBuffer, c2, i, j, match_index, conf - 1, buf_len);

        if (sync_match_complete) {
          int payload_index = j - 2 + SYNC_BYTES - match_index;
          // printf("\nSync field matched from:%x shift:%d", c1, i);
          *final_index = payload_index * 8 + i;
          /*uchar_t payload[payload_len];
          memcpy(payload, &rxBuffer[payload_index], payload_len);
          payload[payload_len] = '\0';
          payload[payload_len + 1] = '\0';
          printMessage(payload, 0, payload_len + 2, i);*/
          break;
        }
        c1 = (c1 << 1) | (c2 >> 7);
        c2 <<= 1;
      }
    }
    if (sync_match_complete)
      break;
    c2 = rxBuffer[j]; // read next character
  }
  return sync_match_complete;
}
