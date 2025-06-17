/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef G_DEFINITIONS_H
#define G_DEFINITIONS_H

#include "config.h"

#ifndef BOOL
#define BOOL
typedef int Bool;
#define True (1)
#define False (0)
#endif

#ifndef Null
#define Null ((void *)0)
#endif

#ifndef MAX_INPUT
#define MAX_INPUT (10000)
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL (2)
#endif

static const int BITS_PER_BYTE = 8;

#endif  // G_DEFINITIONS
