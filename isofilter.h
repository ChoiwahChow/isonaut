/* isofilter.h : Mace4 style isofiltering. */
/* Version 1.1, July 2023. */


#include <algorithm>
#include <string>
#include <vector>
#include <bits/stdc++.h>

#include <iostream>
// #include <ostream>
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
    IsoFilter(bool ocg) : out_cg(ocg) {};
    IsoFilter(): out_cg(false) {};

    bool parse_model(std::istream& f, Model& m, std::string& m_str);
    bool parse_unary(const std::string& line, Model& m);
    bool parse_bin(std::istream& f, Model& m, std::string& m_str);
    void parse_row(std::string& line, std::vector<size_t>& row);
    int  process_all_models();
    int  find_arity(const std::string& func);
    void blankout(std::string& s) { std::replace( s.begin(), s.end(), ',', ' '); };

    bool build_graph(Model& m);

    bool is_non_iso(const Model&);
    bool is_non_iso_hash(const Model&);
};


