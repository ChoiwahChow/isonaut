/* isofilter.h : isofiltering for Mace4 models. */
/* Version 1.1, July 2023. */

#ifndef MODEL_H
#define MODEL_H

#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <string>
#include <vector>
#include <bits/stdc++.h>

#include <iostream>
#include <fstream>


struct sparsegraph;

class Model {
public:
    static const std::string Interpretation_label;
    static const std::string Function_label;
    static const std::string Relation_label;
    static const std::string Function_arity_label;
    static const std::string Function_unary_label;
    static const std::string Function_binary_label;
    static const std::string Function_stopper;
    static const std::string Model_stopper;

public:
    std::vector<std::vector<std::vector<std::vector<int>>>> ternary_ops;
    std::vector<std::vector<std::vector<int>>> bin_ops;
    std::vector<std::vector<std::vector<int>>> bin_rels;
    std::vector<std::vector<int>> un_ops;
    std::vector<int> constants;

    std::vector<std::string>  op_symbols;

    size_t       order;
    size_t       el_fixed_width;
    sparsegraph* cg;
    std::string  model_str;
    std::vector<std::size_t>  iso;

private:
    static const char Base64Table[];
    static const char unassigned = '?';
    static const char op_end = ';';

private:
    void   set_width(size_t order);
    size_t compress_str(int label, size_t width, std::string& cms) const;
    size_t find_graph_size(size_t& num_vertices, size_t& num_edges);
    void   color_vertices(int* ptn, int* lab, int ptn_sz);
    void   count_occurrences(std::vector<size_t>& R_v_count);
    void   count_truth_values(std::vector<size_t>& L_v_count);
    void   build_vertices(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                          const int R_v, const int L_v, const int A_c);
    void   build_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                       const int R_v, const int L_v, const int A_c);

    void   debug_print_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                             const int R_v, const int A_c, bool has_S);

    bool parse_unary(const std::string& line, bool ignore_op);
    bool parse_bin(std::istream& f, bool is_func, bool ignore_op);
    void parse_row(std::string& line, std::vector<int>& row);
    int  find_arity(const std::string& func);
    void blankout(std::string& s) { std::replace( s.begin(), s.end(), ']', ' '); std::replace( s.begin(), s.end(), ',', ' '); };

public:
    Model(): order(2), el_fixed_width(1), cg(nullptr) {};
    Model(size_t odr, std::vector<int>& constants, std::vector<std::vector<int>>& un_ops,
          std::vector<std::vector<std::vector<int>>>& bin_ops, std::vector<std::vector<std::vector<int>>>& bin_rels);
    ~Model();

    bool operator==(const Model& a) const;
    std::string  graph_to_string(sparsegraph* g, const char* sep = "\n", bool shorten = false) const;
    std::string  graph_to_shortened_string(sparsegraph* g) const;
    std::string  cg_to_string(const char* sep = "\n", bool shorten = false) { return graph_to_string(cg, sep); };

    void print_model(std::ostream&, const std::string& canon_str, bool out_cg=false) const;

    void fill_meta_data(const std::string& interp);
    std::string find_func_name(const std::string& func);

    bool parse_model(std::istream& f, const std::string& check_sym);
    bool build_graph();
    std::string compress_cms() const;
};

#endif

