/*
 * lisa_sync.h
 *
 *  Created on: Oct 11, 2017
 *      Author: Abhilash
 */

#ifndef LISA_SYNC_H_
#define LISA_SYNC_H_

#include <stdint.h>

typedef unsigned char uchar_t;

#define FILE_SIZE 128 //128 bytes(1k)
#define SYNC_BYTES 32
#define file_path "mytestdata.txt"

/**
 * createFile() - Creates file with random data, sync fields and payload.
 *
 * Return: File length.
 */
int createFile();

/**
 * printFile() - print file contents character by character with hex values for debugging
 */
void printFile();

/**
 * corruptSyncField() - Corrupts sync fields present in the receiver buffer
 * @corrupt_per:	percentage of sync fields to be corrupted
 *
 * Calculates number of sync fields to be corrupted from percentage and starts
 * corrupting those many sync fields from a random location.
 */
void corruptSyncField(uint8_t n);

/**
 * matchPattern() - Match given character with the sync fields
 * @data:	character to be find in the sync fields
 *
 * Return: Index of the sync field matched with data, or -1 if no match found.
 */
int matchPattern(uchar_t data);

/**
 * scrambling() - Scramble message in order to remove long “1s” or “0s”
 * because they tend to lose timing
 * @order:	Scrambling order.
 * @data: Original message to be scrambled.
 * @out: Output buffer to store scrambled message.
 * @len: Length of original message.
 *
 */
void scrambling(int order, bool data[], bool out[], int len);

/**
 * descrambling() - Descramble received payload in order to receive original payload
 * @order:	Scrambling order.
 * @data: Received payload.
 * @out: Output buffer.
 * @len: Length of received payload.
 *
 */
void descrambling(int order, bool data[], bool out[], int len);

/**
 * sequenceMatch() - Sequentially matches receiver buffer with the sync fields.
 * @rxBuf:   receiver buffer.
 * @c2:      character that is being processed in the buffer currently.
 * @shift:   number of bits shifted in the current character.
 * @ind:     receiver buffer index.
 * @seq_ind: Sync field index.
 * @conf:    Confidence level.
 * @buf_len: buffer length.
 *
 * This function is called once any one of the sync field is located within the
 * receiver buffer. It then try to Sequentially match required (as per
 * Confidence level) number of sync fields within the receiver buffer.
 *
 * Return: true, if required number of sync fields found Sequentially within
 * the receiver bufffer, or false.
 */
bool sequenceMatch(uchar_t rxBuf[], uchar_t c2, int shift, int ind, int seq_ind, int conf, int buf_len);

/**
 * printMessage() - Print the payload message character by character with
 * required bit shift.
 * @buf:	receiver buffer part which contains payload.
 * @ind:	Index of 1st character to be processed in the buffer.
 * @buflen:	Length of the buffer.
 * @shift:	Number of bits to be shifted.
 */
void printMessage(uchar_t buf[], int ind, int buflen, int shift);

/**
 * parseData() - Parse receiver buffer to find out payload
 * @conf:	Confidence level.
 * @rxBuffer: Receiver buffer.
 * @buf_len: Length of receiver buffer.
 *
 * Based on the Confidence level, this function looks for sync field present in
 * the receiver buffer. If it finds required number of sync fields, it traces
 * the payload start and prints the message.
 *
 * Return: true if it successfully finds the payload, or false.
 */
bool parseData(int conf, uchar_t rxBuffer[], int buf_len, int *final_ind);


#endif /* LISA_SYNC_H_ */
