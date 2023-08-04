/* This program demonstrates how an isomorphism is found between
   two graphs, using the Moebius graph as an example.
   This version uses sparse form with dynamic allocation.
*/

// #include "nausparse.h"    /* which includes nauty.h */

#include <stdlib.h>
#include "CLI11.hpp"
#include "nauty_utils.h"
#include "isofilter.h"


int
main(int argc, char *argv[])
{
    
    CLI::App app("Lexicography smallest automorphic model.");
    Options opt;
    
    app.add_option("file_name", opt.file_name, "file name")->default_val("-");
    app.add_flag("-c", opt.out_cg, "output canonical graphs also")->default_val(false);
    app.add_flag("-m", opt.max_cache, "max num of canonical graphs (as string) in cache")->default_val(-1);

    CLI11_PARSE(app, argc, argv);

    IsoFilter filter(opt);
    filter.process_all_models();

    exit(0);
}
