/**
 * M25P Driver
 * 
 * Arduino Library
 * for M25P Flash Memory
 * 
 * License: BSD 3-Clause
 * 
 * Nick Schwarzenberg,
 * 11/2015, v0.1.0α UNRELEASED
 */


// include these dependencies also in your top-level .ino
#include <Arduino.h>
#include <SPI.h>

// prevent redefinitions
#ifndef M25P_driver
#define M25P_driver


class M25P
{
    private:
        
        // select pin (not defined by SPI library)
        char selectPin;
        
        // memory size
        char memorySize;
        
        // SPI functions
        void beginSPI();
        void endSPI();
    
    public:
        
        M25P(
            
            // SPI slave select pin (pulled down by Arduino during transfers)
            char selectPin=10
            
        );
        
        // read device information (to use identify memory size)
        void identify();
};


// see ifndef above
#endif
