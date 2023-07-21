/* This program demonstrates how an isomorphism is found between
   two graphs, using the Moebius graph as an example.
   This version uses sparse form with dynamic allocation.
*/

// #include "nausparse.h"    /* which includes nauty.h */

#include <stdlib.h>
#include "isofilter.h"
#include "test_iso_graph.h"


int
main(int argc, char *argv[])
{
    //test_iso_graph(2);
    IsoFilter filter;
    filter.process_all_models();

    exit(0);
}
