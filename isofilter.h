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

extern int isofilter();


class Model {
public:
    static const std::string Interpretation_label;
    static const std::string Function_label;
    static const std::string Function_unary_label;
    static const std::string Function_binary_label;
    static const std::string Function_stopper;
    static const std::string Model_stopper;

public:
    std::vector<std::vector<std::vector<std::vector<size_t>>>> ternary_ops;
    std::vector<std::vector<std::vector<size_t>>> bin_ops;
    std::vector<std::vector<size_t>> un_ops;

    std::vector<std::string>  op_symbols;

    size_t       order;
    sparsegraph* cg;
    std::string  model_str;

public:
    bool parse_model(std::istream& f);
    bool parse_unary(const std::string& line);
    bool parse_bin(std::istream& f);
    void parse_row(std::string& line, std::vector<size_t>& row);
    int  find_arity(const std::string& func);
    void blankout(std::string& s) { std::replace( s.begin(), s.end(), ',', ' '); };

public:
    Model(): order(2) {};
    ~Model() {SG_FREE(*cg);};

    bool operator==(const Model& a) const;
    std::string  graph_to_string(sparsegraph* g) const;

    void print_model(std::ostream&, bool out_cg=false) const;

    void fill_meta_data(const std::string& interp);
    void find_func_name(const std::string& func);
    bool find_graph_size(size_t& num_vertices, size_t& num_edges, bool& has_S, bool& has_T);
    void color_graph(int* ptn, int* lab, int ptn_sz, bool has_S);
    void count_occurrences(std::vector<size_t>& R_v_count);
    void build_vertices(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                        const int R_v, const int A_c, bool has_S);
    void build_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                     const int R_v, const int A_c, bool has_S);
    bool build_graph();
};

class IsoFilter {
private:
    std::vector<Model> non_iso_vec;
    std::unordered_set<std::string>  non_iso_hash;
    bool out_cg;

public:
    double  start_time;       // in micro sec
    double  start_cpu_time;   // in micro sec

public:
    IsoFilter(bool ocg) : out_cg(ocg) {};
    IsoFilter(): out_cg(false) {};

    int  process_all_models();

    bool is_non_iso(const Model&);
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


