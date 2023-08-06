/* model.cpp
 */
#include <sstream>
#include <iostream>
#include "nauty_utils.h"
#include "model.h"

/*
interpretation( 2, [number=1, seconds=0], [
  function(*(_,_), [
    0,0,
    0,1 ]),
  function(/(_,_), [
    1,0,
    1,1 ]),
  function(\(_,_), [
    1,1,
    0,1 ]),
  relation(<(_,_), [
    1,1,
    0,1 ])]).
Read line-by-line from a file
*/

const std::string Model::Interpretation_label = "interpretation";
const std::string Model::Function_label = "function";
const std::string Model::Relation_label = "relation";
const std::string Model::Function_unary_label = "(_)";
const std::string Model::Function_binary_label = "(_,_)";
const std::string Model::Function_stopper = "])";
const std::string Model::Model_stopper = "]).";


Model::Model(size_t odr, std::vector<std::vector<int>>& in_un_ops, 
             std::vector<std::vector<std::vector<int>>>& in_bin_ops,
             std::vector<std::vector<std::vector<int>>>& in_bin_rels) 
       : order(odr), bin_ops(in_bin_ops), un_ops(in_un_ops), bin_rels(in_bin_rels), cg(0)
{
    build_graph();
}

Model::~Model() {
    if (cg != nullptr)
        SG_FREE(*cg);
}

std::string
Model::graph_to_string(sparsegraph* g, const char* sep) const
{
    return put_sg_str(g, sep);
}


bool
Model::operator==(const Model& a) const
{
    return aresame_sg(cg, a.cg);
}

void
Model::print_model(std::ostream& os, bool out_cg) const
{
    os << model_str;
    if (out_cg)
        os << graph_to_string(cg);
}

std::string
Model::find_func_name(const std::string& func)
{
    int start = func.find("(");
    int end = func.find("(", start+1);
    std::string name = func.substr(start+1, end-start-1);
    op_symbols.push_back(name);
    return name;
}

void
Model::fill_meta_data(const std::string& interp)
{
    int start = interp.find("( ");
    int end = interp.find(", ", start+1);
    std::stringstream ss(interp.substr(start+1, end-start-1));
    ss >> order;
    model_str.append(interp);
    model_str.append("\n");
}

int
Model::find_arity(const std::string& func)
{
    size_t arity = 0;
    if (func.find(Function_binary_label) != std::string::npos)
        arity = 2;
    else if (func.find(Function_unary_label) != std::string::npos)
        arity = 1;
    return arity;
}

void
debug_print(std::vector<std::vector<int>>& m)
{
    for (size_t row = 0; row < m.size(); row++) {
        for (size_t col = 0; col < m[row].size(); col++) {
            std::cout << m[row][col] << ",";
        } 
        std::cout << std::endl;
    }
}

bool
Model::parse_model(std::istream& fs, const std::string& check_sym)
{
    /*  The "interpretation" line is already parsed.
     *  check_sym is a comma-delimited list of symbols to parse
        Returns true if success, false otherwise.
     */
    bool done = false;
    while (!done && fs) {
        std::string line;
        getline(fs, line);
        if (line[0] == '%')
            continue;
        bool is_func = line.find(Function_label) != std::string::npos;
        bool is_rel = line.find(Relation_label) != std::string::npos;
        model_str.append(line);
        model_str.append("\n");
        if (is_func || is_rel) {
            int arity = find_arity(line);
	    std::string sym(",");
	    sym.append(find_func_name(line));
	    sym.append(",");
	    bool ignore_op = false;
	    if (!check_sym.empty() && check_sym.find(sym) == std::string::npos)
                ignore_op = true;
            switch (arity) {
            case 1:
                done = parse_unary(line, ignore_op);
                break;
            default:
                done = parse_bin(fs, is_func, ignore_op);
            }
        }
    }
    return true;
}

void
Model::parse_row(std::string& line, std::vector<int>& row)
{
    /* input format:
         0,1,0,  or   0,1,0
       fill the numbers into the vector "row"
    */
    blankout(line);
    std::stringstream ss(line);
    std::string temp;
    int num;
    ss >> temp;
    while (!ss.eof()) {
        if (std::stringstream(temp) >> num)
            row.push_back(num);
        ss >> temp;
    }
}

bool
Model::parse_unary(const std::string& line, bool ignore_op)
{
    /* sample line:
       function('(_), [0,1,2 ]),
    */
    bool end_of_model = false;
    if (line.find(Model_stopper) != std::string::npos) 
        end_of_model = true;

    size_t start = line.find("[");
    size_t end = line.find("]");
    std::string row_str = line.substr(start+1, end - start - 1);
    std::vector<int> row;
    parse_row(row_str, row);
    if (!ignore_op)
        un_ops.push_back(row); 

    return end_of_model;    
}

bool
Model::parse_bin(std::istream& fs, bool is_func, bool ignore_op)
{
    /* This function parse a 2-d function or relation.
       The "[" token is already seen before coming to this function 
       On return, the line containing "])" tokens will have been removed
       The binary operation will be pushed to m.bin_ops
       Lines starting with % are comments, and are ignored.
       Returns true if all functions/relations are extracted, false otherwise */
    
    // TODO: add checking for correct # of rows etc
    bool done = false;
    bool end_model = false;
    std::vector<std::vector<int>>  two_d;
    std::vector<int>  row;
    while (!done && fs) {
        std::string line;
        getline(fs, line);
        if (line[0] == '%')
            continue;
        model_str.append(line);
        model_str.append("\n");
        if (line.find(Function_stopper) != std::string::npos) {
            done = true;
            if (line.find(Model_stopper) != std::string::npos)
                end_model = true;
        }
	if (!ignore_op) {
            parse_row(line, row);
            two_d.push_back(row);
            row.clear();
	}
    }
    if (!ignore_op) {
        if (is_func)
            bin_ops.push_back(two_d);
        else
            bin_rels.push_back(two_d);
    }
    return end_model;
}
 

size_t
Model::find_graph_size(size_t& num_vertices, size_t& num_edges)
{

    size_t num_ternary_ops = ternary_ops.size();
    size_t num_bin_ops = bin_ops.size();
    size_t num_bin_rels = bin_rels.size();
    size_t num_unary_ops = un_ops.size();
    size_t max_arity = num_bin_ops + num_bin_rels > 0 ? 2 : 1;

    // vertices
    // vertices for domain elements
    num_vertices = 2 * order;  // vertices for E and F

    if (num_unary_ops + num_bin_ops > 0) 
        num_vertices += order;       // vertices for R

    if (num_bin_ops > 0 || num_bin_rels > 0 || num_ternary_ops > 0) {
        num_vertices += order;       // vertices for S
    }
    if (num_bin_rels > 0) {
        num_vertices += 2;   // for True and False
    }
    // vertices for op/relation tables
    num_vertices += num_unary_ops * order;
    num_vertices += (num_bin_ops + num_bin_rels) * order * order;

    // debug print
    // std::cout << "debug, find_graph_size:  Number of vertices " << num_vertices << std::endl;

    // edges
    // edges for domain elements
    num_edges = order;      // undirected edge E to F
    if (num_unary_ops + num_bin_ops > 0) 
        num_edges += order;     // undirected edges from E to R
   
    if (num_bin_ops + num_bin_rels > 0)
        num_edges += order;     // undirected edge E to S

    // edges for op tables
    num_edges += 2 * order * num_unary_ops;
    num_edges += 3 * order * order * (num_bin_rels + num_bin_ops);

    num_edges *= 2;   // directed edges
    // debug print
    // std::cout << "debug:  Number of edges " << num_edges << std::endl;

    return max_arity;                                                                       
}

void
Model::color_vertices(int* ptn, int* lab, int ptn_sz)
{
    /*  vertices must be assigned in the order E, R, L, F, S, un_ops, bin_ops, bin_rels
     */
    // colors
    for (size_t idx=0; idx < ptn_sz; ++idx) {
        ptn[idx] = 1;
        lab[idx] = idx;
    }

    size_t color_end = 0;
    // E segments
    color_end += order;
    ptn[color_end - 1] = 0;

    // R segment
    if (bin_ops.size() + un_ops.size() > 0) {
        color_end += order;
        ptn[color_end - 1] = 0;
    }

    // L, if exists, each true and false is has color of its own
    if (bin_rels.size() > 0) {
        color_end += 2;
        ptn[color_end-2] = 0;
        ptn[color_end-1] = 0;
    }

    // F segments
    color_end += order;
    ptn[color_end - 1] = 0;

    // S, if exists
    if (bin_ops.size() + bin_rels.size() > 0) {
        color_end += order;
        ptn[color_end-1] = 0;
    }
 
    // op tables
    for (auto op : un_ops) {
        color_end += order;
        ptn[color_end-1] = 0;
    }
    for (auto op : bin_ops) {
        color_end += order * order;
        ptn[color_end-1] = 0;
    }
    for (auto op : bin_rels) {
        color_end += order * order;
        ptn[color_end-1] = 0;
    }
    // debug print
    // std::cout << "color array size: " << color_end << " space allocated: " << ptn_sz << std::endl;
    // for (size_t idx=0; idx < ptn_sz; ++idx)
    //     std::cout << ptn[idx] << " ";
    // std::cout << std::endl;
}


void
Model::count_truth_values(std::vector<size_t>& L_v_count)
{
    // count number of times true or false appears in the relations tables.
    // -1 means the cell is not occupied
    for (auto op : bin_rels) {
        for (auto row : op) {
            for (auto v : row) {
                if (v != -1)
                    L_v_count[v]++;
            }
        }
    }
}

void
Model::count_occurrences(std::vector<size_t>& R_v_count)
{
    // count number of times a domain element appears in the op tables.
    // -1 means the cell is not occupied
    for (auto op : un_ops) {
        for (auto v : op) {
            if (v != -1)
                R_v_count[v]++;
        }
    }
    for (auto op : bin_ops) {
        for (auto row : op) {
            for (auto v : row) {
                if (v != -1)
                    R_v_count[v]++;
            }
        }
    }
    for (auto op : ternary_ops) {  // not supported yet: 2023/07/26
        for (auto first_arg : op) {
            for (auto secord_arg : first_arg) {
                for (auto v : secord_arg)  {
                    if (v != -1)
                        R_v_count[v]++;
                }
            }
        }
    }
}

void
Model::build_vertices(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v, const int L_v, const int A_c)
{
    std::vector<size_t> R_v_count(order, 0);
    count_occurrences(R_v_count);
    std::vector<size_t> L_v_count(2, 0);
    count_truth_values(L_v_count);

    // set up vertices for E, F, S, T, R and L
    // each node in E points to F, S, T R and L and to each of the cells in a row
    // S and T may not exist (ie. value zero)
    const size_t num_F = un_ops.size();
    const size_t num_S = bin_ops.size() + bin_rels.size();
    const size_t num_T = ternary_ops.size();     // 7/29/2023, not supported yet
    bool has_S = num_S > 0;
    bool has_R = (un_ops.size() + bin_ops.size()) > 0;

    const size_t E_outd = 1 + (has_S? 1 : 0) + (bin_ops.size() > 0? 1 : 0);    //num of out edges per vertex
    const size_t E_size = E_outd * order;
    const size_t F_outd = num_F + num_S * order + 1;  // out-degree of first arg
    const size_t F_size = F_outd * order;
    const size_t S_outd = num_S * order + 1;          // out-degree of second arg
    const size_t S_size = num_S? S_outd * order : 0;
    size_t L_pos = E_size + F_size + S_size;
    const size_t L_size = bin_rels.size() * order * order;   // L (true/false) does not point back to E
    size_t R_pos = L_pos + L_size;

    /* debug print
    std::cout << "E_size " << E_size << " F_size " << F_size << " S_size " << S_size << " L_size " << L_size << std::endl;
    std::cout << "debug: start R_pos: " << R_pos << "  L_pos: " << L_pos << " L_size: " << L_size << std::endl;
    */
    for (size_t idx = 0; idx < order; ++idx) {
        sg1.v[E_e+idx] = E_outd * idx;
        sg1.d[E_e+idx] = E_outd;                         // out-degree
        sg1.v[F_a+idx] = E_size + F_outd * idx;
        sg1.d[F_a+idx] = F_outd;                         // out-degree
        if (num_S > 0) {
            sg1.v[S_a+idx] = E_size + F_size + S_outd * idx;
            sg1.d[S_a+idx] = S_outd;                     // out-degree
        }

        if (has_R) {
            sg1.v[R_v+idx] = R_pos;
            sg1.d[R_v+idx] = R_v_count[idx] + 1;
            R_pos += sg1.d[R_v+idx];
        }
    }
    // debug print
    // std::cout << "debug: ending R_pos: " << R_pos << "  A_c " << A_c << std::endl;

    // set up vertices for rel values (true/false)
    if (bin_rels.size() > 0) {
        for( size_t idx=0; idx < 2; ++idx) {
            sg1.v[L_v + idx] = L_pos; 
            sg1.d[L_v + idx] = L_v_count[idx];
            L_pos += sg1.d[L_v + idx];
        }
    }

    // set up vertices of op/rel tables
    size_t A_c_el = A_c;
    size_t A_c_pos = R_pos;

    // unary op tables
    for (auto op : un_ops) {
        for (auto cell : op) {
            sg1.v[A_c_el] = A_c_pos;
            sg1.d[A_c_el] = 2;
            A_c_pos += sg1.d[A_c_el];
            A_c_el++;
        }
    }
    // debug print
    // std::cout << "un_ops. Domain element: " << A_c_el << " edge pos (A_c_pos) " << A_c_pos << std::endl;

    // binary op tables
    for (auto op : bin_ops) {
        for (auto row : op) {
            for (auto cell : row) {
                sg1.v[A_c_el] = A_c_pos;
                sg1.d[A_c_el] = 3;
                A_c_pos += sg1.d[A_c_el];
                A_c_el++;
            }
        }
    }
    // debug print
    // std::cout << "bin_ops. Domain element: " << A_c_el << " edge pos " << A_c_pos << std::endl;

    // binary rel tables
    for (auto op : bin_rels) {
        for (auto row : op) {
            for (auto cell : row) {
                sg1.v[A_c_el] = A_c_pos;
                sg1.d[A_c_el] = 3;
                A_c_pos += sg1.d[A_c_el];
                A_c_el++;
            }
        }
    }
    // debug print
    // std::cout << "bin_rels. Domain element: " << A_c_el << " edge pos " << L_pos << " " << A_c_pos << std::endl;

    // ternary op tables - not supported, 7/26/2023
}

void
Model::debug_print_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v, const int A_c, bool has_S)
{
    for (size_t idx = 0; idx < order; ++idx) {
        std::cout << E_e+idx << "|" << sg1.e[sg1.v[E_e+idx]] << "  " << sg1.e[sg1.v[E_e+idx]+1] << "  " << sg1.e[sg1.v[E_e+idx]+2] << "  ";
    }
    std::cout << std::endl;
    for (size_t idx = 0; idx < order; ++idx) {
        std::cout << R_v+idx << "|";
        for (size_t jdx = 0; jdx < sg1.d[R_v+idx]; ++jdx)
            std::cout << sg1.e[sg1.v[R_v+idx]+jdx] << "  ";
    }
    std::cout << std::endl;
    for (size_t idx = 0; idx < order; ++idx) {
        std::cout << "**d " << sg1.d[F_a+idx] << " " << F_a+idx << "|" << sg1.e[sg1.v[F_a+idx]] << "  " << sg1.e[sg1.v[F_a+idx]+1] << " " << sg1.e[sg1.v[F_a+idx]+2] << " ";
    }
    std::cout << std::endl;
    for (size_t idx = 0; idx < order; ++idx) {
        std::cout << "**d " << sg1.d[S_a+idx] << " " << S_a+idx << "|" << sg1.e[sg1.v[S_a+idx]] << "  " << sg1.e[sg1.v[S_a+idx]+1] << "  " << sg1.e[sg1.v[S_a+idx]+2] << " ";
    }
    std::cout << std::endl;
    for (size_t idx = 0; idx < order*order; ++idx) {
        std::cout << "**d " << sg1.d[A_c+idx] << " " << A_c+idx << "|" << sg1.e[sg1.v[A_c+idx]] << "  " << sg1.e[sg1.v[A_c+idx]+1] << "  " << sg1.e[sg1.v[A_c+idx]+2] << " ";
    }
    std::cout << std::endl;
}


void
Model::build_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v, const int L_v, const int A_c)
{
    const size_t num_S = bin_ops.size() + bin_rels.size();

    // E to R, F, S, and T 
    for (size_t idx = 0; idx < order; ++idx) {
        sg1.e[sg1.v[E_e+idx]] = F_a+idx;
        sg1.e[sg1.v[F_a+idx]] = E_e+idx;
        size_t epos = 1;
        // joining R and E
        if (un_ops.size() + bin_ops.size() > 0) {
            sg1.e[sg1.v[E_e+idx]+epos] = R_v+idx;
            sg1.e[sg1.v[R_v+idx]] = E_e+idx;
            epos++;
        }
        // joining S and E
        if (num_S > 0) {
            sg1.e[sg1.v[E_e+idx]+epos] = S_a+idx;
            sg1.e[sg1.v[S_a+idx]] = E_e+idx;
            epos++;
        }
         /* debug print
         std::cout << "Writing first pos " << sg1.v[E_e+idx] << " " << sg1.v[F_a+idx]
               << " " << sg1.v[E_e+idx]+1 << " " << sg1.v[R_v+idx] << " " << sg1.v[E_e+idx]+2 << " " 
               << " " << sg1.v[S_a+idx] << std::endl;
        */
    }

    // A_abc, a*b=c ->  edges from A_abc to F_a, S_b, and R_c or L_c
    size_t A_c_el = A_c;
    std::vector<size_t> F_a_pos(order, 1);   // First position of F_a points to E_e
    std::vector<size_t> S_a_pos(order, 1);   // First position of S_a points to E_e
    std::vector<size_t> R_v_pos(order, 1);   // First position of R_v points to E_e
    std::vector<size_t> L_v_pos(2, 0);       // L_v does not point to E_e

    for (size_t op=0; op < un_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {

            sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
            sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

            int cval = un_ops[op][f_arg];
            if (cval == -1 ) {
                sg1.d[A_c_el]--;
            }
            else {
                sg1.e[sg1.v[A_c_el]+1] = R_v + cval;
                sg1.e[sg1.v[R_v+cval]+R_v_pos[cval]] = A_c_el; 
                R_v_pos[cval]++;
            }
            
            F_a_pos[f_arg]++;
            A_c_el++;
        }
    }
 
    for (size_t op=0; op < bin_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            for (size_t s_arg=0; s_arg < order; ++s_arg) {
                sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
                sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+1] = S_a + s_arg;
                sg1.e[sg1.v[S_a+s_arg]+S_a_pos[s_arg]] = A_c_el; 

                int cval = bin_ops[op][f_arg][s_arg];
		if (cval == -1) {
                    sg1.d[A_c_el]--;
                }
                else {
                    sg1.e[sg1.v[A_c_el]+2] = R_v + cval;
                    sg1.e[sg1.v[R_v+cval]+R_v_pos[cval]] = A_c_el; 
                    R_v_pos[cval]++;
                }
                /* debug print
                std::cout << "Writing R pos " << sg1.v[A_c_el] << " " << sg1.v[F_a+f_arg]+F_a_pos[f_arg]
                   << " " << sg1.v[A_c_el]+1 << " " << sg1.v[S_a+s_arg]+S_a_pos[s_arg]
                   << " " << sg1.v[A_c_el]+2 << " " << sg1.v[R_v+cval]+R_v_pos[cval] << std::endl;
                */
            
                F_a_pos[f_arg]++;
                S_a_pos[s_arg]++;
                A_c_el++;
            }
        }
    }

    for (size_t op=0; op < bin_rels.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            for (size_t s_arg=0; s_arg < order; ++s_arg) {
                sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
                sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+1] = S_a + s_arg;
                sg1.e[sg1.v[S_a+s_arg]+S_a_pos[s_arg]] = A_c_el; 

                int cval = bin_rels[op][f_arg][s_arg];
		if (cval == -1) {
                    sg1.d[A_c_el]--;
                }
                else {
                    sg1.e[sg1.v[A_c_el]+2] = L_v + cval;
                    sg1.e[sg1.v[L_v+cval]+L_v_pos[cval]] = A_c_el; 
                    L_v_pos[cval]++;
                }
                /* debug print
                std::cout << A_c_el << " " << L_v+cval << " Writing L pos " << sg1.v[A_c_el] << " " << sg1.v[F_a+f_arg]+F_a_pos[f_arg]
                   << " " << sg1.v[A_c_el]+1 << " " << sg1.v[S_a+s_arg]+S_a_pos[s_arg]
                   << " " << sg1.v[A_c_el]+2 << " " << sg1.v[L_v+cval]+L_v_pos[cval] << std::endl;
                */
            
                F_a_pos[f_arg]++;
                S_a_pos[s_arg]++;
                A_c_el++;
                // debug print: std::cout << "Next pos: " << A_c_el << " F " << F_a_pos[f_arg] << " S " << S_a_pos[s_arg] << " L " <<  L_v_pos[cval] << std::endl;
            }
        }
    }
}

bool
Model::build_graph()
{
    /*  7/26/2023: supports only binary and unary operations
        E represents the domain elements
        F represents the first argument of an operation
        S represents the second argument of an operation
        R represents the function value (or result of applying the function)
        L represents the relation value (0/1 for false/true)
        A represents the operation table
     */
    bool   has_rel = bin_rels.size() > 0;
    bool   has_func = bin_ops.size() + un_ops.size() > 0;
    if (!has_rel && !has_func)   // empty graph
        return false;
    size_t num_vertices, num_edges;
    size_t max_arity = find_graph_size(num_vertices, num_edges);
    bool   has_S = max_arity > 1;
    // debug print
    // std::cout << "num_vertices: " << num_vertices << " num_edges: " << num_edges << std::endl;

    DYNALLSTAT(int,lab,lab_sz);
    DYNALLSTAT(int,ptn,ptn_sz);
    DYNALLSTAT(int,orbits,orbits_sz);
    DYNALLSTAT(int,map,map_sz);
    static DEFAULTOPTIONS_SPARSEDIGRAPH(options);
    statsblk stats;

    /* Declare and initialize sparse graph structures */
    SG_DECL(sg1);
    SG_DECL(cg1);

   /* Select option for canonical labelling */

    options.getcanon = TRUE;
    options.defaultptn = FALSE;

    int mx = SETWORDSNEEDED(num_vertices);
    nauty_check(WORDSIZE,mx,num_vertices,NAUTYVERSIONID);
    // debug print
    // std::cout << "WORDSIZE " << WORDSIZE << " return value for SETWORDSNEEDED(num_vertices) " << mx << std::endl;

    DYNALLOC1(int,lab,lab_sz,num_vertices,"malloc");
    DYNALLOC1(int,ptn,ptn_sz,num_vertices,"malloc");
    DYNALLOC1(int,orbits,orbits_sz,num_vertices,"malloc");
    DYNALLOC1(int,map,map_sz,num_vertices,"malloc");

    // debug print
    // std::cout << "lab_sz " << lab_sz  << " ptn_sz " << ptn_sz << " orbits_sz " << orbits_sz << " map_sz " << map_sz << std::endl;

    // make the graph
    SG_ALLOC(sg1,num_vertices,num_edges,"malloc");
    sg1.nv = num_vertices;     // Number of vertices
    sg1.nde = num_edges;       // Number of directed edges = twice of # undirected edges here

    // vertices, offsets by domain element number to make them unique
    // E.g. E_e+2 represents domain element 2, F_a represents first function_argument (row) 2,
    // S_a+2 represents the same 2, but second function argument (column) 2,
    // R_v+2 represents the same 2 (cell value) in the op table cell
    // A_c+2 represents the table cell (0, 2) (the cell position, not the value in the cell).
    // L_v+0,1 represents 0 or 1.  They cannot be renamed (moved) because they represents false/true
    int        ptr = 0;
    const int  E_e = ptr;
    ptr += order;

    const int  R_v = ptr;
    if (has_func) ptr += order;

    const int  L_v = ptr;
    if (has_rel) ptr += 2;

    const int  F_a = ptr;
    ptr += order;

    const int  S_a = ptr;
    if (has_S) ptr += order;

    const int  A_c = ptr;

    // debug print
    // std::cout << "E R L F S A: " << E_e << " " << R_v << " " << L_v << " " << F_a << " " << S_a << " " << A_c << std::endl;

    // vertices
    build_vertices(sg1, E_e, F_a, S_a, R_v, L_v, A_c);

    // color the graph
    color_vertices(ptn, lab, ptn_sz);
    /* debug print
    // std::cout << " space allocated: " << ptn_sz << " " << lab_sz << std::endl;
    for (size_t idx=0; idx < lab_sz; ++idx) 
        std::cout << lab[idx] << " ";
    std::cout << std::endl;
    for (size_t idx=0; idx < ptn_sz; ++idx)
        std::cout << ptn[idx] << " ";
    std::cout << std::endl;
    */

    // edges
    build_edges(sg1, E_e, F_a, S_a, R_v, L_v, A_c);

    /* debug print
    // debug_print_edges(sg1, E_e, F_a, S_a, R_v, L_v, A_c);
    std::cout << "sg1:\n" << graph_to_string(&sg1) << std::endl;
    */

    // compute canonical form
    sparsenauty(&sg1,lab,ptn,orbits,&options,&stats,&cg1);

    // debug print
    // std::cout << graph_to_string(&cg1) << std::endl;

    sortlists_sg(&cg1);
    // debug print
    // std::cout << graph_to_string(&cg1) << std::endl;

    cg = copy_sg(&cg1, NULL);
    SG_FREE(sg1);
    return true;
}

