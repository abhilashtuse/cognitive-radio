#ifndef TX1_LORA_H_
#define TX1_LORA_H_




#include <stddef.h>
#include <stdint.h>
// registers
#define REG_HOP_PERIOD		 0x24
#define REG_HOP_CHANNEL		 0x1c

#define REG_BITRATE_MSB		 0x02

#define REG_BITRATE_LSB		 0x03
		
#define REG_SYNC_CFG		 0x27

#define REG_NODE_ADRS		 0x33

#define REG_BROADCAST_ADDR	 0x34

#define REG_PKT_CFG1		 0x30

#define REG_PKT_CFG2		 0x31

#define REG_FIFO                 0x00

#define REG_OP_MODE              0x01

#define REG_FRF_MSB              0x06

#define REG_FRF_MID              0x07

#define REG_FRF_LSB              0x08

#define REG_PA_CONFIG            0x09

#define REG_LNA                  0x0c

#define REG_FIFO_ADDR_PTR        0x0d

#define REG_FIFO_TX_BASE_ADDR    0x0e

#define REG_FIFO_RX_BASE_ADDR    0x0f

#define REG_FIFO_RX_CURRENT_ADDR 0x10

#define REG_IRQ_FLAGS            0x12

#define REG_RX_NB_BYTES          0x13

#define REG_PKT_RSSI_VALUE       0x1a

#define REG_PKT_SNR_VALUE        0x1b

#define REG_MODEM_CONFIG_1       0x1d

#define REG_MODEM_CONFIG_2       0x1e

#define REG_PREAMBLE_MSB         0x20

#define REG_PREAMBLE_LSB         0x21

#define REG_PAYLOAD_LENGTH       0x22

#define REG_RSSI_WIDEBAND        0x2c

#define REG_DETECTION_OPTIMIZE   0x31

#define REG_DETECTION_THRESHOLD  0x37

#define REG_SYNC_WORD            0x39

#define REG_DIO_MAPPING_1        0x40

#define REG_VERSION              0x42



// modes

#define MODE_LONG_RANGE_MODE     0x80

#define MODE_SLEEP               0x00

#define MODE_STDBY               0x01

#define MODE_TX                  0x03

#define MODE_RX_CONTINUOUS       0x05

#define MODE_RX_SINGLE           0x06

#define ADDR_FILTER_MODE1	 0x01

#define ADDR_FILTER_MODE2	 0x02

// PA config

#define PA_BOOST                 0x80



// IRQ masks

#define IRQ_TX_DONE_MASK           0x08

#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20

#define IRQ_RX_DONE_MASK           0x40

#define IRQ_FHSS_CHANNEL_CHANGE    (1 << 1)
#define FHSS_PRESENT_CHANNEL 	   0x3F



#define MAX_PKT_LENGTH           255

#define	LOW 	0

#define HIGH	1



#define function_not_required 0


  int LoRabegin(long frequency);
  int LoRabeginPacket(int implicitHeader);
  int LoRaendPacket();

  void setNodeAddress(uint8_t addr);
  void setBroadCastAddress(uint8_t addr);
  int parsePacket(int size );
  int packetRssi();
  float packetSnr();

  // from Print
  size_t writebyte(uint8_t byte);
  size_t lora_write(const uint8_t *buffer, size_t size);

  // from Stream
  int available();
  int lora_read();
  int peek();
  void flush();

  void onReceive(void(*callback)(int));

  void receive(int size);
  void idle();
  void lora_sleep();

  void setTxPower(int level);
  void setFrequency(long frequency);
  void setSpreadingFactor(int sf);
  void setSignalBandwidth(long sbw);
  void setCodingRate4(int denominator);
  void setPreambleLength(long length);
  void setSyncWord(int sw);
  void lora_mode_crc_enable();
  void fsk_mode_crc_enable();
  void noCrc();
  void enableScrambling();

 // void dumpRegisters(Stream& out);
  uint8_t readRegister(uint8_t address);

  void explicitHeaderMode();
  void implicitHeaderMode();

  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

  void finish();
  int set_freq_hop_period(uint8_t freq_hop_period);
  uint8_t get_fhss_present_channel(void);
  int check_fhss_channel_change(long freq);

#endif /* TX1_LORA_H_ */
