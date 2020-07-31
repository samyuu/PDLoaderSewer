#pragma once
#include "Types.h"
#include <stdio.h>

#define LOG_WRITELINE(format, ...)	printf(__FUNCTION__ "(): " format "\n", ##__VA_ARGS__)
