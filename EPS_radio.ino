// ################################################
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
// SPI uses pin 13 as SCK - overriding onboard LED!
// ################################################
// Use own 'efficient' ATMEGA power supply so sleep actually matters

#include <SPI.h>
#include <RadioHead.h>
#include <RH_NRF24.h> // Radio module
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h> // Use PRR (power reduction register) to save in idle(/running!)

// Could be useful
// nrf24.waitAvailableTimeout(500)
// nrf24.waitAvailable()
// detachInterrupt(irqNum)
// LED_BUILTIN

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
#define IRQ_PIN 2

// Singleton instance of the radio driver
RH_NRF24 nrf24;

const byte irqNum = digitalPinToInterrupt(IRQ_PIN); // Use 2 or 3
bool interrupted = false;

// Runs on startup
void setup() {
	pinMode(IRQ_PIN, INPUT); //Interrupt
	DEBUG_BEGIN(9600);
	check("init       ", nrf24.init());
	check("setChannel ", nrf24.setChannel(1)); // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
	check("setRF      ", nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm));
	DEBUG_PRINTLN();

	DEBUG_PRINTLN(F("Sleeping..."));
	delay(1000);
	sleepNow();
}

// Loops after setup()
void loop() {
	// sendMsg("Hello World!");
	if (interrupted == true) {
		DEBUG_PRINTLN(F("Woken!"));
		interrupted = false;
	}
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

// Called on an interrupt, wake from sleep
void isr() {
	sleep_disable(); // Stops a rare case of sleeping with not interrupt set
	detachInterrupt(0); // Already called, stop it being called repeatedly
	interrupted = true;
}

// Call this to sleep. Returns when woken
void sleepNow()
{
	// Maybe check out Narcoleptic library
	// https://github.com/brabl2/narcoleptic/blob/master/Narcoleptic.h
	// http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
	// The 5 different modes are:
	// 	SLEEP_MODE_IDLE		- least power savings
	// 	SLEEP_MODE_ADC
	// 	SLEEP_MODE_PWR_SAVE
	// 	SLEEP_MODE_STANDBY
	// 	SLEEP_MODE_PWR_DOWN	- most power savings
	// LOW trigger must be used if not using SLEEP_MODE_IDLE
	set_sleep_mode(SLEEP_MODE_IDLE);

	sleep_enable(); // Enables sleep bit in the MCUCR
	attachInterrupt(irqNum, isr, CHANGE); // LOW, CHANGE, RISING, FALLING

	power_adc_disable();
	power_spi_disable();
	power_twi_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	// power_all_disable();

	sleep_mode(); // Device is actually put to sleep here
	// AFTER WAKING UP - CONTINUES FROM HERE
	sleep_disable(); // Disables sleep bit in the MCUCR
	power_all_enable();
	// delay(2000); // Allow time to turn back on
}
