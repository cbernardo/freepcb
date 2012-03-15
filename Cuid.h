// This is a class to generate simple unique IDs within an application
//
#pragma once
#include "stdafx.h"
//#include <basetsd.h>

class Cuid {
private:
	UINT32 last_uid;

public:
	Cuid();
	~Cuid();
	int GetNewUID();
};