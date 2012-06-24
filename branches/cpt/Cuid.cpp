// This is a class to generate simple unique identifiers (UIDs) within an application
// Since these never appear outside a single execution of FreePCB, it just 
// returns sequential 32-bit numbers starting at 1
//
// a UID value of 0 or -1 indicates an unassigned or invalid UID
//
#include "stdafx.h"
#include "Cuid.h"

// global Cuid
Cuid pcb_cuid;

// global pointer to the document
extern CFreePcbDoc * pcb;

Cuid::Cuid()
{
	last_uid = 0;	// initialize UID counter
}

Cuid::~Cuid()
{
}

// Return next UID
//
int Cuid::GetNewUID()
{
	last_uid++;
	// check for overflow
	if( last_uid == 0xffffffff )
		ASSERT(0);
	return last_uid;
}
