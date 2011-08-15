// This is a class to generate simple unique identifiers (UIDs) within an application
// These UIDs are integers with values from 0 to 0x1fffff, so there are about 
// 2 million possible values
// A value of -1 indicates an invalid or unassigned UID
//
#include "stdafx.h"
#include "Cuid.h"

Cuid::Cuid()
{
	if( MAX_VALUE > 0x1fffff )
		ASSERT(0);
	// initialize mask table
	for( int i=0; i<32; i++ )
		mask_table[i] = 0x1 << i;
	// clear bit array
	for( int i=0; i<BITS_SIZE; i++ )
		bits[i] = 0;
	n_uids = 0;		// uid counter
	srand(0);		// seed random number generator
}

Cuid::~Cuid()
{
	// check for all uids released
	if( n_uids )
	{
		// for debugging, find first 10 unreleased uids
		int u_uid[10];
		int n_u_uids = 0;
		for( int i=0; i<BITS_SIZE; i++ )
		{
			int test_bits = bits[i];
			int npos = 0;
			while( test_bits )
			{
				if( test_bits & 0x1 )
				{
					int uid = (i<<5) + npos;
					u_uid[n_u_uids] = uid;
					n_u_uids++;
					if( n_u_uids >= 10 )
						break;
				}
				test_bits >>= 1;
				npos++;
				if( n_u_uids >= 10 )
					break;
			}
		}
		ASSERT(0);
	}
}

// Find available uid and set bit in bits[] to mark it as assigned
//
int Cuid::GetNewUID()
{
	int i;	// index into bits[]

	// find an element of bits[] that has a bit = 0
	UINT32 test_bits = 0xffffffff;
	while( test_bits == 0xffffffff )
	{
		i = rand() & INDEX_MASK;	
		test_bits = bits[i];
	}
	// get position of first zero bit in bits[i]
	for( int npos = 0; npos<32; npos++ )
	{
		int test = test_bits & 0x1;
		if( !test )
		{
			// right-most bit is 0
			bits[i] |= mask_table[npos];	// set bit flag
			n_uids++;
			if( n_uids > MAX_VALUE/2 )
				ASSERT(0);				// too many uids assigned
			int uid = (i<<5) + npos;
			return uid;
		}
		test_bits >>= 1;	// shift right
	}
	// failed, shouldn't happen
	ASSERT(0);
	return -1;	
}

// Check availability of a particular uid
// returns TRUE if available, FALSE if unavailable
BOOL Cuid::CheckUID( int uid )
{
	if( uid < 0 || uid > MAX_VALUE )
		ASSERT(0);
	int i = uid>>5;			// index into bits[]
	int npos = uid & 0x1f;	// bit number
	if( bits[i] & mask_table[npos] )
	{
		// uid already taken
		return FALSE;
	}
	return TRUE;
}

// Request a particular uid
// returns TRUE with uid assigned if available, FALSE if unavailable
BOOL Cuid::RequestUID( int uid )
{
	if( uid < 0 || uid > MAX_VALUE )
	{
		ASSERT(0);
	}
	int i = uid>>5;			// index into bits[]
	int npos = uid & 0x1f;	// bit number
	if( bits[i] & mask_table[npos] )
	{
		// uid already taken
		return FALSE;
	}
	else
	{
		bits[i] |= mask_table[npos];	// set bit flag
		n_uids++;
		if( n_uids > MAX_VALUE/2 )
		{
			ASSERT(0);			// overflow
		}
	}
	return TRUE;
}

// Release uid by clearing bit in bits[]
//
void Cuid::ReleaseUID( UINT32 uid )
{
	if( uid == -1 )	// never assigned
	{
		return;	
	}

	if( uid < 0 || uid > MAX_VALUE )
		ASSERT(0);
	int i = uid>>5;			// index into bits[]
	int npos = uid & 0x1f;	// bit number to clear
	int test = bits[i] & mask_table[npos]; 
	if( !test )			// check to see if uid was assigned
		ASSERT(0);		// no
	bits[i] &= ~mask_table[npos];	// clear bit
	n_uids--;			// decrement assigned uid counter
}
