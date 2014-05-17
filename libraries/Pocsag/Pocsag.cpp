// pocsag.cpp
// Author: Kristoff Bonne (kristoff@skypro.be)
// Copyright (C) 2014 Kristoff Bonne (ON1ARF)

// for 32bit integers
#include <stdint.h>

// for string operations
#include <string.h>

// include .h file
#include <Pocsag.h>


// constructor
Pocsag::Pocsag () {
	state=0;
	size=0;
}; // end constructor

// Get State
int Pocsag::GetState () {
	return(state);
}; // end method Get State

// Get Size
int Pocsag::GetSize () {
	return(size);
}; // end method Get Size

// Get Error
int Pocsag::GetError () {
	return(error);
}; // end method Get Size

// Get Message Pointer
void * Pocsag::GetMsgPointer () {
	return((void *) &Pocsagmsg);
}; // end method Get Msg Pointer

// creates pocsag message in pocsagmsg structure
// return 0 on failure, 1 on success
int Pocsag::CreatePocsag (long int address, int source, char * text, int option_batch2, int option_invert) {
int txtlen;
unsigned char c; // tempory var for character being processed
int stop; /// temp var

char lastchar; // memorise last char of input text. (changed at the beginning of the function
					// 		reset when done

unsigned char txtcoded[56]; // encoded text can be up to 56 octets
int txtcodedlen;

// local vars to encode address line
int currentframe;
uint32_t addressline;

// counters to encode text
int bitcount_in, bitcount_out, bytecount_in, bytecount_out;

// table to convert size to n-times 1 bit mask
const unsigned char size2mask[7] = {0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f};


// some sanity checks
txtlen=strlen(text);
if (txtlen > 40) {
  // messages can be up to 40 chars (+ terminating EOT)
  txtlen=40;
}; // end if

// reinit state to 0 (no message)
state = 0;
error = POCSAGRC_UNDETERMINED;
size=0;

// some sanity checks for the address
// addreess are 21 bits
if ((address > 0x1FFFFF) || (address <= 0)) {
  error=POCSAGRC_INVALIDADDRESS;
  return(POCSAG_FAILED);
}; // end if

// source is 2 bits
if ((source < 0) || (source > 3)) {
  error=POCSAGRC_INVALIDSOURCE;
  return(POCSAG_FAILED);
}; // end if

// option "batch2" goes from 0 to 2
if ((option_batch2 < 0) || (option_batch2 > 2)) {
  error=POCSAGRC_INVALIDBATCH2OPT;
  return(POCSAG_FAILED);
}; // end if

// option "invert" should be 0 or 1
if ((option_invert < 0) || (option_invert > 1)) {
  error=POCSAGRC_INVALIDINVERTOPT;
  return(POCSAG_FAILED);
}; // end if



// replace terminating \0 by EOT
lastchar=text[txtlen];
text[txtlen]=0x04; // EOT (end of transmission)
txtlen++; // increase size by 1 (for EOT)

// now we know everything is OK. Set state to 1 (message)
state = 1;


// create packet

// A POCSAG packet has the following structure:
// - transmission syncronisation (576 bit "01" pattern)

// one or multiple "batches".
// - a batch starts with a 32 frame syncronisation pattern
// - followed by 8 * 2 * 32 bits of actual data or "idle" information:
//				the "POCSAG message"


// A POSAG message consists of one or multiple batches. A batch is 64
// octets: 8 frames of 2 codewords of 32 bits each

// This application can generate one pocsag-message containing one
// text-message of up to 40 characters + a terminating EOT
// (end-of-transmission, ASCII 0x04).

// the message (i.e. the destination-address + the text messate itself)
// always starts in the first batch but not necessairy at the beginning
// of the batch. The start-position of message inside the batch is
// determined by (the 3 lowest bits of) the address.

// if the message reaches the end of the first batch, if overflows
// into the 2nd batch.

// As a result, we do not know beforehand if the POCSAG frame will be
// one or two POCSAG message, so we will initialise both batches. If
// -and the end of the process- it turns out that the message fits into
// one single batch, there are three options:
// - truncate the message and send a POCSAG-frame containing one POCSAG-
// 	message
// - repeat the first batch in the 2nd batch  and send both batches
// - leave the 2nd batch empty (i.e. filled with the "idle" pattern) and
// 	send that

// note that before actually transmitting the pattern, all bits are inverted 0-to-1
// for the parts of the frame with a fixed text (syncronisation pattern), the
// bits are inverted in the code

// part 0.1: frame syncronisation pattern
if (option_invert == 0) {
  memset(Pocsagmsg.sync,0xaa,72);
} else {
  memset(Pocsagmsg.sync,0x55,72); // pattern 0x55 is invers of 0xaa
}; // end else - if

// part 0.2: batch syncronisation

// a batch begins with a sync codeword
// 0111 1100 1101 0010 0001 0101 1101 1000
Pocsagmsg.synccw1[0]=0x7c;  Pocsagmsg.synccw1[1]=0xd2;
Pocsagmsg.synccw1[2]=0x15;  Pocsagmsg.synccw1[3]=0xd8;

Pocsagmsg.synccw2[0]=0x7c;  Pocsagmsg.synccw2[1]=0xd2;
Pocsagmsg.synccw2[2]=0x15;  Pocsagmsg.synccw2[3]=0xd8;

// invert bits if needed
if (option_invert == 1) {
  Pocsagmsg.synccw1[0] ^= 0xff; Pocsagmsg.synccw1[1] ^= 0xff;
  Pocsagmsg.synccw1[2] ^= 0xff; Pocsagmsg.synccw1[3] ^= 0xff;

  Pocsagmsg.synccw2[0] ^= 0xff; Pocsagmsg.synccw2[1] ^= 0xff;
  Pocsagmsg.synccw2[2] ^= 0xff; Pocsagmsg.synccw2[3] ^= 0xff;
};

// part 0.3: init batches with all "idle-pattern"
for (int l=0; l< 16; l++) {
  Pocsagmsg.batch1[l]=0x7a89c197;
  Pocsagmsg.batch2[l]=0x7a89c197;
}; // end for


// part 1:
// address line:
// the format of a pocsag codeword containing an address is as follows:
// 0aaaaaaa aaaaaaaa aaassccc ccccccp
// "0" = fixed (indicating an codeword containing an address)
// a[18] = 18 upper bits of the address
// s[2]  = 2 bits source (encoding 4 different "address-spaces")
// c[10] = 10 bits of crc
// p = parity bit

// the lowest 3 bits of address are not encoded in the address line as
// found in the POCSAG message. However, they are used to determine
// where in the POCSAG-message, the message is started
currentframe = ((address & 0x7)<<1);


addressline=address >> 3;

// add address source
addressline<<=2;
addressline += source;

//// replace address line
replaceline(&Pocsagmsg,currentframe,createcrc(addressline<<11));





// part 2.1: 
// convert text to podsag format

// POCSAG codewords containing a message have the following format:
// 1ttttttt tttttttt tttttccc ccccccp
// "1" = fixed (indicating a codeword containing an encoded message)
// t[20] = 20 bits of text
// c[10] = 10 bits CRC
// p = parity bit

// text-messages are encoded using 7 bits ascii, IN INVERTED BITORDER (MSB first)

// As up to 20 bits can be stored in a codeword, each codeword contains 3 chars
//    minus 1 bit. Hence, the text is "wrapped" around to the next codewords


// The message is first encoded and stored into a tempory array

// init vars
// clear txtcoded
memset(txtcoded,0x00,56); // 56 octets, all "0x00"

bitcount_out = 7; // 1 char can contain up to 7 bits
bytecount_out = 0;

bitcount_in = 7;
bytecount_in=0;
c=flip7charbitorder(text[0]); // start with first char


txtcodedlen=0;
txtcoded[0] = 0x80; // output, initialise as empty with  leftmost bit="1"



// loop for all chars
stop=0;

while (!stop) {
  int bits2copy;
  unsigned char t; // tempory var bits to be copied

  // how many bits to copy?
  // minimum of "bits available for input" and "bits that can be stored"
  if (bitcount_in > bitcount_out) {
    bits2copy = bitcount_out;
  } else {
    bits2copy = bitcount_in;
  }; // end if

	// copy "bits2copy" bits, shifted the left if needed
   t = c & (size2mask[bits2copy-1] << (bitcount_in - bits2copy) );

  // where to place ?
  // move left to write if needed
  if (bitcount_in > bitcount_out) {
    // move to right and export
    t >>= (bitcount_in-bitcount_out);
  } else if (bitcount_in < bitcount_out) {
    // move to left
    t <<= (bitcount_out-bitcount_in);
  }; // end else - if

  // now copy bits
  txtcoded[txtcodedlen] |= t;

  // decrease bit counters
  bitcount_in -= bits2copy;
  bitcount_out -= bits2copy;


  // outbound queue is full
  if (bitcount_out == 0) {
    // go to next position in txtcoded
    bytecount_out++;
    txtcodedlen++;

    // data is stored in codeblocks, each containing 20 bits of usefull data
    // The 1st octet of every codeblock always starts with a "1" (leftmost char)
    // to indicate the codeblock contains data
    // The 1st octet of a codeblock contains 8 bits: "1" + 7 databits
    // The 2nd octet of a codeblock contains 8 bits: 8 databits
    // the 3th octet of a codeblock contains 5 bits: 5 databits

    if (bytecount_out == 1) {
      // 2nd char of codeword:
      // all 8 bits used, init with all 0
      txtcoded[txtcodedlen]=0x00;
      bitcount_out=8;
    } else if (bytecount_out == 2) {
      // 3th char of codeword:
      // only 5 bits used, init with all 0
      txtcoded[txtcodedlen]=0x00;
      bitcount_out=5;
    } else if (bytecount_out >= 3) {
      // 1st char of codeword (wrap around of "bytecount_out" value)
      // init with leftmost bit as "1"
      txtcoded[txtcodedlen]=0x80;
      bitcount_out = 7;

      // wrap around "bytecount_out" value
      bytecount_out = 0;
    }; // end elsif - elsif - if

  }; // end if

  // ingress queue is empty
  if (bitcount_in == 0) {
    bytecount_in++;

    // any more text ?
    if (bytecount_in < txtlen) {
      // reinit vars
      c=flip7charbitorder(text[bytecount_in]);
      bitcount_in=7; // only use 7 bits of every char
    } else {
      // no more text
      // done, so ... stop!!
      stop=1;

      continue;
    }; // end else - if
  }; // end if
}; // end while (stop)

// increase txtcodedlen. Was used as index (0 to ...) above. Add one to
// make it indicate a lenth
txtcodedlen++;



// Part 2.2: copy coded message into pocsag message

// now insert text message
for (int l=0; l < txtcodedlen; l+= 3) {
  uint32_t t;

  // move up to next frame
  currentframe++;

  // copying is done octet per octet to be architecture / endian independant
  t = (uint32_t) txtcoded[l] << 24; // copy octet to bits 24 to 31 in 32 bit struct
  t |= (uint32_t) txtcoded[l+1] << 16; // copy octet to bits 16 to 23 in 32 bit struct
  t |= (uint32_t) txtcoded[l+2] << 11; // copy octet to bits 11 to 15 in 32 bit struct

  replaceline(&Pocsagmsg,currentframe,createcrc(t));

  // note: move up 3 chars at a time (see "for" above)
}; // end for


// invert bits if needed
if (option_invert) {
  for (int l=0; l<16; l++) {
    Pocsagmsg.batch1[l] ^= 0xffffffff;
  }; // end for

  for (int l=0; l<16; l++) {
    Pocsagmsg.batch2[l] ^= 0xffffffff;
  }; // end for
  
}; 


/// convert to make endian/architecture independant


for (int l=0; l<16; l++) {
  int32_t t1; 
  
  // structure to convert int32 to architecture  / endian independant 4-char array
	struct int32_4char {
		union {
			int32_t i;
			unsigned char c[4];
		}; 
	} t2; 


//  struct int32_4char t2;

  // batch 1
  t1=Pocsagmsg.batch1[l];
  
  // left most octet
  t2.c[0]=(t1>>24)&0xff; t2.c[1]=(t1>>16)&0xff;
  t2.c[2]=(t1>>8)&0xff; t2.c[3]=t1&0xff;

  // copy back
  Pocsagmsg.batch1[l]=t2.i;

  // batch 2
  t1=Pocsagmsg.batch2[l];
  
  // left most octet
  t2.c[0]=(t1>>24)&0xff; t2.c[1]=(t1>>16)&0xff;
  t2.c[2]=(t1>>8)&0xff; t2.c[3]=t1&0xff;

  // copy back
  Pocsagmsg.batch2[l]=t2.i;

}; // end for

  

// done
// return

// reset last char in input char (was overwritten at the beginning of the function)
text[txtlen]=lastchar;

// If only one single batch used
if (currentframe < 16) {
  // batch2 option:
  // 0: truncate to one batch
  // 1: copy batch1 to batch2
  // 2: leave batch2 as "idle"

  if (option_batch2 == 0) {
    // done. set length to one single batch (140 octets)
    size=140;
    return(POCSAG_SUCCESS);
  } else if (option_batch2 == 1) {
    memcpy(Pocsagmsg.batch2,Pocsagmsg.batch1,64); //  16 codewords of 32 bits
  }; // end of

  // return for (option_batch2 == 1) or (option_batch2 == 2)
  // set length to 2 batches (208 octets)
	size=208;
  return(POCSAG_SUCCESS);
};


// more then one batch found
// length =  2 batches (208 octets)
size=208;
return(POCSAG_SUCCESS);


}; // end function create_pocsag







/////////////////////////////
/// replaceline

void Pocsag::replaceline (Pocsag::Pocsagmsg_s *msg, int line, uint32_t val) {

// sanity checks
if ((line < 0) || (line > 32)) {
  return;
}; // end if


if (line < 16) {
  msg->batch1[line]=val;
} else {
  msg->batch2[line-16]=val;
}; // end if


}; // end replaceline


//////////////////////////
// flip7charbitorder

unsigned char Pocsag::flip7charbitorder (unsigned char c_in) {
int i;
char c_out;

// init
c_out=0x00;

// copy and shift 6 bits
for (int l=0; l<6; l++) {
  if (c_in & 0x01) {
    c_out |= 1;
  }; // end if

  // shift data
  c_out <<= 1; c_in >>= 1;
}; // end for

// copy 7th bit, do not shift
if (c_in & 0x01) {
  c_out |= 1;
}; // end if

return(c_out);

}; // end flip2charbitorder


//////////////////////////////
/// createcrc

uint32_t Pocsag::createcrc(uint32_t in) {

// local vars
uint32_t cw; // codeword

uint32_t local_cw = 0;
uint32_t parity = 0;

// init cw
cw=in;

// move up 11 bits to make place for crc + parity
local_cw=in;  /* bch */

// calculate crc
for (int bit=1; bit<=21; bit++, cw <<= 1) {
  if (cw & 0x80000000) {
    cw ^= 0xED200000;
  }; // end if
}; // end for

local_cw |= (cw >> 21);

// parity
cw=local_cw;

for (int bit=1; bit<=32; bit++, cw<<=1) {
  if (cw & 0x80000000) {
    parity++;
  }; // end if
}; // end for


// make even parity
if (parity % 2) {
  local_cw++;
}; // end if

// done
return(local_cw);


}; // end createcrc

