/* This program demonstrates how an isomorphism is found between
   two graphs, using the Moebius graph as an example.
   This version uses sparse form with dynamic allocation.
*/

// #include "nausparse.h"    /* which includes nauty.h */


#include <sys/resource.h>
#include <sys/time.h>

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
    app.add_option("-m", opt.max_cache, "max num of canonical graphs (as string) in cache")->default_val(-1);
    app.add_option("-k", opt.check_sym, "list of comma-separated func/relation symbols to check for isomorphism")->default_val("");
    app.add_flag("-x", opt.compress, "compress the canonical graph string")->default_val(false);
    app.add_flag("-s", opt.shorten_str, "shortend canonical graph string")->default_val(false);
    app.add_flag("-t", opt.test, "run isomorphismAlgebras")->default_val(false);

    CLI11_PARSE(app, argc, argv);

    IsoFilter filter(opt);
    if (opt.test)
        filter.Test_IsomorphismAlgebras();
    else
        filter.process_all_models();

  struct rusage usage;
  int ret = getrusage(RUSAGE_THREAD, &usage);

  std::cerr << "\nMaximum resident size: " << usage.ru_maxrss/1000000.0 << " GB" << std::endl;

    exit(0);
}
