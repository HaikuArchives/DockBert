/*
 * Copyright 2017-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once


#include <Bitmap.h>

// Gets an icon from executable's resources
status_t GetVectorIcon(const BString& icon, BBitmap* bitmap);
