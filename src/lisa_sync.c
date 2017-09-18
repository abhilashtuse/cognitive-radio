/*
 * lisa_sync.c: LISA Implementation
 * Author : Abhilash Tuse
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#define FILE_SIZE 128 //128 bytes(1k)
#define SYNC_BYTES 32
#define file_path "mytestdata.txt"
typedef unsigned char uchar_t;

uchar_t message[] = "Hello, SJSU_CMPE245_Abhilash_9326";
uint8_t payload_len;

static uchar_t sync_field[SYNC_BYTES] ={ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                      0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,0x5E, 0x5F,
        									            0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        									            0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
                                    };

void createFile() {
  FILE *fp;
  fp = fopen(file_path, "wb");
  assert(fp != NULL);
  uint8_t random_data_len = FILE_SIZE - SYNC_BYTES - payload_len;
  for (uint8_t i = 0; i < random_data_len; ++i) {
    fprintf(fp, "%c", rand() % 256);
  }
  fwrite(sync_field, sizeof(uchar_t), sizeof(sync_field), fp);
  fwrite(message, sizeof(char), payload_len, fp);
  fclose(fp);
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

void corruptSyncField(uint8_t n) {
  uint8_t sync_field_start = FILE_SIZE - SYNC_BYTES - payload_len;
  FILE *fp;
  fp = fopen(file_path, "r+b");
  assert(fp != NULL);
  int corr_start = rand() % SYNC_BYTES;
  int corr_count = SYNC_BYTES * (n / 100.0);
  if (corr_count > 0) {
    int rem = SYNC_BYTES - corr_start - corr_count;
    // printf("\nCorr_start:%d count:%d rem:%d", corr_start, corr_count, rem);
    if (rem < 0)
      corr_start += rem;
    // printf("\nupdated corr_start:%d", corr_start);
    printf("\ncorrupted sync field from %d to %d", corr_start + 1, corr_start + corr_count);
    //fseek(fp, sync_field_start, SEEK_SET);
    //printf("\nstart of sync: %x\n", fgetc(fp));
    fseek(fp, sync_field_start + corr_start, SEEK_SET);
    //printf("\ntrying to corrupt: %x\n", fgetc(fp));
    while (corr_count-- > 0)
      fputc(0xff, fp);
  }

  fclose(fp);
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

bool sequenceMatch(uchar_t rxBuf[], uchar_t c2, int shift, int ind, int seq_ind, int conf) {
  while (conf > 0 && seq_ind < SYNC_BYTES && ind < FILE_SIZE) {
    //printf("\nc1:%x c2:%x ind:%d, seq_ind:%d, buf_char:%x", c1,c2,ind, seq_ind,rxBuf[ind]);
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

bool parseData(int conf) {
  bool sync_match_complete = false;;
  uchar_t rxBuffer[FILE_SIZE];
  uchar_t c1, c2;
  int j = 0, match_index = 0, read_bytes = 0;
  FILE *fp;
  fp = fopen(file_path, "rb");
  assert(fp != NULL);
  read_bytes = fread(rxBuffer, 1, FILE_SIZE, fp);
  assert(FILE_SIZE == read_bytes);
  // printf("\nread: %d bytes\n", read_bytes);
  c1 = rxBuffer[j];//fgetc(fp);
  j++;
  c2 = rxBuffer[j];//fgetc(fp);
  j++;

  for (; j < FILE_SIZE; j++) {
    for (int i = 0; i < 8; i++) {
      int res = matchPattern(c1);
      //printf("\nC1: %x, C2: %x", c1, c2);
      if (res == -1) {
        c1 = (c1 << 1) | (c2 >> 7);
        c2 <<= 1;
      } else {
        match_index = res;
        sync_match_complete = sequenceMatch(rxBuffer, c2, i, j, match_index, conf-1);

        if (sync_match_complete) {
          //jump x bytes to get payload
          int payload_index = j - 2 + SYNC_BYTES - match_index;
          //printf("\nfile_ptr:%c %x match_index: %d",rxBuffer[j], rxBuffer[j], match_index);
          //printf("\n*******DONE:shift:%d Last match index: %d, char: %x %c, payload start char:%c %x \n",
          //       j,match_index, c1, c1, rxBuffer[payload_index], rxBuffer[payload_index]);
          printf("\nSync field matched from:%x", c1);
          char payload[payload_len];
          memcpy(payload, &rxBuffer[payload_index], payload_len);
          payload[payload_len] = '\0';
          printf("\nPayload:%s", payload);
          break;
        }
        c1 = (c1 << 1) | (c2 >> 7);
        c2 <<= 1;
      }
    }
    if (sync_match_complete)
      break;
    c2 = rxBuffer[j];//fgetc(fp);
  }
  fclose(fp);
  return sync_match_complete;
}

int main(void) {
  int corrupt_per, conf_level;
  payload_len = strlen((const char*)message);
  srand(time(NULL));
  createFile();
  printf("\nwhat percentage of sync field is corrupted? ");
  scanf("%d", &corrupt_per);
  corruptSyncField(corrupt_per);

  /*uchar_t rb[] = {0x8a, 0x92, 0x9a, 0xa2, 0xa8};
  bool test = sequenceMatch(0x50, 0x40, 5, rb, 0, 1, 3);
  if (test)
    printf("\nSuccess");
  else
    printf("\nfailed\n");
  return 0;*/

  printf("\nAfter Corruption:\n");
  printFile();
  printf("\nWhat is the confidence level? ");
  scanf("%d", &conf_level);
  if(!parseData(conf_level))
    printf("\nCould not locate payload. Frame is corrupted");

  return 0;
}
