/*
 * Copyright 2024, Nexus6 <nexus6@disroot.org>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Preferences.h"


Preferences::Preferences(BMessenger target)
	: fTarget(target)
{
}


Preferences::~Preferences()
{
}


int32
Preferences::CountProperties(BString property)
{
	BMessage message(B_COUNT_PROPERTIES);
	message.AddSpecifier(property);
	BMessage reply;
	fTarget.SendMessage(&message, &reply);
	return reply.GetInt32("response", -1);
}