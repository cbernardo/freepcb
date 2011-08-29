// This is a class to generate simple unique IDs within an application
//
#pragma once
#include "stdafx.h"
//#include <basetsd.h>

class Cuid {
private:
	enum{ 
		MAX_VALUE = 0x1fffff, 			// max uid value
		BITS_SIZE = (MAX_VALUE+1)/32,	// size of bit array
		INDEX_MASK = MAX_VALUE >> 5		// mask for index into array
	};	
	UINT32 bits[BITS_SIZE];		// array of bits, one bit for each possible uid 
	int n_uids;					// number of uids assigned
	UINT32 mask_table[32];		// mask for bit positions

public:
	Cuid();
	~Cuid();

	int GetNewUID();
	void ReleaseUID( UINT32 uid );
	BOOL CheckUID( int uid );
	BOOL RequestUID( int uid );
	BOOL ReplaceUID( int old_uid, int new_uid );
};
