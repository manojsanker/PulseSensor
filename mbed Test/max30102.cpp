*
 Library for the Maxim MAX30102 pulse oximetry system
 */

#include "MAX30102.h"
#include "functions.h"

static Serial pc(SERIAL_TX, SERIAL_RX);

void MAX30102::setLEDs(pulseWidth pw, ledCurrent red, ledCurrent ir){
    char data_write[1];
    data_write[0] = pw | 0x40;
    i2c_write(MAX30102_ADDRESS, MAX30102_SPO2_CONFIG, data_write, 1);     // Mask LED_PW
    data_write[0] = (red<<4);
    i2c_write(MAX30102_ADDRESS, MAX30102_LED_CONFIG_1, data_write, 1); // write LED configs
    data_write[0] = (ir<<4);
    i2c_write(MAX30102_ADDRESS, MAX30102_LED_CONFIG_2, data_write, 1); // write LED configs
}

void MAX30102::setFIFO(sampleAvg avg){
    char data_write[1];
    data_write[0] = 0x40;    // enable data ready interrupt
    i2c_write(MAX30102_ADDRESS, MAX30102_INT_ENABLE, data_write, 1);
    data_write[0] = (avg<<5) | 0x10;    // set data averaging
    i2c_write(MAX30102_ADDRESS, MAX30102_FIFO_CONFIG, data_write, 1);
}

void MAX30102::setSPO2(sampleRate sr, high_resolution hi_res){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_SPO2_CONFIG, data_read, 1);
    data_read[0] = data_read[0] & 0xA3; // Set SPO2_SR to 000 and SPO2_HI_RES_EN to 0
    data_write[0] = data_read[0] | (sr<<2) | (hi_res<<6);
    i2c_write(MAX30102_ADDRESS, MAX30102_SPO2_CONFIG, data_write, 1); // Mask SPO2_SR
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);
    data_write[0] = data_read[0] & 0xF8; // Set Mode to 000
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1); // Mask MODE
}

void MAX30102::setInterruptSPO2(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_INT_ENABLE, data_read, 1);
    data_write[0] = data_read[0] | 0x10; // Set Interrupt enable for SPO2
    i2c_write(MAX30102_ADDRESS, MAX30102_INT_ENABLE, data_write, 1); // Mask ENB_SPO2_RDY
}

int MAX30102::getNumSamp(void){
    char data_read[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_W_POINTER, data_read, 1);
    char wrPtr = data_read[0];
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_R_POINTER, data_read, 1);
    char rdPtr = data_read[0];
    return (abs( 16 + (int)wrPtr - (int)rdPtr ) % 16);
}

int MAX30102::getDataRdyStatus(void){
    char data_read[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_INT_STATUS, data_read, 1);
    data_read[0] = data_read[0] & 0x40;   // Only look at data ready status
    return (data_read[0] >> 6);
}

void MAX30102::setTemp(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);
    data_write[0] = data_read[0] | 0x0B;    // Set SPO2 Mode and enable temperature
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1); // Mask MODE
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);
}

void MAX30102::setSPO2mode(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);
    data_write[0] = data_read[0] | 0x03;    // Set SPO2 Mode
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1);
}

void MAX30102::setHRmode(void){
    char data_write[1];
    data_write[0] = 0x02;    // Set HR Mode
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1);
}

int MAX30102::readTemp(void){
    char data_read[1];
    char temp_int, temp_fract;
    int temp_measured;
    i2c_read(MAX30102_ADDRESS, MAX30102_TEMP_INTEGER, data_read, 1);
    temp_int = data_read[0];
    i2c_read(MAX30102_ADDRESS, MAX30102_TEMP_FRACTION, data_read, 1);
    temp_fract = data_read[0] & 0x0F;
    temp_measured = ((int)temp_int)+(((int)temp_fract) >> 4);
    return temp_measured;
}

void MAX30102::readSensor(void){
    char data_read[3];
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_DATA_REG, data_read, 3);  // Read three times from the FIFO
    HR = (data_read[0]<<16) | (data_read[1]<<8) | data_read[2];    // Combine values to get the actual number
    //SPO2 = (data_read[2]<<8) | data_read[3];  // Combine values to get the actual number
}

void MAX30102::shutdown(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);  // Get the current register
    data_write[0] = data_read[0] | 0x80;
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1);   // mask the SHDN bit
}

void MAX30102::reset(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);  // Get the current register
    data_write[0] = data_read[0] | 0x40;
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1);   // mask the RESET bit
}

void MAX30102::startup(void){
    char data_read[1];
    char data_write[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);  // Get the current register
    data_write[0] = data_read[0] & 0x7F;
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1);   // mask the SHDN bit
}

char MAX30102::getRevID(void){
    char data_read[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_REVISION_ID, data_read, 1);
    return data_read[0];
}

char MAX30102::getPartID(void){
    char data_read[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_PART_ID, data_read, 1);
    return data_read[0];
}

void MAX30102::begin(pulseWidth pw, ledCurrent ir, sampleRate sr){
    char data_write[1];
    data_write[0] = 0x03;
    i2c_write(MAX30102_ADDRESS, MAX30102_CONFIG, data_write, 1); // Heart rate only
    data_write[0] = ir;
    i2c_write(MAX30102_ADDRESS, MAX30102_LED_CONFIG_2, data_write, 1);
    data_write[0] = ((sr<<2)|pw);
    i2c_write(MAX30102_ADDRESS, MAX30102_SPO2_CONFIG, data_write, 1);
}

void MAX30102::init(pulseWidth pw, sampleRate sr, high_resolution hi_res, ledCurrent red, ledCurrent ir){
    
    setLEDs(pw, red, ir);
    setSPO2(sr, hi_res);
    
}

void MAX30102::printRegisters(void){
    char data_read[1];
    i2c_read(MAX30102_ADDRESS, MAX30102_INT_STATUS, data_read, 1);
    pc.printf("INT_STATUS: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_INT_ENABLE, data_read, 1);
    pc.printf("INT_ENABLE: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_W_POINTER, data_read, 1);
    pc.printf("FIFO_W_POINTER: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_OVR_COUNTER, data_read, 1);
    pc.printf("OVR_COUNTER: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_R_POINTER, data_read, 1);
    pc.printf("FIFO_R_POINTER: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_DATA_REG, data_read, 1);
    pc.printf("FIFO_DATA_REG: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_FIFO_CONFIG, data_read, 1);
    pc.printf("FIFO_CONFIG: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_CONFIG, data_read, 1);
    pc.printf("CONFIG: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_SPO2_CONFIG, data_read, 1);
    pc.printf("SPO2_CONFIG: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_LED_CONFIG_1, data_read, 1);
    pc.printf("LED_CONFIG_1: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_LED_CONFIG_2, data_read, 1);
    pc.printf("LED_CONFIG_2: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_TEMP_INTEGER, data_read, 1);
    pc.printf("TEMP_INTEGER: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_TEMP_FRACTION, data_read, 1);
    pc.printf("TEMP_FRACTION: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_REVISION_ID, data_read, 1);
    pc.printf("REVISION_ID: %#4X\r\n", data_read[0]);
    i2c_read(MAX30102_ADDRESS, MAX30102_PART_ID, data_read, 1);
    pc.printf("PART_ID: %#4X\r\n", data_read[0]);
}