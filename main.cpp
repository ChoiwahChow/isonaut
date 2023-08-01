/* This program demonstrates how an isomorphism is found between
   two graphs, using the Moebius graph as an example.
   This version uses sparse form with dynamic allocation.
*/

// #include "nausparse.h"    /* which includes nauty.h */

#include <stdlib.h>
#include "CLI11.hpp"
#include "isofilter.h"


int
main(int argc, char *argv[])
{
    
    CLI::App app("Lexicography smallest automorphic model.");
    bool out_cg = false;

    app.add_flag("-c", out_cg, "output canonical graph also")->default_val(false);

    CLI11_PARSE(app, argc, argv);

    IsoFilter filter(out_cg);
    filter.process_all_models();

    exit(0);
}
