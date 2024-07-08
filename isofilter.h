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

#include <ext/pb_ds/assoc_container.hpp>

#include <iostream>
#include <fstream>
// #include <zlib.h>
#include <ext/pb_ds/assoc_container.hpp>
#include "model.h"

struct Options {
    bool        out_cg;
    bool        compress;
    int         max_cache;
    bool        shorten_str;
    std::string file_name;
    std::string check_sym;
    bool        test;

    Options() : out_cg(false), compress(false), max_cache(-1), shorten_str(false), test(false) {};
};


class IsoFilter {
private:
    std::vector<Model>               non_iso_vec;     // copy and assignment constructors for Model are needed if this vector is to be used
    std::unordered_set<std::string>  non_iso_hash;
    Options opt;
    size_t branch_key;
    __gnu_pbds::gp_hash_table<std::string, size_t> non_iso_hash_table;

private:
    size_t get_branch_key(const std::string& canon_str);

public:
    double  start_time;       // in micro sec
    double  start_cpu_time;   // in micro sec

public:
    IsoFilter(const Options& opt) : opt(opt), branch_key(0) {};
    IsoFilter() {};

    void set_options(Options& in_opt) { opt=in_opt; };

    int  process_all_models();

    bool is_non_iso(const Model&);    // for debugging only
    bool is_non_iso_hash(const Model&, std::string&);

    static double read_cpu_time() {
        struct rusage ru;
        getrusage(RUSAGE_SELF, &ru);
        return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
    };
    static unsigned read_wall_clock() {
        time_t t=time( (time_t *) NULL );
        return (unsigned) t;
    }
    bool is_non_isomorphic(Model& m, std::string& shortened_str);
    bool cache_exceeded() const { return opt.max_cache >= 0 && non_iso_hash.size() >= opt.max_cache; }
    // std::string compress(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION);

    bool IsomorphicAlgebras(const Model& model1, const Model& model2) const;
    void Test_IsomorphismAlgebras();
};

#endif

