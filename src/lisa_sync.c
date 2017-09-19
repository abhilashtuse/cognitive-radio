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

#define FILE_SIZE 128 //128 bytes(1k)
#define SYNC_BYTES 32
#define file_path "mytestdata.txt"
typedef unsigned char uchar_t;

uchar_t message[] = "Hello, SJSU_CMPE245_Abhilash_9326";
uint8_t payload_len;

static uchar_t sync_field[SYNC_BYTES] ={0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                                        0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                                        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                        0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,0x5E, 0x5F };

/**
 * createFile() - Creates file with random data, sync fields and payload.
 */
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

/**
 * printFile() - print file contents character by character with hex values for debugging
 */
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

/**
 * corruptSyncField() - Corrupts sync fields present in the receiver buffer
 * @corrupt_per:	percentage of sync fields to be corrupted
 *
 * Calculates number of sync fields to be corrupted from percentage and starts
 * corrupting those many sync fields from a random location.
 */
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
    printf("\ncorrupted sync field from %d to %d", corr_start + 1, corr_start + corr_count);
    fseek(fp, sync_field_start + corr_start, SEEK_SET);
    while (corr_count-- > 0)
      fputc(0xff, fp);
  }

  fclose(fp);
}

/**
 * matchPattern() - Match given character with the sync fields
 * @data:	character to be find in the sync fields
 *
 * Return: Index of the sync field matched with data, or -1 if no match found.
 */
int matchPattern(uchar_t data) {
  for(int i = 0; i < SYNC_BYTES; i++) {
    //printf("\nmatch_index: %d\n", match_index);
    if (data == sync_field[i]) {
      return i;
    }
  }
  return -1;
}

/**
 * sequenceMatch() - Sequentially matches receiver buffer with the sync fields.
 * @rxBuf:   receiver buffer.
 * @c2:      character that is being processed in the buffer currently.
 * @shift:   number of bits shifted in the current character.
 * @ind:     receiver buffer index.
 * @seq_ind: Sync field index.
 * @conf:    Confidence level
 *
 * This function is called once any one of the sync field is located within the
 * receiver buffer. It then try to Sequentially match required (as per
 * Confidence level) number of sync fields within the receiver buffer.
 *
 * Return: true, if required number of sync fields found Sequentially within
 * the receiver bufffer, or false.
 */
bool sequenceMatch(uchar_t rxBuf[], uchar_t c2, int shift, int ind, int seq_ind, int conf) {
  while (conf > 0 && seq_ind < SYNC_BYTES && ind < FILE_SIZE) {
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

/**
 * printMessage() - Print the payload message character by character with
 * required bit shift.
 * @buf:	receiver buffer part which contains payload.
 * @ind:	Index of 1st character to be processed in the buffer.
 * @buflen:	Length of the buffer.
 * @shift:	Number of bits to be shifted.
 */
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

/**
 * parseData() - Parse receiver buffer to find out payload
 * @conf:	Confidence level.
 *
 * Based on the Confidence level, this function looks for sync field present in
 * the receiver buffer. If it finds required number of sync fields, it traces
 * the payload start and prints the message.
 *
 * Return: true if it successfully finds the payload, or false.
 */
bool parseData(int conf) {
  bool sync_match_complete = false;;
  uchar_t rxBuffer[FILE_SIZE];
  uchar_t c1, c2;
  int j, match_index = -1, read_bytes = 0;
  FILE *fp;
  fp = fopen(file_path, "rb");
  assert(fp != NULL);
  read_bytes = fread(rxBuffer, 1, FILE_SIZE, fp);
  assert(FILE_SIZE == read_bytes);
  c1 = rxBuffer[0];
  c2 = rxBuffer[1];

  for (j = 2; j < FILE_SIZE; j++) {
    for (int i = 0; i < 8; i++) {
      int res = matchPattern(c1);
      if (res == -1) {
        c1 = (c1 << 1) | (c2 >> 7);
        c2 <<= 1;
      } else {
        match_index = res;
        // One sync field is matched, check if next 'conf-1' number of sync fields are present in the 'rxBuffer'
        sync_match_complete = sequenceMatch(rxBuffer, c2, i, j, match_index, conf - 1);

        if (sync_match_complete) {
          int payload_index = j - 2 + SYNC_BYTES - match_index;
          // printf("\nSync field matched from:%x shift:%d", c1, i);
          uchar_t payload[payload_len];
          memcpy(payload, &rxBuffer[payload_index], payload_len);
          payload[payload_len] = '\0';
          payload[payload_len + 1] = '\0';
          printMessage(payload, 0, payload_len + 2, i);
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
  fclose(fp);
  return sync_match_complete;
}

int main(void) {
  int corrupt_per, conf_level;
  payload_len = strlen((const char*)message);
  srand(time(NULL));
  createFile();
  printf("what percentage of sync field is corrupted? (0-100):");
  scanf("%d", &corrupt_per);
  corruptSyncField(corrupt_per);

  printf("\nFile after Corruption:\n");
  printFile();
  printf("\nWhat is the confidence level? (1-32):");
  scanf("%d", &conf_level);
  if(!parseData(conf_level))
    printf("\nCould not locate payload. Frame is corrupted.");

  return 0;
}
