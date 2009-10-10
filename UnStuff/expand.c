 /*************************************************************************
 *
 *  UnStuff
 *  © 1990,1992  Peter Heinrich
 *
 *  expand.c
 *
 *  The work-horse module of the program, this file contains all of the
 *  decompression code.  StuffIt uses four methods of file compression:
 *  none, run-length encoding, Lempel-Ziv-Welch, and Huffman encoding.
 *  All four methods are handled here.
 *
 * $Log$
 *************************************************************************/



#include <stdio.h>
#include <stdlib.h>

#include "unstuff.h"
#include "checksum.h"
#include "expand.h"
#include "file.h"

#ifdef __MSDOS__
#pragma warn -pia
#endif



static FILE *in, *out;
static ulong before, after;

/*  Lempel-Ziv-Welch variables  */
static LZWCode lzwNext, lzwNew, *lzwString;
static ulong lzwBuffer, lzwNum;
static ubyte lzwSize, lzwBitCount;
static char *lzwChar;
static bool lzwDone;
static const uword lzwMask[ LZW_MAX_SIZE - LZW_MIN_SIZE + 1 ] =
   {
      0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff
   };

/*  Huffman variables  */
static HuffNode *nList, *nPtr;
static ubyte huffBitCount;



static CheckSum decompress_none( void )
/*
**  This function "decompresses" the file when no compression scheme is used.
**  This routine is also called if a raw dump is requested (-w on the command
**  line).  We'll force both forks to use this function, assuring that they
**  will be extracted unaltered.
*/
{
   CheckSum crc = INIT_CRC;
   ubyte *buffer;

   /*  Allocate an input buffer  */
   if( buffer = (ubyte *)malloc( min( before, MAX_BUFFER_SIZE ) ) )
   {
      ulong num;

      while( before )
      {
         /*  Read as many bytes as possible  */
         num = min( before, MAX_BUFFER_SIZE );
         fread( buffer, sizeof( ubyte ), num, in );

         /*
         **  Update the CRC.  If we're performing a raw dump, this
         **  value won't be used, but it's not a big deal to calculate
         **  it anyway.
         */
         crc = calc_crc( buffer, num, crc );
         if( NOT test() )
            fwrite( buffer, sizeof( ubyte ), num, out );

         before -= num;
      }
      free( buffer );
   }
   else
      clean_exit( "Out of memory" );

   return( crc );
}



static CheckSum decompress_rle( void )
/*
**  This routine decompresses a file that is run-length encoded.
*/
{
   CheckSum crc = INIT_CRC;
   ubyte *buffer;

   /*
   **  Get our output buffer.  Unlike non-compressed files, we'll only
   **  read one byte at a time from RLE ones.  Most of the time we'll
   **  write only one byte, as well.  This could definitely be optimized,
   **  but I don't much feel like it.  It would just clutter up the code.
   */
   if( buffer = (ubyte *)malloc( (MAX_UBYTE_VALUE + 1)/2 ) )
   {
      ubyte ch, last = 0;
      byte num;

      while( before )
      {
         fread( &ch, sizeof( ubyte ), 1, in );
         before--;

         /*
         **  If we read the RLE_REPEAT_BYTE value, then we're supposed to
         **  repeat the character we just read.  The next byte in the
         **  stream tells us how many times.  Note that this count is a
         **  SIGNED number (don't ask me why).
         */
         if( RLE_REPEAT_BYTE == ch )
         {
            fread( &num, sizeof( byte ), 1, in );
            num--;
            before--;
         }
         else
            num = 1;

         /*
         **  If repeating a byte, copy as many as necessary into the
         **  buffer.  If the repeat count is less than one (including
         **  if it's negative), throw it out and assume a repeat count
         **  of 1.  Yeah, THAT makes sense.
         */
         if( num > 1 )
         {
            ubyte i;

            for( i = 0; i < num; i++ )
               *(buffer + i) = last;
         }
         else
         {
            *buffer = last = ch;
            num = 1;
         }

         /*  Update the CRC  */
         crc = calc_crc( buffer, num, crc );
         if( NOT test() )
            fwrite( buffer, sizeof( byte ), num, out );
      }
      free( buffer );
   }
   else
      clean_exit( "Out of memory" );

   return( crc );
}



static void get_lzw_code( void )
/*
**  Returns the next code from the LZW-compressed input stream.  This imple-
**  mentation of LZW uses variable length codes (9-14 bits).
*/
{
   /*
   **  Fill a small buffer (a single long value) with bytes from the
   **  input stream, adding more to the high end as its data gets shifted
   **  down with use.
   */
   while( lzwBitCount <= 24 )
   {
      /*
      **  If we've read as many bytes as specified in the length field
      **  of the file header, shift zeros into the high end of the long
      **  value acting as our buffer.  When the whole thing is zero,
      **  we're all done with the file.
      */
      if( lzwNum++ < before )
         lzwBuffer |= ((ulong)getc( in ) << lzwBitCount);
      else
         lzwDone = ((lzwBuffer >> lzwSize) == 0L);
      lzwBitCount += 8;
   }

   /*
   **  Mask off all bits above the ones we want (lzwSize keeps track of
   **  how many we grab each time), and shift the whole buffer down in
   **  preparation for the next time we call this routine.
   */
   lzwNew = lzwBuffer & lzwMask[ lzwSize - LZW_MIN_SIZE ];
   lzwBuffer >>= lzwSize;
   lzwBitCount -= lzwSize;

   /*
   **  If we just read in a LZW_CLEAR_TABLE code, clear the table (reset
   **  the code size and first code) and re-read the code using the new
   **  size.
   */
   if( LZW_CLEAR_TABLE == lzwNew )
   {
      lzwSize = LZW_MIN_SIZE;
      lzwNext = LZW_FIRST_CODE;
      get_lzw_code();
   }
}



static char *decode_lzw_code( char *buffer, LZWCode code )
/*
**  This routine decodes an LZW code and returns its corresponding string.
**  Due to the method in which the string lookup table is generated, the
**  string ends up in reverse order.
*/
{
   uword count = 0;

   /*  As soon as the code is just a simple byte, exit  */
   while( code > LZW_LAST_CHAR )
   {
      /*
      **  Recursively decode a character and its prefix into the buffer
      **  provided (backwards).
      */
      *buffer++ = lzwChar[ code ];
      code = lzwString[ code ];
      if( count++ > LZW_STACK_SIZE )
         clean_exit( "LZW file overran internal stack" );
   }
   *buffer = code;

   return( buffer );
}



static CheckSum decompress_lzw( void )
/*
**  This routine expands files compressed using the Lempel-Ziv-Welch (LZW)
**  algorithm.  Three buffers are allocated: one for string codes, one for
**  appended characters, and one to act as a decode stack as the codes are
**  read from the input file.
*/
{
   CheckSum crc = INIT_CRC;

   /*  Get the string buffer  */
   if( lzwString = (LZWCode *)malloc( sizeof( LZWCode ) * LZW_TABLE_SIZE ) )
   {
      /*  Get the buffer for the individual characters  */
      if( lzwChar = (char *)malloc( LZW_TABLE_SIZE ) )
      {
         char *stack;

         /*  Get the stack buffer  */
         if( stack = (char *)malloc( LZW_STACK_SIZE ) )
         {
            LZWCode oldCode;
            char character, *string;

            /*  Initialize the major variables  */
            lzwNext = LZW_FIRST_CODE;
            lzwSize = LZW_MIN_SIZE;
            lzwBitCount = 0;
            lzwBuffer = lzwNum = 0L;

            /*  Get and process the first code in the file  */
            get_lzw_code();
            oldCode = character = lzwNew;
            crc = calc_crc( &character, 1, crc );
            if( NOT test() )
               putc( character, out );

            lzwDone = FALSE;
            while( NOT lzwDone )
            {
               /*
               **  Get and decode the next code.  There's only one special
               **  case for the LZW decompression algorithm, occurring
               **  when the code read hasn't been added to the string
               **  table yet.  It's handled here.
               */
               get_lzw_code();
               if( lzwNew >= lzwNext )
               {
                  *stack = character;
                  string = decode_lzw_code( stack + 1, oldCode );
               }
               else
                  string = decode_lzw_code( stack, lzwNew );

               /*
               **  Output the decoded string, updating the CRC for each
               **  character it contains.
               */
               character = *string;
               while( string >= stack )
               {
                  crc = calc_crc( string, 1, crc );
                  if( NOT test() )
                     putc( *string, out );
                  string--;
               }

               /*  Add the new code to the string table  */
               if( lzwNext < LZW_MAX_LAST_CODE )
               {
                  *(lzwString + lzwNext) = oldCode;
                  *(lzwChar + lzwNext) = character;
               }
               oldCode = lzwNew;

               /*
               **  Since this implementation of LZW uses variable-length
               **  codes, we have to increase their size periodically (as
               **  soon as lzwNext can no longer be represented with the
               **  current number of bits).
               */
               if( ++lzwNext == LZW_LAST_CODE )
               {
                  if( lzwSize < LZW_MAX_SIZE )
                     lzwSize++;
               }
            }
            free( stack );
         }
         else
            clean_exit( "Out of memory" );

         free( lzwChar );
      }
      else
         clean_exit( "Out of memory" );

      free( lzwString );
   }
   else
      clean_exit( "Out of memory" );

   return( crc );
}



static bool get_huff_bit( void )
/*
**  This routine returns the next bit from the input stream, which will be
**  decoded and added to the decoding tree.
*/
{
   static ubyte ch;

   if( NOT huffBitCount )
   {
      fread( &ch, sizeof( ubyte ), 1, in );
      huffBitCount = 8;
   }
   huffBitCount--;

   return( (bool)((ch >> huffBitCount) & 1) );
}



static ubyte get_huff_byte( enum HuffMode mode )
/*
**  Stores or retrieves a byte to or from the Huffman decoding tree.  When
**  building the tree, values are stored (obviously).  Subsequent calls
**  translate the coded bytes from the input stream.
*/
{
   HuffNode *node;
   ubyte count = 0, value = 0;

   if( FIND == mode )
   {
      node = nList;
      while( NOT node->bit )
         node = get_huff_bit() ? node->one : node->zero;
      value = node->value;
   }
   else
      while( count++ < 8 )
         value = (value << 1) + get_huff_bit();

   return( value );
}



static HuffNode *read_huff_tree( void )
/*
**  Builds a branch of the Huffman decoding tree based on the value of the
**  next bit from the input stream.  A pointer to its first node is returned,
**  since the tree is built recursively.
*/
{
   HuffNode *node;

   node = nPtr++;
   if( get_huff_bit() )
   {
      node->bit   = 1;
      node->value = get_huff_byte( BUILD );
   }
   else
   {
      node->bit  = 0;
      node->zero = read_huff_tree();
      node->one  = read_huff_tree();
   }

   return( node );
}



static CheckSum decompress_huff( void )
/*
**  This function expands files compressed using adaptive Huffman encoding.
*/
{
   CheckSum crc = INIT_CRC;
   ubyte *buffer;

   /*  The buffer should be based on the decompressed size  */
   if( buffer = (ubyte *)malloc( min( after, MAX_BUFFER_SIZE ) ) )
   {
      /*  Too bad we don't know how many these we'll need before hand  */
      if( nList = (HuffNode *)malloc( sizeof( HuffNode ) * HUFF_MAX_NODES ) )
      {
         ulong i, num;

         /*  Initialize some stuff and build the decoding tree  */ 
         nPtr = nList;
         huffBitCount = 0;
         read_huff_tree();

         while( after )
         {
            /*  Grab as many bytes as possible  */
            num = min( after, MAX_BUFFER_SIZE );
            for( i = 0; i < num; i++ )
               *(buffer + i) = get_huff_byte( FIND );

            /*  Update the CRC  */
            crc = calc_crc( buffer, num, crc );
            if( NOT test() )
               fwrite( buffer, sizeof( ubyte ), num, out );

            after -= num;
         }
         free( nList );
      }
      else
         clean_exit( "Out of memory" );

      free( buffer );
   }
   else
      clean_exit( "Out of memory" );

   return( crc );
}



CheckSum expand( FILE *input, const FileHeader *fileHdr,
   char *name, enum Command fork )
/*
**  This routine tries to decompress the file described by the FileHeader
**  passed.  If we're actually unstuffing something (not just testing the
**  archive's integrity), output will go to 'name'.
*/
{
   Method method;
   CheckSum crc = INIT_CRC;

   /*
   **  This is a pretty clunky way of doing this, but hey, it's all
   **  relative anyway.  I mean, I've written lots of code, some of it
   **  like this, but mostly the very picture of cogent eloquence.
   **  Right...
   */
   if( DATA == fork )
   {
      before = fileHdr->compDLength;
      after  = fileHdr->dataLength;
      method = fileHdr->compDMethod;
   }
   else
   {
      before = fileHdr->compRLength;
      after  = fileHdr->rsrcLength;
      method = fileHdr->compRMethod;
   }
   in = input;

   /*
   **  Make sure the fork in question actually HAS information in it,
   **  then try to open an output file if necessary.
   */
   if( before && (test() || (out = fopen( name, "wb" ))) )
   {
      /*  Use the decompression appropriate for the file  */
      switch( method )
      {
         case NONE:
            crc = decompress_none();
            break;

         case RLE:
            crc = decompress_rle();
            break;

         case LZW:
            crc = decompress_lzw();
            break;

         case HUFF:
            crc = decompress_huff();
            break;

         default:
            clean_exit( "File uses unknown compression type" );
            break;
      }

      if( NOT test() )
         fclose( out );
   }

   return( crc );
}
