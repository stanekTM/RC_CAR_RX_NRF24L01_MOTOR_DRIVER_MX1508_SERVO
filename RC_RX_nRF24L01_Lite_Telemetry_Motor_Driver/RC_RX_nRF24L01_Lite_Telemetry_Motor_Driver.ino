
#include <RF24.h>         //https://github.com/nRF24/RF24
#include <nRF24L01.h>     //https://github.com/nRF24/RF24
#include <DigitalIO.h>    //https://github.com/greiman/DigitalIO
#include "PWMFrequency.h" //https://github.com/TheDIYGuy999/PWMFrequency

//free pins
//pin           2
//pin           4
//pin           5
//pin           6
//pin           7
//pin           8
//pin           12
//pin           A6
//pin           A7
 
//pwm pins for motor
#define pwm1    9
#define pwm2    10
#define pwm3    3
#define pwm4    11

//RX vcc, RF on/off RX LED
#define led     A4 

//input vcc analog telemetry
#define inRXvcc A5

//pins for nRF24L01
#define CE      A3 
#define CSN     A2 

//software SPI http://tmrh20.github.io/RF24/Arduino.html
//----- MOSI    15 //A1
//----- MISO    14 //A0
//----- SCK     13

RF24 radio(CE, CSN); //setup CE and CSN pins

const byte addresses[][6] = {"tx001", "rx002"};

//************************************************************************************************************************************************************************
//this structure defines the received data in bytes (structure size max. 32 bytes) ***************************************************************************************
//************************************************************************************************************************************************************************
struct rx_data
{
  byte ch1;
  byte ch2;
  byte ch3;
  byte ch4;
  byte ch5;
  byte ch6;
  byte ch7;
  byte ch8;
};
rx_data rc_data; //create a variable with the above structure

//************************************************************************************************************************************************************************
//this struct defines data, which are embedded inside the ACK payload ****************************************************************************************************
//************************************************************************************************************************************************************************
struct ackPayload
{
  float RXvcc; //analog telemetry
};
ackPayload payload;

//************************************************************************************************************************************************************************
//we will create variables with an initial integer ***********************************************************************************************************************
//************************************************************************************************************************************************************************
int motA_value = 0;
int motB_value = 0;

//************************************************************************************************************************************************************************
//reset values ​​(min = 0, mid = 127, max = 255) ***************************************************************************************************************************
//************************************************************************************************************************************************************************
void resetData()
{
  rc_data.ch7 = 127;
  rc_data.ch8 = 127;
}

//************************************************************************************************************************************************************************
//setup frequencies and motors control ***********************************************************************************************************************************
//************************************************************************************************************************************************************************
void outputPWM()
{  
/*
 * The base frequency for pins 3, 9, 10, 11 is 31250Hz.
 * The base frequency for pins 5, 6 is 62500Hz.
 * 
 * The divisors available on pins 5, 6, 9, 10 are: 1, 8, 64, 256, and 1024.
 * The divisors available on pins 3, 11       are: 1, 8, 32, 64, 128, 256, and 1024.
 * 
 * Pins 5, 6  are paired on timer0
 * Pins 9, 10 are paired on timer1, Servo library
 * Pins 3, 11 are paired on timer2, ServoTimer2 library
 *  
 * PWM frequency (default)  
 * D5  //pwm 976Hz, timer0, 8-bit
 * D6  //pwm 976Hz, timer0, 8-bit
 *
 * D9  //pwm 488Hz, timer1, 16-bit, Servo library
 * D10 //pwm 488Hz, timer1, 16-bit, Servo library     
 * 
 * D3  //pwm 488Hz, timer2, 8-bit, ServoTimer2 library
 * D11 //pwm 488Hz, timer2, 8-bit, ServoTimer2 library, SPI MOSI hardware
*/

//PWM frequency pin D9 or pin D10: 1024 = 30Hz, 256 = 122Hz, 64 = 488Hz(default), 8 = 3906Hz
//MotorA (pin D9 or pin D10, prescaler 8)  
  setPWMPrescaler(pwm1, 8);  

//PWM frequency pin D3 or pin D11:  128 = 244Hz, 64 = 488Hz(default), 32 = 976Hz, 8 = 3906Hz
//MotorB (pin D3 or pin D11, prescaler 8)  
  setPWMPrescaler(pwm3, 8);  

//MotorA ------------------------------------------------------------------------------------ 

  if (rc_data.ch7 < 120) // < 127, dead band of poor quality joysticks
  {
    motA_value = map(rc_data.ch7, 120, 0, 0, 255);
    analogWrite(pwm1, motA_value); 
    digitalWrite(pwm2, LOW);
  }
  else if (rc_data.ch7 > 127) // > 127, dead band of poor quality joysticks
  {
    motA_value = map(rc_data.ch7, 127, 255, 0, 255);
    analogWrite(pwm2, motA_value); 
    digitalWrite(pwm1, LOW);
  }
  else
  {
//    digitalWrite(pwm1, LOW); //"HIGH" brake, "LOW" no brake
//    digitalWrite(pwm2, LOW); //"HIGH" brake, "LOW" no brake
    analogWrite(pwm1, motA_value = 255); //adjustable brake (0-255)
    analogWrite(pwm2, motA_value = 255); //adjustable brake (0-255)
  }

//  Serial.println(motA_value); //print value ​​on a serial monitor
  
//MotorB ------------------------------------------------------------------------------------

  if (rc_data.ch8 < 120) // < 127, dead band of poor quality joysticks
  {
    motB_value = map(rc_data.ch8, 120, 0, 0, 255); 
    analogWrite(pwm3, motB_value); 
    digitalWrite(pwm4, LOW);
  }
  else if (rc_data.ch8 > 127) // > 127, dead band of poor quality joysticks
  {
    motB_value = map(rc_data.ch8, 127, 255, 0, 255); 
    analogWrite(pwm4, motB_value); 
    digitalWrite(pwm3, LOW);
  }
  else
  {
    digitalWrite(pwm3, HIGH); //"HIGH" brake, "LOW" no brake
    digitalWrite(pwm4, HIGH); //"HIGH" brake, "LOW" no brake
//    analogWrite(pwm3, motB_value = 127); //adjustable brake (0-255)
//    analogWrite(pwm4, motB_value = 127); //adjustable brake (0-255)
  }
}

//************************************************************************************************************************************************************************
//initial main settings **************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void setup()
{
  Serial.begin(9600);

  pinMode(pwm1, OUTPUT);
  pinMode(pwm2, OUTPUT);
  pinMode(pwm3, OUTPUT);
  pinMode(pwm4, OUTPUT);
  
  pinMode(led, OUTPUT);    //RX vcc, RF on/off RX LED
  pinMode(inRXvcc, INPUT); //input vcc analog telemetry
  
  resetData(); //reset each channel value

  //define the radio communication
  radio.begin();
  
  radio.setAutoAck(1);             //ensure autoACK is enabled (address 1, tx001)
  radio.enableAckPayload();        //enable custom ack payloads on the acknowledge packets
  radio.enableDynamicPayloads();   //enable dynamically-sized payloads
  radio.setRetries(5, 5);          //set the number and delay of retries on failed submit (max. 15 x 250us delay (blocking !), max. 15 retries)
  
  radio.setChannel(76);            //which RF channel to communicate on (0-125, default 76) 
  radio.setDataRate(RF24_250KBPS); //RF24_250KBPS (fails for units without +), RF24_1MBPS, RF24_2MBPS
  radio.setPALevel(RF24_PA_MIN);   //RF24_PA_MIN (-18dBm), RF24_PA_LOW (-12dBm), RF24_PA_HIGH (-6dbm), RF24_PA_MAX (0dBm) 

  radio.openWritingPipe(addresses[0]);    //(address 1, tx001) open a pipe for writing via byte array
  radio.openReadingPipe(1, addresses[1]); //(address 2, rx002) open all the required reading pipes, and then call "startListening"
                                          //which number pipe to open (0-5)
                                          //the 24, 32 or 40 bit address of the pipe to open
                                          
  radio.startListening(); //set the module as receiver. Start listening on the pipes opened for reading
}

//************************************************************************************************************************************************************************
//program loop ***********************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void loop()
{
  receive_time();
  send_and_receive_data();

  outputPWM();
  
  battery_voltage();
 
} //end program loop

//************************************************************************************************************************************************************************
//get time and reset data ************************************************************************************************************************************************
//************************************************************************************************************************************************************************
unsigned long lastReceiveTime = 0;

void receive_time()
{
  //check whether we keep receving data, or we have a connection between the two modules
  if(millis() >= lastReceiveTime + 1000) //1000 (1second)
  {
    resetData();        //if connection is lost, reset the data
    RFoff_indication(); 
  }
}

//************************************************************************************************************************************************************************
//send and receive data **************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void send_and_receive_data()
{
  byte pipeNo;
  
  if (radio.available(&pipeNo)) //check whether there is data to be received
  {
    radio.writeAckPayload(pipeNo, &payload, sizeof(ackPayload)); //prepare the ACK payload
   
    radio.read(&rc_data, sizeof(rx_data)); //read the radia data and send out the ACK payload
    lastReceiveTime = millis();            //at this moment we have received the data
    RFon_indication();                     
  } 
}

//************************************************************************************************************************************************************************
//analog telemetry with undervoltage detection ***************************************************************************************************************************
//************************************************************************************************************************************************************************
void battery_voltage()
{
  //------------------------------------ vcc ------------ monitored voltage
  payload.RXvcc = analogRead(inRXvcc) * (4.5 / 1023.0) <= 3.3; 
}

//************************************************************************************************************************************************************************
//after receiving the RF data, it activates the telemetry of the monitored voltage RX vcc by means of a flashing LED indication ******************************************
//************************************************************************************************************************************************************************
unsigned long ledTime = 0;
int ledState;

void RFon_indication()
{
  if (millis() >= ledTime + 500) //1000 (1second)
  {
    ledTime = millis();
    
    if (ledState >= !payload.RXvcc + HIGH)
    {
      ledState = LOW;
    }
    else
    {
      ledState = HIGH;
    }   
    digitalWrite(led, ledState);
      
//    digitalWrite(led, payload.RXvcc); //LED indication without flashing
  }
}

//************************************************************************************************************************************************************************
//when RX is switched on and TX is switched off, or after the loss of RF data, the LED activates flashing ****************************************************************
//************************************************************************************************************************************************************************
void RFoff_indication()
{
  if (millis() >= ledTime + 100) //1000 (1second)
  {
    ledTime = millis();
    
    if (ledState)
    {
      ledState = LOW;
    }
    else
    {
      ledState = HIGH;
    }   
    digitalWrite(led, ledState);
  }
}
   
