// pocsag.h
// Author: Kristoff Bonne (kristoff@skypro.be)
// Copyright (C) 2014 Kristoff Bonne (ON1ARF)
//


#ifndef POCSAG_H
#define POCSAG_H

class Pocsag {
public:

// enum's:
typedef enum {
	POCSAGRC_UNDETERMINED = 0, // Undetermined error
	POCSAGRC_INVALIDADDRESS, // RETURN CODE FOR INVALID ADDRESS
	POCSAGRC_INVALIDSOURCE, // RETURN CODE FOR INVALID ADDRESS SOURCE
	POCSAGRC_INVALIDBATCH2OPT, // RETURN CODE FOR INVALUD "BATCH2" OPTION
	POCSAGRC_INVALIDINVERTOPT // RETURN CODE FOR INVALID "INVERT" OPTION
} Pocsag_error;

typedef enum {
	POCSAG_FAILED = 0, // failed
	POCSAG_SUCCESS // success
} Pocsag_rc;


// structures
// a 2-batch pocsag message
typedef struct {
	unsigned char sync[72];

	// batch 1
	unsigned char synccw1[4];
	uint32_t batch1[16];

	// batch 2
	unsigned char synccw2[4];
	uint32_t batch2[16];
} Pocsagmsg_s; // end struct


// data
	Pocsagmsg_s Pocsagmsg;

// methodes
	// constructor
	Pocsag();

	// create Pocsag Message
	int CreatePocsag (long int, int, char *, int, int);	

	// get state
	int GetState ();

	// get size
	int GetSize ();

	// get error
	int GetError ();

	// get pointer to data
	void * GetMsgPointer();

private:
	// vars
	int state;
	int size;
	int error;

	uint32_t createcrc(uint32_t);
	unsigned char flip7charbitorder (unsigned char);
	void replaceline (Pocsag::Pocsagmsg_s *, int, uint32_t);
	
}; // end class Pocsag
#endif
