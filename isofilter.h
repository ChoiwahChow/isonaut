/* isofilter.h : Mace4 style isofiltering. */
/* Version 1.1, July 2023. */


#include <algorithm>
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include "nausparse.h"    /* which includes nauty.h */

extern int isofilter();


class Model {
public:
    static const std::string Interpretation_label;
    static const std::string Function_label;
    static const std::string Function_stopper;
    static const std::string Model_stopper;

public:
    std::vector<std::vector<std::vector<size_t>>> bin_ops;
    std::vector<std::vector<size_t>> un_ops;

    std::vector<std::string>  ops_symbol;

    size_t       order;
    sparsegraph* canon;

    Model(): canon(NULL), order(2) {};
};

class IsoFilter {
private:
    std::vector<Model> models;

public:
    bool parse_model(std::istream& f, Model& m, std::string& m_str);
    bool parse_bin(std::istream& f, Model& m, std::string& m_str);
    int  process_all_models();
    void find_name(const std::string& func, std::string& name);
    void fill_meta_data(const std::string& interp, Model& m);
    int  find_arity(const std::string& func);
    void blankout(std::string& s) { std::replace( s.begin(), s.end(), ',', ' '); };

    bool build_binop_graph(Model& m, size_t op = 0);

    size_t remove_iso();
};


