

#ifndef NAUTY_UTILS_H
#define NAUTY_UTILS_H

#include <string>

#include "nausparse.h"    /* which includes nauty.h */

std::string put_sg_str(sparsegraph *sg, const char* sep = "\n", boolean digraph = false, int linelength = 10000000);

#endif
