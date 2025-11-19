#include <msp430.h>
#include "i2c.h"

// I2C initialization with configurable clock frequency
void i2c_initialize(uint32_t smclk_freq, uint32_t i2c_freq) {
    // Configure I2C pins (P3.0 -> SDA, P3.1 -> SCL)
    P3SEL |= BIT0 | BIT1;
    
    // Put USCI_B0 in reset state for configuration
    UCB0CTL1 = UCSWRST;
    
    // Configure I2C master mode, I2C mode, synchronous
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;
    
    // Use SMCLK as clock source
    UCB0CTL1 |= UCSSEL_2;
    
    // Calculate and set clock prescaler
    uint32_t prescaler_value = smclk_freq / i2c_freq;
    UCB0BR0 = prescaler_value & 0xFF;
    UCB0BR1 = (prescaler_value >> 8) & 0xFF;
    
    // Release from reset to start I2C operation
    UCB0CTL1 &= ~UCSWRST;
}

// Write multiple bytes to I2C device
uint8_t i2c_send_data(uint8_t device_addr, const uint8_t *data_buffer, uint8_t data_length) {
    // Wait for any pending stop condition
    while (UCB0CTL1 & UCTXSTP);
    
    // Set target device address
    UCB0I2CSA = device_addr;
    
    // Configure as transmitter and generate start condition
    UCB0CTL1 |= UCTR | UCTXSTT;
    
    // Send all data bytes
    while (data_length--) {
        // Wait until TX buffer is ready
        while (!(UCB0IFG & UCTXIFG));
        
        // Send current data byte
        UCB0TXBUF = *data_buffer++;
    }
    
    // Wait for final transmission to complete
    while (!(UCB0IFG & UCTXIFG));
    
    // Generate stop condition
    UCB0CTL1 |= UCTXSTP;
    
    // Wait for stop condition to complete
    while (UCB0CTL1 & UCTXSTP);
    
    return 1; // Success
}

// Write single byte to I2C device
uint8_t i2c_send_byte(uint8_t device_addr, uint8_t data_byte) {
    return i2c_send_data(device_addr, &data_byte, 1);
}

// Read multiple bytes from I2C device
uint8_t i2c_receive_data(uint8_t device_addr, uint8_t *receive_buffer, uint8_t byte_count) {
    // Wait for any pending stop condition
    while (UCB0CTL1 & UCTXSTP);
    
    // Set target device address
    UCB0I2CSA = device_addr;
    
    // Configure as receiver and generate start condition
    UCB0CTL1 &= ~UCTR;
    UCB0CTL1 |= UCTXSTT;
    
    // Wait for start condition to complete
    while (UCB0CTL1 & UCTXSTT);
    
    // Single byte read requires immediate stop condition
    if (byte_count == 1) {
        UCB0CTL1 |= UCTXSTP;
    }
    
    // Receive all requested bytes
    for (uint8_t i = 0; i < byte_count; i++) {
        // Wait for received data
        while (!(UCB0IFG & UCRXIFG));
        
        // Store received byte
        receive_buffer[i] = UCB0RXBUF;
        
        // Send stop condition after receiving last byte
        if (i == (byte_count - 2)) {
            UCB0CTL1 |= UCTXSTP;
        }
    }
    
    return 1; // Success
}

// Read single byte from I2C device
uint8_t i2c_receive_byte(uint8_t device_addr) {
    uint8_t received_byte;
    i2c_receive_data(device_addr, &received_byte, 1);
    return received_byte;
}

// Check if I2C bus is ready
uint8_t i2c_bus_ready(void) {
    return !(UCB0CTL1 & (UCTXSTP | UCTXSTT));
}

// Generate I2C stop condition
void i2c_stop_condition(void) {
    UCB0CTL1 |= UCTXSTP;
    while (UCB0CTL1 & UCTXSTP);
}