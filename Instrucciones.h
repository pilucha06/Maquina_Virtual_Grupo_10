#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include "MV.h"

#define MAXINSTR 32

typedef void (*instrucciones)(TMV *, int, int);

extern instrucciones vecInstr[MAXINSTR];

#endif