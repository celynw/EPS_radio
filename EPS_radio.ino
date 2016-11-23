// ##############################################
// EPS Group H 2016
// Sketch for ATMEGA328P and NRF24L01
//
// CPU			8-bit AVR
// Frequency	20 MHz
// Performance	20 MIPS (at 20 MHz)
// Flash memory	32kB	(32,256)
// SRAM			2kB		(2048)
// EEPROM		1kB		(1024)
// Package		28-pin PDIP MLF / 32-pin TQFP MLF
//
// Pin		Arduino		Radio
// GND		GND
// VCC		3.3V
// CE		8
// CSN		10
// SCK		13
// MOSI		11
// MISO		12
// IRQ		2/3
//
// ##############################################

// We'll need so use our own 'efficient' ATMEGA power supply so sleep actually matters

#include <SPI.h>
#include <RadioHead.h>
#include <RH_NRF24.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

// Could be useful
// nrf24.waitAvailableTimeout(500)
// nrf24.waitAvailable()
// detachInterrupt(irqNum)

// Comment next line to remove serial messages from compilation
#define DEBUG
#ifdef DEBUG
	#define DEBUG_BEGIN(x) Serial.begin(x)
	#define DEBUG_PRINT(x) Serial.print(x)
	#define DEBUG_PRINTLN(x) Serial.println(x)
#else
	#define DEBUG_BEGIN(x)
	#define DEBUG_PRINT(x)
	#define DEBUG_PRINTLN(x)
#endif

// Singleton instance of the radio driver
RH_NRF24 nrf24;

const byte irqNum = digitalPinToInterrupt(2); // Use 2 or 3

// Runs on startup
void setup() {
	DEBUG_BEGIN(9600);
	check("init       ", nrf24.init());
	check("setChannel ", nrf24.setChannel(1)); // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
	check("setRF      ", nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm));
	attachInterrupt(irqNum, isr, CHANGE); // LOW, CHANGE, RISING, FALLING

	DEBUG_PRINTLN();
}

// Loops after setup()
void loop() {
	// sendMsg("Hello World!");
	if (!recvMsg()) DEBUG_PRINT(F("."));
	delay(1000);
}

//Handy for checking if functions fail
void check(char* input, bool test) {
	DEBUG_PRINT(input);
	if (test)
		DEBUG_PRINTLN(F("[  OK  ]"));
	else
		DEBUG_PRINTLN(F("[FAILED]"));
}

//Send a message
void sendMsg(uint8_t data[]) {
	DEBUG_PRINT(F("Sent: "));
	DEBUG_PRINTLN((char*)data);
	// Send
	nrf24.send(data, sizeof(data));
	nrf24.waitPacketSent();
}

//Receive a message from a server if there is one
bool recvMsg() {
	if (nrf24.available()) {
		// There's a message waiting
		uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
		uint8_t len = sizeof(buf);
		if (nrf24.recv(buf, &len)) {
			DEBUG_PRINTLN();
			DEBUG_PRINT(F("Rcvd: "));
			DEBUG_PRINTLN((char*)buf);
			}
			else
				DEBUG_PRINTLN(F("recv failed"));
		return true;
	}
	else
		// No messages waiting
		return false;
}

// Called on an interrupt
void isr() {
	detachInterrupt(0); // Already called, stop it being called repeatedly
	// http://playground.arduino.cc/Learning/ArduinoSleepCode
	// POWER_MODE_IDLE
	// SLEEP_MODE_IDLE
	// Ensure that the 8-bit timer is disabled if using arduino layer
	// PRR: Power Reduction Register
	// 	Before entering sleep mode:
	// 	PRR = PRR | 0b00100000
	// 	After exiting sleep mode:
	// 	PRR = PRR & 0b0000000
	// millis() now unreliable for time comparison before and after sleep
}

void sleepNow()
{
	// http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
	// The 5 different modes are:
	// 	SLEEP_MODE_IDLE		- least power savings
	// 	SLEEP_MODE_ADC
	// 	SLEEP_MODE_PWR_SAVE
	// 	SLEEP_MODE_STANDBY
	// 	SLEEP_MODE_PWR_DOWN	- most power savings
	set_sleep_mode(SLEEP_MODE_IDLE);

	sleep_enable(); // Enables sleep bit in the MCUCR

	// Functions in avr/power.h for more savings
	power_adc_disable();
	power_spi_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	power_twi_disable();

	sleep_mode(); // Device is actually put to sleep here
	// AFTER WAKING UP - CONTINUES FROM HERE
	sleep_disable(); // Disables sleep bit in the MCUCR
	power_all_enable();
}
