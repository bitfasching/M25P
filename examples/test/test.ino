#include <M25P.h>

M25P Memory( 6 );

const int length = 218;
const unsigned char testByte = 0x33;
unsigned char bytes[length];
unsigned long checkSum = 0;

unsigned long address, time;

void setup()
{
    delay( 1000 );
    Serial.begin( 115200 );
    Serial.println( F("M25P Memory Test") );
    Serial.print( F("Data chunk size: ") ); Serial.print( length ); Serial.println( " Bytes" );
    delay( 1000 );
    
    for ( int i = 0; i < length; i++ )
    {
        bytes[i] = testByte;
        checkSum += testByte;
    }
    
    address = 0;
}

void loop()
{
    Serial.print( "Sector: " );
    Serial.println( address / 65536UL );
    Serial.print( "- erasing..." );
    
    time = micros();
    Memory.eraseSector( address );
    Memory.waitToWrite();
    time = micros() - time;
    
    Serial.print( " (" ); Serial.print( (float)time/1000/1000, 2 ); Serial.println( " s)" );
    delay(100);
    
    for ( unsigned long offset = 0; offset < 65536UL; offset += length )
    {
        Serial.print( "- writing page: " ); Serial.print( (address+offset)/256 );
        Serial.print( " address: " ); Serial.print( address+offset );
        
        time = micros();
        Memory.writeData( address+offset, bytes, length );
        Memory.waitToWrite();
        time = micros() - time;
        
        Serial.print( " (" ); Serial.print( (float)time/1000, 2 ); Serial.println( " ms)" );
    }
    delay(100);
        
    for ( unsigned long offset = 0; offset < 65536UL; offset += length )
    {
        Serial.print( "- reading page: " ); Serial.print( (address+offset)/256 );
        Serial.print( " address: " ); Serial.print( address+offset );
        
        time = micros();
        Memory.readData( address+offset, bytes, length );
        time = micros() - time;
        
        unsigned long sum = 0;
        for ( int i = 0; i < length; i++ ) sum += bytes[i];
        if ( sum == checkSum )
        {
            Serial.print( " OK! " );
        }
        else
        {
            Serial.print( " ERROR: " );
            for ( int i = 0; i < length; i++ ) { Serial.print( bytes[i], HEX ); Serial.print( "h " ); }
        }
        
        Serial.print( "(" ); Serial.print( (float)time/1000, 2 ); Serial.print( " ms)" );
        Serial.println();
    }
    delay( 100 );
    
    address += 65536UL;
    //if ( address >= 65536UL*31 ) address = 0;
}
