/**
 * M25P16 Driver
 * Methods
 *
 * (see header for information)
 */


// include header
#include <M25P.h>




/* :: High-Level Methods :: */


/**
 * Constructor
 * - sets up hardware SPI pins
 * - initializes slave select pin
 */
M25P::M25P( char selectPin )
{
    // set up hardware SPI pins
    SPI.begin();

    // set up FastPin instance for SPI slave select pin
    this->SelectPin.setup( selectPin, OUTPUT );

    // pull slave select pin up (idle state)
    SelectPin.write( HIGH );
}


/**
 * Read Data
 * - reads data bytes from given address onwards into a buffer
 * - address counter rolls over after upper end
 */
void M25P::readData( unsigned long startAddress, unsigned char targetBuffer[], unsigned long length )
{
    // start SPI
    this->beginSPI();

    // send READ DATA BYTES command
    SPI.transfer( M25P::Command::readData );

    // send start address
    this->transferAddress( startAddress );

    // read requested amount of bytes
    for ( unsigned long index = 0; index < length; index++ )
    {
        // read out next byte
        targetBuffer[ index ] = SPI.transfer( 0x00 );
    }

    // release SPI
    this->endSPI();
}


void M25P::longReadBegin( unsigned long startAddress )
{
  // wait for any ongoing write operation to finish
  this->waitToWrite();

  // start SPI
  this->beginSPI();

  // send READ DATA BYTES command
  SPI.transfer( M25P::Command::readData );

  // send start address
  this->transferAddress( startAddress );
}

unsigned char M25P::longReadGetByte()
{
  // read out next byte
  return SPI.transfer( 0x00 );
}

void M25P::longReadEnd()
{
  // release SPI
  this->endSPI();
}


/**
 * Write Data
 * - writes data bytes starting at given address
 * - bits can only be changed from 1 to 0 (erase must be run before!)
 * - memory is organized in pages of 256 bytes
 * - waits & iterates if given data spans across multiple pages
 */
void M25P::writeData( unsigned long startAddress, unsigned char data[], unsigned long length )
{
    // compute number of bytes to write in first page
    // (which is the page where the start address belongs to)
    int bytesUntilEndOfPage = M25P::pageSize - ( startAddress % M25P::pageSize );

    // write bytes up to the end of the first page (but don't write more than data available)
    this->programPage( startAddress, data, min( bytesUntilEndOfPage, length ) );

    // keep track of the amount of written bytes
    int written = bytesUntilEndOfPage;

    // if there are more data bytes than those previously written
    if ( length > written )
    {
        // set address counter to the beginning of the next page
        startAddress += written;

        // write remaining data as whole pages
        while ( length - written > M25P::pageSize )
        {
            // write page (indexing the data array at the number of bytes already written)
            this->programPage( startAddress, &data[ written ], M25P::pageSize );

            // increase address and byte count by page size
            startAddress += M25P::pageSize;
            written      += M25P::pageSize;
        }

        // write the last chunk of data to the last page
        this->programPage( startAddress, &data[ written ], length - written );
    }
}




/* :: Low-Level Methods :: */


/**
 * Begin SPI
 * - selects slave
 * - configures SPI
 * - SPI.begin() must have been called before (done in constructor)
 */
void M25P::beginSPI()
{
    // drive select pin low
    this->SelectPin.write( LOW );

    // apply SPI settings (up to 20 MHz, MSB first, shift out on rising edge)
    SPI.beginTransaction( SPISettings( 20000000, MSBFIRST, SPI_MODE0 ) );
}


/**
 * End SPI
 * - deselects slave
 * - releases SPI
 */
void M25P::endSPI()
{
    // reset select pin to idle state
    this->SelectPin.write( HIGH );

    // release SPI bus
    SPI.end();
}



/**
 * Is Writing?
 * - queries write in progress (WIP) status bit
 */
bool M25P::isWriting()
{
    // start SPI
    this->beginSPI();

    // send READ STATUS command
    SPI.transfer( M25P::Command::readStatus );

    // shift in 8 status bits
    unsigned char statusBits = SPI.transfer( 0x00 );

    // release SPI
    this->endSPI();

    // return WIP status bit
    return bitRead( statusBits, 0 );
}


/**
 * Wait To Write
 * - busy waiting (blocking)
 * - polls write in progress (WIP) status bit every 100 µs
 */
void M25P::waitToWrite()
{
    // poll WIP status bit
    while ( this->isWriting() );
}


/**
 * Enable Write
 * - waits for ongoing write operations
 * - sets write enable (WEL) latch bit
 * - must be run before every program, erase or write command!
 */
void M25P::enableWrite()
{
    // wait for any ongoing write operation to finish
    this->waitToWrite();

    // start SPI
    this->beginSPI();

    // send WRITE ENABLE command
    SPI.transfer( M25P::Command::writeEnable );

    // release SPI
    this->endSPI();

    // wait until register has been updated
    //this->waitToWrite();
}


/**
 * Transfer Address
 * - shifts out a 3-byte memory address over SPI
 * - does not touch SPI setup
 */
void M25P::transferAddress( unsigned long address )
{
    // transfer each address byte, from MSB to LSB
    //SPI.transfer( address >> (2*8) );
    //SPI.transfer( address >> (1*8) );
    SPI.transfer( (unsigned char)(( address >> (2*8) ) & 0xFF ) );
    SPI.transfer( (unsigned char)(( address >> (1*8) ) & 0xFF ) );
    SPI.transfer( (unsigned char)(( address >> (0*8) ) & 0xFF ) );
}


/**
 * Program Page
 * - writes data bytes to one page
 * - bytes exceeding the page overwrite the beginning of the current page
 * - bits can only be changed from 1 to 0 (erase must be run before!)
 */
void M25P::programPage( unsigned long startAddress, unsigned char data[], int length )
{
    // abort if there is no data to write
    if ( length <= 0 ) { return; }

    // make memory writable
    this->enableWrite();

    // start SPI
    this->beginSPI();

    // send PAGE PROGRAM command
    SPI.transfer( M25P::Command::pageProgram );

    // send start address
    this->transferAddress( startAddress );

    // fill current page
    for ( int index = 0; index < length; index++ )
    {
        // shift out next byte
        SPI.transfer( data[ index ] );
    }

    // release SPI
    this->endSPI();
}


/**
 * Erase Sector By Address
 * - erases a sector specified by an address within this sector
 * - erasing sets all bits from 0 to 1
 */
void M25P::eraseSector( unsigned long addressWithinSector )
{
    // make memory writable
    this->enableWrite();

    // start SPI
    this->beginSPI();

    // send SECTOR ERASE command
    SPI.transfer( M25P::Command::sectorErase );

    // send address within the sector to erase
    this->transferAddress( addressWithinSector );

    // release SPI
    this->endSPI();
}


/**
 * Erase All
 * - performs a bulk erase
 * - erasing sets all bits from 0 to 1
 */
void M25P::eraseAll()
{
    // make memory writable
    this->enableWrite();

    // start SPI
    this->beginSPI();

    // send BULK ERASE command
    SPI.transfer( M25P::Command::bulkErase );

    // release SPI
    this->endSPI();
}
