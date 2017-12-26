// exampleApp.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "jetsonGPIO.h"
#include "tx1_lora.h"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REG_VERSION              0x42

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;

int fd = -1;
int _frequency=0;

int _packetIndex=0;
int _implicitHeaderMode=0;
char receiveLora=0;

jetsonTX1GPIONumber lora_cs = gpio187 ;
jetsonTX1GPIONumber lora_reset = gpio219 ;     // Output

void setNodeAddress(uint8_t addr)
{
	writeRegister(REG_PKT_CFG1, readRegister(REG_PKT_CFG1) | ADDR_FILTER_MODE2 << 1); //Set address filtering
	writeRegister(REG_NODE_ADRS, addr); //Set node address
}

void setBroadCastAddress(uint8_t addr)
{
	writeRegister(REG_BROADCAST_ADDR, addr);
}

void SPI_Init(void)
{
	int ret;
	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
}

static uint8_t spi_transfer(int byte_val)
{
	int ret;
	uint8_t tx[] = {
		byte_val
	};
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	/*for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 6))
			puts("");
		//printf("%.2X ", rx[ret]);
	}*/
	//puts("");
	return rx[0];
}


uint8_t singleTransfer(uint8_t address, uint8_t value)
{
  uint8_t response=0;

  //digitalWrite(_ss, LOW);
  gpioSetValue(lora_cs, off);  //CHIP_SELECT();
  //SPI.beginTransaction(_spiSettings);
  response = spi_transfer(address);
  // printf("address response %x\n",response);
  response = spi_transfer(value);
  // printf("value response %x\n",response);
  gpioSetValue(lora_cs, on);   //CHIP_DESELECT();

  //digitalWrite(_ss, HIGH);

  return response;
}

uint8_t readRegister(uint8_t address)
{
  return singleTransfer(address & 0x7f, 0x00);
}

void writeRegister(uint8_t address, uint8_t value)
{
  singleTransfer(address | 0x80, value);
}

void idle()
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void lora_sleep()
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void setTxPower(int level)
{
	if (level < 2)
	{
		level = 2;
	}
	else if (level > 17)
	{
		level = 17;
	}

	writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
}

void setFrequency(long frequency)
{
	if (frequency >= 525000000)
		writeRegister(REG_OP_MODE, readRegister(REG_OP_MODE) & ~(1 << 3)); // Access high frequency mode registers
	_frequency = frequency;
	printf("frequency is %ld,%d\n",frequency,_frequency);

	uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

	writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
	writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
	writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

int LoRabegin(long frequency)
{
  // setup pins
  int i;
  uint8_t version =0;

  // Initialize GPIO for resetting LORA 
  gpioExport(lora_reset) ;
  gpioSetDirection(lora_reset,outputPin) ;
  
  // Initialize GPIO for chip select

  gpioExport(lora_cs) ;
  gpioSetDirection(lora_cs,outputPin) ;

  // perform reset

  gpioSetValue(lora_reset, off);
  for(i=0;i<100000;i++);
  gpioSetValue(lora_reset, on);
  for(i=0;i<10000000;i++);

  // set SS high
  gpioSetValue(lora_cs, on); //CHIP_DESELECT();
 
  SPI_Init();

  // start SPI
  //SPI.begin();

  // check version
  version = readRegister(REG_VERSION);
  printf("Version is %x\n",version);
  if (version != 0x12) {
    return 0;
  }

  // put in sleep mode
  lora_sleep();
  // set frequency
  setFrequency(frequency);

  // set base addresses
  writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
  writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

  // set LNA boost
  writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

  // set output power to 17 dBm
  setTxPower(17);

  // put in standby mode
  idle();

  return 1;
}

void explicitHeaderMode()
{
  _implicitHeaderMode = 0;

  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

void implicitHeaderMode()
{
  _implicitHeaderMode = 1;

  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
}

int LoRabeginPacket(int implicitHeader)
{
  // put in standby mode
  idle();

  if (implicitHeader)
  {
    implicitHeaderMode();
  }
  else {

    explicitHeaderMode();
  }

  // reset FIFO address and paload length
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  writeRegister(REG_PAYLOAD_LENGTH, 0);

  return 1;
}

int LoRaendPacket()
{
	// put in TX mode
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

  // wait for TX done
   while((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);

  // clear IRQ's
  writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

  return 1;
}

int parsePacket(int size)
{
	int packetLength = 0;
	int irqFlags = readRegister(REG_IRQ_FLAGS);

	if (size > 0)
	{
		implicitHeaderMode();
		writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
	}
	else
	{
		explicitHeaderMode();
	}

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, irqFlags);

	if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)
	{
		// received a packet
		_packetIndex = 0;

		// read packet length
		if (_implicitHeaderMode)
		{
			packetLength = readRegister(REG_PAYLOAD_LENGTH);
		}
		else
		{
			packetLength = readRegister(REG_RX_NB_BYTES);
		}

		// set FIFO address to current RX address
		writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

		// put in standby mode
		idle();
	}
	else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE))
	{
		// not currently in RX mode

		// reset FIFO address
		writeRegister(REG_FIFO_ADDR_PTR, 0);

		// put in single RX mode
		writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
	}

	return packetLength;
}

int packetRssi()
{
	return (readRegister(REG_PKT_RSSI_VALUE) - (_frequency < 868E6 ? 164 : 157));
}

float packetSnr()
{
	return ((int8_t)readRegister(REG_PKT_SNR_VALUE)) * 0.25;
}

size_t lora_write(const uint8_t *buffer, size_t size)
{
	int currentLength = readRegister(REG_PAYLOAD_LENGTH);
	size_t i=0;
	// check size
	if ((currentLength + size) > MAX_PKT_LENGTH)
	{
		size = MAX_PKT_LENGTH - currentLength;
	}

	// write data
	for (i = 0; i < size; i++)
	{
		writeRegister(REG_FIFO, buffer[i]);
	}

	// update length
	writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);

	return size;
}

size_t writebyte(uint8_t byte)
{
	return lora_write(&byte, sizeof(byte));
}

int available()
{
	return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

int lora_read()
{
	if (!available()) {
		return -1;
	}

	_packetIndex++;

	return readRegister(REG_FIFO);
}

int peek()
{
	if (!available()) {
		return -1;
	}

	// store current FIFO address
	int currentAddress = readRegister(REG_FIFO_ADDR_PTR);

	// read
	uint8_t b = readRegister(REG_FIFO);

	// restore FIFO address
	writeRegister(REG_FIFO_ADDR_PTR, currentAddress);

	return b;
}

void finish()
{
    close(fd);
    gpioUnexport(lora_cs) ;
    gpioUnexport(lora_reset) ;
}

void setSpreadingFactor(int sf)
{
	if (sf < 6)
	{
		sf = 6;
	}
	else if (sf > 12)
	{
		sf = 12;
	}

	if (sf == 6)
	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
	}
	else
	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
	}

	writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
}

void setSignalBandwidth(long sbw)
{
  int bw;

  if (sbw <= 7.8E3) {
    bw = 0;
  } else if (sbw <= 10.4E3) {
    bw = 1;
  } else if (sbw <= 15.6E3) {
    bw = 2;
  } else if (sbw <= 20.8E3) {
    bw = 3;
  } else if (sbw <= 31.25E3) {
    bw = 4;
  } else if (sbw <= 41.7E3) {
    bw = 5;
  } else if (sbw <= 62.5E3) {
    bw = 6;
  } else if (sbw <= 125E3) {
    bw = 7;
  } else if (sbw <= 250E3) {
    bw = 8;
  } else /*if (sbw <= 250E3)*/ {
    bw = 9;
  }

  writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
}

void setCodingRate4(int denominator)
{
  if (denominator < 5) {
    denominator = 5;
  } else if (denominator > 8) {
    denominator = 8;
  }

  int cr = denominator - 4;

  writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

void setSyncWord(int sw)
{
  writeRegister(REG_SYNC_WORD, sw);
}

void lora_mode_crc_enable()
{
  writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) | 0x04);
}

void fsk_mode_crc_enable()
{
  writeRegister(REG_PKT_CFG1, readRegister(REG_PKT_CFG1) | (1 << 4));
}

void noCrc()
{
  writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) & 0xfb);
}

void enableScrambling()
{
  writeRegister(REG_PKT_CFG1, readRegister(REG_PKT_CFG1) | 0x40);
}

void enableBeacon()
{
  writeRegister(REG_PKT_CFG1, readRegister(REG_PKT_CFG1) & ~(1 << 7)); // Switch to fixed length packet format first in order to set beacon
  writeRegister(REG_PKT_CFG2, readRegister(REG_PKT_CFG2) | (1 << 3));
}


// Frequency hoping
int set_freq_hop_period(uint8_t freq_hop_period)
{
	// hoppingperiod = Ts * Freq_hopping_period
	writeRegister(REG_HOP_PERIOD, freq_hop_period);
	return 1;
}

uint8_t get_fhss_present_channel(void)
{
	uint8_t channel = readRegister(REG_HOP_CHANNEL);
	channel &= FHSS_PRESENT_CHANNEL;

	return channel;
}

int check_fhss_channel_change(long freq)
{
    // pg 32
    int success = 0;
	uint8_t irq_reg = readRegister(REG_IRQ_FLAGS);
	printf("reg_hop_period:%d irq_reg: %x\n", readRegister(REG_HOP_PERIOD), irq_reg);
	if(irq_reg & IRQ_FHSS_CHANNEL_CHANGE)
	{
        setFrequency(freq);
        writeRegister(REG_IRQ_FLAGS, IRQ_FHSS_CHANNEL_CHANGE);
        success = 1;
	}
    return success;
}

