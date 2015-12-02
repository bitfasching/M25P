/**
 * M25P16 Driver
 * Methods
 * 
 * (see header for information)
 */


// include header
#include <M25P.h>


// initialization
M25P::M25P( char selectPin )
{
    // remember pin
    this->selectPin = selectPin;

    // set I/O mode of slave select pin
    pinMode( selectPin, OUTPUT );

    // pull slave select pin up (idle state)
    digitalWrite( selectPin, HIGH );
}


// read device information (to use identify memory size)
void M25P::identify()
{
     // pull select pin down
    digitalWrite( this->selectPin, LOW );
    
    // initialize SPI
    SPI.begin();
    
    // apply SPI settings (up to 25 MHz, MSB first, shift out on rising edge)
    SPI.beginTransaction( SPISettings( 1, MSBFIRST, SPI_MODE0 ) );
    
    // send READ IDENTIFICATION command
    SPI.transfer( 0x9F );
    
    // reconfigure to shift in on falling edges
    SPI.beginTransaction( SPISettings( 1, MSBFIRST, SPI_MODE1 ) );

    // discard next byte (manufacturer ID)
    char manufacturer = SPI.transfer( 0x00 );

    // read memory information
    char memoryType = SPI.transfer( 0x00 );
    char memorySize = SPI.transfer( 0x00 );

    this->memorySize = memorySize;
    
    // release select pin
    digitalWrite( this->selectPin, HIGH );

    // release SPI
    SPI.end();
    
    Serial.println( "DEBUG: Memory Identification" );
    Serial.println( manufacturer, BIN );
    Serial.println( memoryType, BIN );
    Serial.println( memorySize, BIN );
}
