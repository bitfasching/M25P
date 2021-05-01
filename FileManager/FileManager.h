/**
 * File Manager
 *
 * Nick Schwarzenberg
 * 2015
 */

#ifndef FileManager_h
#define FileManager_h

#include <M25P.h>

// message for host indicating ready to write
#define FileManager_readyMessage "READY"


class FileManager
{
    public:

        // memory capacity in bytes
        static const auto capacity = 2097152;

        // metadata format
        static const auto metaSizeIndex = 0;
        static const auto metaLength = 3;

        // pointer to driver object
        M25P* Memory;

        // current memory address
        unsigned long address;

        // stop address after end of file
        unsigned long endOfFile;

        // constructor
        FileManager( M25P* Memory ) : Memory(Memory), address(0), endOfFile(0) {};

        // download/upload file from/to host
        void download();
        void upload();

        // read/write data chunk from/to file on memory
        void seek( unsigned long position );
        char read();
        void longReadBegin();
        unsigned char longReadGetByte();
        void longReadEnd();
        int readBytes( char targetBuffer[], int requestedLength );
        void writeBytes( char sourceBuffer[], int sourceLength );

        // finish file (write metadata)
        void finish();

        // rewind memory address counter to beginning
        void rewind();

        // erase complete memory
        void erase();

        // metadata methods
        void readMetadata( unsigned char targetBuffer[ FileManager::metaLength ] );
        void writeMetadata( unsigned char sourceBuffer[ FileManager::metaLength ] );
        void saveMetadata( unsigned long fileSize );
        unsigned long getFileSize();
        unsigned long getPosition();

        // discover stored file by reading the metadata
        void discover();

    private:

        // start address for storing the file (next beginning of a page after metadata)
        static const auto fileStartAddress = M25P::pageSize * ( FileManager::metaLength / M25P::pageSize + 1 );

        // data chunk size in bytes for download & upload
        static const auto hostTransferChunkSize = M25P::pageSize;

        // timeout for downloading data [ms]
        static const auto downloadTimeoutMillis = 2000;

        // internal buffer for reading single bytes
        char singleByteBuffer[1];
};




/* :: Reading :: */


void FileManager::seek( unsigned long position )
{
    // seek to a given byte offset in file
    this->address = FileManager::fileStartAddress + position;
}


void          FileManager::longReadBegin()   { Memory->longReadBegin( this->address ); }
unsigned char FileManager::longReadGetByte() { this->address++; return Memory->longReadGetByte(); }
void          FileManager::longReadEnd()     { Memory->longReadEnd(); }


int FileManager::readBytes( char targetBuffer[], int requestedLength )
{
    // don't read beyond end of file
    requestedLength = min( requestedLength, this->endOfFile - this->address );

    // read bytes from memory
    Memory->readData( this->address, (unsigned char*) targetBuffer, requestedLength );

    // increase address by number of read bytes
    this->address += requestedLength;

    // return number of read bytes
    return requestedLength;
}


char FileManager::read()
{
    // read and return byte
    this->readBytes( singleByteBuffer, 1 );
    return singleByteBuffer[0];
}




/* :: Writing :: */


void FileManager::writeBytes( char sourceBuffer[], int sourceLength )
{
    // write bytes to memory (doesn't wait to finish)
    Memory->writeData( this->address, (unsigned char*) sourceBuffer, sourceLength );

    // increase address by number of written bytes
    this->address += sourceLength;
}


void FileManager::upload()
{
    // data transfer buffer
    unsigned char buffer[ FileManager::hostTransferChunkSize ];

    // check out stored file (to find end of file)
    this->discover();

    // reset address counter
    this->rewind();

    // no check here, loop ends with a break
    while ( true )
    {
        // read a full chunk of data, but not more than until the end of file
        int length = min( sizeof(buffer), this->endOfFile - this->address );

        // read data
        Memory->readData( this->address, buffer, length );

        // increase address counter
        this->address += length;

        // forward to host immediately
        // (no waiting needed, the host is definitely faster)
        Serial.write( buffer, length );

        // read less than a full chunk?
        if ( length < sizeof(buffer) )
        {
            // nothing more to read
            break;
        }
    }
}




/* :: Download & Upload :: */


void FileManager::download()
{
    // erase memory (implicitly resets address)
    this->erase();

    // data transfer buffer
    unsigned char buffer[ FileManager::hostTransferChunkSize ];

    // set timeout for serial interface to host
    Serial.setTimeout( FileManager::downloadTimeoutMillis );

    // no check here, loop ends with a break
    while ( true )
    {
        // tell host to send next data chunk
        Serial.println( FileManager_readyMessage );

        // try to receive data from host
        int length = Serial.readBytes( buffer, sizeof(buffer) );

        // received something?
        if ( length > 0 )
        {
            // about to write over the memory's capacity?
            if ( this->address + length >= FileManager::capacity )
            {
                // truncate the data to write (-1 because address starts at zero)
                length = FileManager::capacity - this->address - 1;
            }

            // write to memory and wait until done
            Memory->writeData( this->address, buffer, length );
            Memory->waitToWrite();

            // increase address counter
            this->address += length;
        }
        else
        {
            // nothing received, assume that's all
            break;
        }
    }

    // save file size
    this->finish();
}


void FileManager::finish()
{
    // mark current write address as end of file
    this->endOfFile = this->address;

    // save file size as metadata on memory
    this->saveMetadata( this->address - FileManager::fileStartAddress );
}


void FileManager::rewind()
{
    // reset address counter to beginning of file
    this->address = FileManager::fileStartAddress;
}


void FileManager::erase()
{
    // perform bulk erase
    Memory->eraseAll();

    // wait to finish
    Memory->waitToWrite();

    // reset end-of-file mark
    this->endOfFile = 0;

    // reset address counter
    this->rewind();
}




/* :: Metadata :: */


/**
 * Read/Save Metadata
 * - retrieves/stores metadata from/in reserved page on memory
 */
void FileManager::readMetadata( unsigned char targetBuffer[ FileManager::metaLength ] )
{
    // read file information from reserved memory page
    Memory->readData( 0, targetBuffer, FileManager::metaLength );
}
void FileManager::writeMetadata( unsigned char sourceBuffer[ FileManager::metaLength ] )
{
    // write file information to reserved memory page
    Memory->writeData( 0, sourceBuffer, FileManager::metaLength );
    Memory->waitToWrite();
}


/**
 * Save Metadata
 * - saves information about stored file as metadata in memory
 */
void FileManager::saveMetadata( unsigned long fileSize )
{
    // buffer for metadata
    unsigned char metadata[ FileManager::metaLength ];

    // split 24-bit file size on three bytes, MSB first
    metadata[ FileManager::metaSizeIndex + 0 ] = (unsigned char)( fileSize >> 16 );
    metadata[ FileManager::metaSizeIndex + 1 ] = (unsigned char)( fileSize >> 8 );
    metadata[ FileManager::metaSizeIndex + 2 ] = (unsigned char)( fileSize >> 0 );

    // write metadata to memory
    this->writeMetadata( metadata );
}


/**
 * Get File Size
 * - retrieves size of stored file from metadata
 */
unsigned long FileManager::getFileSize()
{
    // read metadata from memory
    unsigned char metadata[ FileManager::metaLength ];
    Memory->readData( 0, metadata, sizeof(metadata) );

    // add bytes to integer, MSB first
    unsigned long fileSize = 0;
    fileSize += (unsigned long) metadata[0] << 16;
    fileSize += (unsigned long) metadata[1] << 8;
    fileSize += (unsigned long) metadata[2] << 0;

    // all bits equal to 1? (memory empty?)
    if ( fileSize == 0xFFFFFF )
    {
        // no file size information found
        return 0;
    }
    else
    {
        // seems valid, return file size
        return fileSize;
    }
}


unsigned long FileManager::getPosition()
{
    return this->address - FileManager::fileStartAddress;
}


/**
 * Discover
 * - "discovers" the stored file
 * - configures FileManager according to metadata
 * - must be run before .read()
 */
void FileManager::discover()
{
    // determine end of file
    this->endOfFile = FileManager::fileStartAddress + this->getFileSize();
}

#endif // see #ifndef on top
