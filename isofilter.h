/* isofilter.h : isofiltering for Mace4 models. */
/* Version 1.1, July 2023. */

#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <string>
#include <vector>
#include <bits/stdc++.h>

#include <iostream>
#include <fstream>
#include "nausparse.h"    /* which includes nauty.h */
#include "nauty_utils.h"
#include "model.h"


class IsoFilter {
private:
    std::vector<Model>               non_iso_vec;     // copy and assignment constructors for Model are needed if this vector is to be used
    std::unordered_set<std::string>  non_iso_hash;
    bool out_cg;

public:
    double  start_time;       // in micro sec
    double  start_cpu_time;   // in micro sec

public:
    IsoFilter(bool ocg) : out_cg(ocg) {};
    IsoFilter(): out_cg(false) {};

    int  process_all_models();

    bool is_non_iso(const Model&);    // for debugging only
    bool is_non_iso_hash(const Model&);

    static double read_cpu_time() {
        struct rusage ru;
        getrusage(RUSAGE_SELF, &ru);
        return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
    };
    static unsigned read_wall_clock() {
        time_t t=time( (time_t *) NULL );
        return (unsigned) t;
    }
};


