#ifndef FUZZER_H_INCLUDED
#define FUZZER_H_INCLUDED

#include "gstruct.h"

typedef size_t depth_t;

void fuzz(Grammar* grammar, depth_t max_depth);

#endif