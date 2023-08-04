/* isofilter.h : isofiltering for Mace4 models. */
/* Version 1.1, July 2023. */

#ifndef ISOFILTER_H
#define ISOFILTER_H

#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <string>
#include <vector>
#include <bits/stdc++.h>

#include <iostream>
#include <fstream>
#include "model.h"

struct Options {
    bool out_cg;

    Options() : out_cg(false) {};
};


class IsoFilter {
private:
    std::vector<Model>               non_iso_vec;     // copy and assignment constructors for Model are needed if this vector is to be used
    std::unordered_set<std::string>  non_iso_hash;
    Options opt;

public:
    double  start_time;       // in micro sec
    double  start_cpu_time;   // in micro sec

public:
    IsoFilter(const Options& opt) : opt(opt) {};
    IsoFilter() {};

    void set_options(Options& in_opt) { opt=in_opt; };

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
    bool is_non_isomorphic(Model& m);
};

#endif

