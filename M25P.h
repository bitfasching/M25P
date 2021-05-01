/**
 * M25P Driver
 *
 * Arduino Library
 * for M25P Flash Memories
 * up to 128 Mbit (16 MB)
 *
 * License: BSD 3-Clause
 *
 * Nick Schwarzenberg,
 * 12/2015, v0.2.0
 */


// include these dependencies also in your top-level .ino
#include <Arduino.h>
#include <FastDigitalPin.h> // https://github.com/bitfasching/FastDigitalPin
#include <SPI.h>

// prevent redefinitions
#ifndef M25P_driver
#define M25P_driver


class M25P
{
    public:

        // constructor
        M25P( char selectPin=10 );

        // memory page size in bytes
        static const int pageSize = 256;

        // command words
        static struct Command {
            static const unsigned char
            writeEnable = 0x06,
            readIdentification = 0x9F,
            readStatus = 0x05,
            readData = 0x03,
            pageProgram = 0x02,
            sectorErase = 0xD8,
            bulkErase = 0xC7;
        } Command;

        // FastDigitalPin instance for SPI slave select pin
        FastDigitalPin SelectPin;

        // SPI setup methods
        void beginSPI();
        void endSPI();

        // low-level methods
        bool isWriting();
        void waitToWrite();
        void enableWrite();
        void transferAddress( unsigned long address );
        void programPage( unsigned long startAddress, unsigned char data[], int length );
        void eraseSector( unsigned long addressWithinSector );
        void eraseAll();

        // intermediate methods
        void longReadBegin( unsigned long startAddress );
        unsigned char longReadGetByte();
        void longReadEnd();

        // high-level methods
        void readData( unsigned long startAddress, unsigned char targetBuffer[], unsigned long length );
        void writeData( unsigned long startAddress, unsigned char data[], unsigned long length );
};


// see ifndef above
#endif
