#include "pins_arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <RTC8564.h>

#define LDAC 9
#define sspin 10

int h;
int m;
int s;
int s_previous;
int mode;
float pid;
float e_integral;
int trace[120];

void setup(){
	Serial.begin(9600);
	Rtc.begin();
	int i;
	for(i = 2; i <= 6; i++){
		pinMode(i, OUTPUT);
	}
	pinMode(A0, INPUT);
	pinMode(LDAC, OUTPUT);
	pinMode(sspin, OUTPUT);
	digitalWrite(sspin, HIGH);
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	SPI.setDataMode(SPI_MODE0) ;	
/*
	byte date_and_time[7]; 
	date_and_time[0] = 0x00;  // 秒 
	date_and_time[1] = 0x09;  // 分 
	date_and_time[2] = 0x20;  // 時 
	date_and_time[3] = 0x05;  // 日 
	date_and_time[4] = 0x03;  // 曜日 
	date_and_time[5] = 0x08;  // 月 
	date_and_time[6] = 0x14;  // 年
	Rtc.sync(date_and_time);
*/
	initialize();
}

void loop(){
	int i;
	int loopcount;
	int A0inputvalue;
	int A1inputvalue;
	int zrhvalue;
	int pidvalue;
	A0inputvalue = analogRead(A0);
	zrhvalue = map(A0inputvalue, 0, 1023, 0, 1015);	// Modified CO2 conc
/*	
	Rtc.available();
	h = Rtc.hours(RTC8564::Decimal);
	m = Rtc.minutes(RTC8564::Decimal);
	s = Rtc.seconds(RTC8564::Decimal);
*/		
	if(Serial.available() == 3){
		h = Serial.read();
		m = Serial.read();
		s = Serial.read();
		Serial.flush();
		mode = getmode(m);
		for(i = 0; i < 5; i++){
			if(i == mode){
				digitalWrite(i + 2, HIGH);
			}else{
				digitalWrite(i + 2, LOW);
			}
		}
		if(mode == 0){
			pid = pid + getpid(s, zrhvalue);
		}
		Serial.write(mode);
		Serial.write(zrhvalue >> 8);	// High byte
		Serial.write(zrhvalue & 0xff);	// Low byte
	}	

	/*
	if(s != s_previous){
		pid = pid + getpid(s, zrhvalue);
		Serial.print("pid: ");
		Serial.println(pid);
	}
	*/
	pidvalue = pid * 4095 / 10;
	digitalWrite(LDAC,HIGH); // close latch
	digitalWrite(sspin, LOW);  // LOW signal to an interested SS which operates machine #i
	SPI.transfer(((pidvalue >> 8) & 0x0f) | 0x30) ; // High byte
	SPI.transfer(pidvalue & 0xff) ;        // Low byte
	digitalWrite(sspin, HIGH) ;     // signal meaning writing data ended
	digitalWrite(LDAC,LOW) ;        // open latch	
	s_previous = s;
}

void initialize(){
	int i;
	h = 99;
	m = 99;
	s = 99;
	s_previous = 99;
	mode = 99;
	pid = 3.00;
	for(i = 0; i < 5; i++){
		digitalWrite(i + 2, LOW);
	}
}

float getpid(int _s, int _zrhvalue){
	int i;
	float p_parameter = 0.0002;
	float i_parameter = 1000;	// Ti
	float d_parameter = 0.001;	// Td
	int target = 400;
	float dpid;
	int e;
	float e_dif;
	e = target - _zrhvalue;
	trace[_s] = _zrhvalue;
	if(_s == 0){
		e_integral = 0;
		e_dif = 0;
		for(i = 0; i < 120; i++){
			trace[i] = 0;
		}
		dpid = 0;
	}else if((_s >= 30) && (_s <= 90)){
		e_integral = e_integral + e;
		e_dif = (target - trace[_s]) - (target - trace[_s - 1]);
		dpid = p_parameter * (e + 1 / i_parameter * e_integral + d_parameter * e_dif);
	}else{
		dpid = 0;
	}
	/*
	if(s != s_previous){
		Serial.print("p_parameter");
		Serial.print(p_parameter);
		Serial.print(", ");		
		Serial.print("e: ");
		Serial.print(e);
		Serial.print(", ");
		Serial.print("dpid: ");
		Serial.println(dpid);
	}
	*/
	return dpid;
}

int getmode(int _m){
	int quotient;
	int remain;
	int cycle;
	int nmode;
	int modecycle;
	cycle = 6;	// [min]
	nmode = 3;	// 0, 1, 3
	modecycle = cycle / nmode;	// [min]
//	quotient = (_m % 20) / 5;
//	remain = (_m % 20) % 5;
	quotient = (_m % cycle) / modecycle;
//	if(remain < modecycle){
	switch (quotient) {
	    case 0:
	    	return 0;
	    case 1:
	    	return 1;
    	case 2:
    		return 3;
/*
	}else{
//		return 1;
		switch(quotient){
			case 0:
				return 1;
			case 1:
				return 2;
			case 2:
				return 3;
			case 3:
				return 4;
*/
			default:
//				Serial.println("Error in getactivepin() switch");
				break;
//		}

	}
}
