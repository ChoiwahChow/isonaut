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
const std::string Model::Function_arity_label = "_";
const std::string Model::Function_unary_label = "(_)";
const std::string Model::Function_binary_label = "(_,_)";
const std::string Model::Function_stopper = "])";
const std::string Model::Model_stopper = "]).";

// ; for model separator.
// ? for unassigned.
const char Model::Base64Table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};


Model::Model(size_t odr, std::vector<int>& constants,
             std::vector<std::vector<int>>& in_un_ops, 
             std::vector<std::vector<std::vector<int>>>& in_bin_ops,
             std::vector<std::vector<std::vector<int>>>& in_bin_rels,
             bool save_cg) 
       : order(odr), constants(constants), bin_ops(in_bin_ops), un_ops(in_un_ops), bin_rels(in_bin_rels), 
         el_fixed_width(1), cg(nullptr), num_unassigned(0), save_cg(save_cg) 
{
    set_width(odr);
    build_graph(save_cg);
}

Model::~Model() {
    if (cg != nullptr)
        SG_FREE(*cg);
}

std::string
Model::graph_to_string(sparsegraph* g, const char* sep, bool shorten) const
{
    return put_sg_str(g, sep, shorten);
}

std::string
Model::graph_to_shortened_string(sparsegraph* g) const
{
    return compressed_sg_str(order, g);
}


bool
Model::operator==(const Model& a) const
{
    return aresame_sg(cg, a.cg);
}

void
Model::print_model(std::ostream& os, const std::string& canon_str, bool out_cg) const
{
    os << model_str;
    if (out_cg)
        os << canon_str << std::endl;
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
Model::set_width(size_t order)
{
    size_t base = 64;
    if (order > 64) {
        base *= 64;
        el_fixed_width++;
        while (base < order)  {
            el_fixed_width++;
            base *= 64;
        }
    }
}

void
Model::fill_meta_data(const std::string& interp)
{
    int start = interp.find("( ");
    int end = interp.find(", ", start+1);
    std::stringstream ss(interp.substr(start+1, end-start-1));
    ss >> order;
    set_width(order);
    model_str.append(interp);
    model_str.append("\n");
}

int
Model::find_arity(const std::string& func)
{
    size_t arity = 3;
    if (func.find(Function_binary_label) != std::string::npos)
        arity = 2;
    else if (func.find(Function_unary_label) != std::string::npos)
        arity = 1;
    else if (func.find(Function_arity_label) == std::string::npos)
        arity = 0;
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
            case 0:
                break;
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
    std::string row_str = line.substr(start+1, end - start - 1) + " ";
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
        size_t end_pos = line.find(Function_stopper);
        if (end_pos != std::string::npos) {
            done = true;
            line.insert(end_pos, " ");
            if (line.find(Model_stopper) != std::string::npos)
                end_model = true;
        }
        model_str.append(line);
        model_str.append("\n");
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
Model::count_unassigned()
{
    size_t count = 0;
    for (auto v : constants) {
        if (v == -1)
            ++count;
    }
    for (auto op : un_ops) {
        for (auto v : op) {
            if (v == -1)
                ++count;
        }
    }
    for (auto op : bin_ops) {
        for (auto row : op) {
            for (auto v : row) {
                if (v == -1)
                    ++count;
            }
        }
    }
    for (auto op : bin_rels) {
        for (auto row : op) {
            for (auto v : row) {
                if (v == -1)
                    ++count;
            }
        }
    }
    return count;    
}

size_t
Model::find_graph_size(size_t& num_vertices, size_t& num_edges)
{
    size_t num_ternary_ops = ternary_ops.size();
    size_t num_bin_ops = bin_ops.size();
    size_t num_bin_rels = bin_rels.size();
    size_t num_unary_ops = un_ops.size();
    size_t num_constants = constants.size();
    size_t max_arity = num_bin_ops + num_bin_rels > 0 ? 2 : (num_unary_ops > 0? 1 : 0);

    if (max_arity == 0) {
        std::cerr << "find_graph_size: no unary or binary op/relations, abourt." << std::endl;
        return 0;
    }
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

    // uassigned vertex
    if (num_unassigned > 0)
        num_vertices++;

    // vertices for op/relation tables
    num_vertices += num_constants;
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
    num_edges += num_constants;
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
    /*  vertices must be assigned in the order E, R, L, U, F, S, un_ops, bin_ops, bin_rels
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
    if (num_unassigned > 0) {
        color_end++;
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
    for (auto op : constants) {
        color_end += 1;
        ptn[color_end-1] = 0;
    }
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
    /* debug print
    std::cout << "color array size: " << color_end << " space allocated: " << ptn_sz << std::endl;
    for (size_t idx=0; idx < ptn_sz; ++idx)
        std::cout << ptn[idx] << " ";
    */
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
    for (auto v : constants) {
        if (v != -1)
            R_v_count[v]++;
    }
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
    for (auto op : ternary_ops) {  // not supported yet: 2023/09/10
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

size_t
Model::count_unassigned_rels()
{
    size_t count = 0;
    for (auto op : bin_rels) {
        for (auto row : op ) {
            for (auto v : row) {
                if (v == -1)
                    ++count;
            }
        }
    }

    return count;
}

void
Model::build_vertices(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, 
                      const int R_v, const int L_v, const int U_v, const int A_c)
{
    std::vector<size_t> R_v_count(order, 0);
    count_occurrences(R_v_count);
    std::vector<size_t> L_v_count(2, 0);
    count_truth_values(L_v_count);
    size_t num_unassigned_rels = count_unassigned_rels();

    // set up vertices for E, F, S, R, L, and U
    // each node in E points to F, S, R and to each of the cells in a row
    // E does not point to L (true/false, not domain elements), or unassigned
    // L, U, and S may not exist (ie. value zero)
    const size_t num_S = bin_ops.size() + bin_rels.size();
    bool has_R = (constants.size() + un_ops.size() + bin_ops.size()) > 0;

    // E points to first arg, second arg (if exists) and results (if exists)
    const size_t E_outd = 1 + (num_S > 0? 1 : 0) + (has_R? 1 : 0);    //num of out edges per vertex
    const size_t E_size = E_outd * order;
    const size_t F_outd = un_ops.size() * 1 + num_S * order + 1;  // out-degree of first arg
    const size_t F_size = F_outd * order;
    const size_t S_outd = num_S * order + 1;          // out-degree of second arg
    const size_t S_size = num_S? S_outd * order : 0;
    // U may not exist
    size_t U_pos = E_size + F_size + S_size;
    const size_t U_size = num_unassigned;
    size_t L_pos = U_pos + U_size;
    // L may not exist
    const size_t L_size = bin_rels.size() * order * order - num_unassigned_rels;   // L (true/false) does not point back to E
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

    // set up the vertex for unassigned
    if (num_unassigned > 0) {
        sg1.v[U_v] = U_pos;
        sg1.d[U_v] = num_unassigned;
    }

    // set up vertices for rel values (true/false)
    if (bin_rels.size() > 0) {
        for (size_t idx=0; idx < 2; ++idx) {
            sg1.v[L_v + idx] = L_pos; 
            sg1.d[L_v + idx] = L_v_count[idx];
            L_pos += sg1.d[L_v + idx];
        }
    }

    // set up vertices of op/rel tables
    // vertices are arranged in ascending order of arity: constants, unary, binary, ...
    size_t A_c_el = A_c;
    size_t A_c_pos = R_pos;

    // constants
    for (auto u : constants) {
        sg1.v[A_c_el] = A_c_pos;
        sg1.d[A_c_el] = 1;
        A_c_pos += sg1.d[A_c_el];
        A_c_el++;
    }
 
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
Model::build_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v,
                   const int L_v, const int U_v, const int A_c)
{
    // E and F always exist
    const size_t num_S = bin_ops.size() + bin_rels.size();

    // E to R, F, and S
    for (size_t idx = 0; idx < order; ++idx) {
        // joining F and E
        sg1.e[sg1.v[E_e+idx]] = F_a+idx;
        sg1.e[sg1.v[F_a+idx]] = E_e+idx;
        size_t epos = 1;
        // joining R and E
        if (constants.size() + un_ops.size() + bin_ops.size() > 0) {
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
        std::cerr << "debug Writing first pos " << sg1.v[E_e+idx] << " " << sg1.v[F_a+idx]
               << " " << sg1.v[E_e+idx]+1 << " " << sg1.v[R_v+idx] << " " << sg1.v[E_e+idx]+2 << " " 
               << " " << sg1.v[S_a+idx] << std::endl;
        */
    }

    // A_abc, a*b=c ->  edges from A_abc to F_a, S_b, and R_c or L_c or U_v
    size_t A_c_el = A_c;
    std::vector<size_t> F_a_pos(order, 1);   // First position of F_a points to E_e
    std::vector<size_t> S_a_pos(order, 1);   // First position of S_a points to E_e
    std::vector<size_t> R_v_pos(order, 1);   // First position of R_v points to E_e
    std::vector<size_t> L_v_pos(2, 0);       // L_v does not point to E_e
    size_t              U_v_pos = 0;         // U_v does not point to E_e

    for (auto cval : constants) {
        if (cval == -1 ) {
            sg1.e[sg1.v[A_c_el]] = U_v;
            sg1.e[sg1.v[U_v]+U_v_pos] = A_c_el; 
            U_v_pos++;
        }
        else {
            sg1.e[sg1.v[A_c_el]] = R_v + cval;
            sg1.e[sg1.v[R_v+cval]+R_v_pos[cval]] = A_c_el; 
            R_v_pos[cval]++;
        }
        A_c_el++;
    }

    for (size_t op=0; op < un_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
            sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

            int cval = un_ops[op][f_arg];
            if (cval == -1 ) {
                sg1.e[sg1.v[A_c_el]+1] = U_v;
                sg1.e[sg1.v[U_v]+U_v_pos] = A_c_el; 
                U_v_pos++;
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
    // debug print
    // std::cerr << "un ops done" << std::endl;
 
    for (size_t op=0; op < bin_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            for (size_t s_arg=0; s_arg < order; ++s_arg) {
                sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
                sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+1] = S_a + s_arg;
                sg1.e[sg1.v[S_a+s_arg]+S_a_pos[s_arg]] = A_c_el; 

                int cval = bin_ops[op][f_arg][s_arg];
		if (cval == -1) {
                    sg1.e[sg1.v[A_c_el]+2] = U_v;
                    sg1.e[sg1.v[U_v]+U_v_pos] = A_c_el; 
                    U_v_pos++;
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
    // debug print
    // std::cerr << "bin ops done" << std::endl;

    for (size_t op=0; op < bin_rels.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            for (size_t s_arg=0; s_arg < order; ++s_arg) {
                sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
                sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+1] = S_a + s_arg;
                sg1.e[sg1.v[S_a+s_arg]+S_a_pos[s_arg]] = A_c_el; 

                int cval = bin_rels[op][f_arg][s_arg];
		if (cval == -1) {
                    sg1.e[sg1.v[A_c_el]+2] = U_v;
                    sg1.e[sg1.v[U_v]+U_v_pos] = A_c_el; 
                    U_v_pos++;
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
    // debug print
    // std::cerr << "bin rels done" << std::endl;
}

bool
Model::build_graph(bool save_cg)
{
    /*  8/26/2023: supports only constants, binary and unary operations
        E represents the domain elements
        F represents the first argument of an operation
        S represents the second argument of an operation
        R represents the function value (or result of applying the function)
        L represents the relation value (0/1 for false/true)
        A represents the operation table
     */
    num_unassigned = count_unassigned();

    bool   has_rel = bin_rels.size() > 0;
    bool   has_func = bin_ops.size() + un_ops.size() + constants.size() > 0;
    if (!has_rel && !has_func)   // empty graph
        return false;
    size_t num_vertices, num_edges;
    size_t max_arity = find_graph_size(num_vertices, num_edges);
    if (max_arity < 1) {
        std::cerr << "build_graph:  Does not support models with only constants" << std::endl;
        return false;
    }
    bool   has_S = max_arity > 1;
    // debug print
    //std::cerr << "debug num_vertices: " << num_vertices << " num_edges: " << num_edges << std::endl;

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
    // std::cout << "debug: WORDSIZE " << WORDSIZE << " return value for SETWORDSNEEDED(num_vertices) " << mx << std::endl;

    DYNALLOC1(int,lab,lab_sz,num_vertices,"malloc");
    DYNALLOC1(int,ptn,ptn_sz,num_vertices,"malloc");
    DYNALLOC1(int,orbits,orbits_sz,num_vertices,"malloc");
    DYNALLOC1(int,map,map_sz,num_vertices,"malloc");

    // debug print
    // std::cerr << "debug lab_sz " << lab_sz  << " ptn_sz " << ptn_sz << " orbits_sz " << orbits_sz << " map_sz " << map_sz << std::endl;

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
    // U_v     represents unassigned value
    // The vertices must be created the same order as in color_graph: E R L U F S const un_op bin_op bin_rel
    int        ptr = 0;
    const int  E_e = ptr;
    ptr += order;

    const int  R_v = ptr;
    if (has_func) ptr += order;

    const int  L_v = ptr;
    if (has_rel) ptr += 2;

    const int U_v = ptr;
    if (num_unassigned > 0) ptr++;

    const int  F_a = ptr;
    ptr += order;

    const int  S_a = ptr;
    if (has_S) ptr += order;

    const int  A_c = ptr;

    // debug print
    //std::cerr << "debug E R L U F S A: " << E_e << " " << R_v << " " << L_v << " " 
    //            << U_v << " " << F_a << " " << S_a << " " << A_c << std::endl;

    // vertices
    build_vertices(sg1, E_e, F_a, S_a, R_v, L_v, U_v, A_c);

    // color the graph
    color_vertices(ptn, lab, ptn_sz);
    /* debug print
    std::cerr << " debug space allocated: " << ptn_sz << " " << lab_sz << std::endl;
    for (size_t idx=0; idx < lab_sz; ++idx) 
        std::cout << lab[idx] << " ";
    std::cout << std::endl;
    for (size_t idx=0; idx < ptn_sz; ++idx)
        std::cout << ptn[idx] << " ";
    std::cerr << std::endl;
    */

    // edges
    build_edges(sg1, E_e, F_a, S_a, R_v, L_v, U_v, A_c);
    /* debug print
    // debug_print_edges(sg1, E_e, F_a, S_a, R_v, L_v, A_c);
    std::cerr << "debug sg1:\n" << graph_to_string(&sg1) << std::endl;
    */

    // compute canonical form
    sparsenauty(&sg1,lab,ptn,orbits,&options,&stats,&cg1);

    // debug print
    // std::cerr << "debug, graph string: " << graph_to_string(&cg1) << std::endl;

    for (size_t iptr = 0; iptr < order; ++iptr)
        iso.push_back(lab[iptr]);

    sortlists_sg(&cg1);
    // debug print
    // std::cerr << "debug, cg string: " << std::endl;  // << graph_to_string(&cg1) << std::endl;

    SG_FREE(sg1);
    if (save_cg)
        cg = copy_sg(&cg1, NULL);
    SG_FREE(cg1);
    return true;
}

size_t
Model::compress_str(int label, size_t width, std::string& str) const 
{
/*
    if (label < 0)
        return 0;
*/

    size_t slen = 0;
    if (label >= 0) {
        int r = label;
        int q = 1;  // to start to loop
        while (q > 0) {
            if (r >= 64) {
                q = r >> 6;
                r = r - (q << 6);
            }
            else
                q = 0;
            str += Base64Table[r];
            ++slen;
            r = q;
        }
        ++slen;
    }
    else {
        slen = 1;
        str += unassigned;
    }
    while (slen < width) {
        str += padding;
        ++slen;
    }
    return slen;
}


int
Model::get_cell_value(const std::vector<size_t>& inv, int val) 
{
    if (val >= 0)
        return inv[val];
    else
        return val;
}

void
Model::remove_unassigned(std::string& cms) const
{
    size_t pos = cms.find_first_not_of(unassigned);
    if (pos != 0 && pos != std::string::npos) {
        cms.erase(cms.begin(), cms.begin()+pos);
    }
    cms.push_back(op_end);
}

void
Model::compress_small_str(bool is_even, int val, std::string& cms) const
{
    unsigned char x = 0;
    if (val < 0)
        x = 0x0F;
    else
        x = val;
    if (is_even) {
        x <<= 4;
        cms.push_back(x);
    }
    else
        cms[cms.size() - 1] |= x;
}

std::string
Model::compress_cms() const
{
    std::vector<size_t> inv(order, 0);
    // find inverse of the isomorphism that maps the vectors to canonical form
    for (size_t v = 0; v < order; ++v) 
        inv[iso[v]] = v;

    std::string cms;
    for (auto bo : bin_ops) {
        bool is_even = true;
        for (size_t r = 0; r < order; ++r) {
            for (size_t c = 0; c < order; ++c) {
                int v = get_cell_value(inv, bo[iso[r]][iso[c]]);
                if (order > 4 && order < 16) {
                    compress_small_str(is_even, v, cms);
                    is_even = !is_even;
                }
                else 
                    compress_str(v, el_fixed_width, cms);
            }
        }
        //while (cms[cms.size()-1] == unassigned)
        //    cms.erase(cms.length()-1);
        if (order > 8 && order < 16)
            cms.push_back(op_end);
        else
            remove_unassigned(cms);
        //cms.push_back(op_end);
    }
    for (auto bo : bin_rels) {
        for (size_t r = 0; r < order; ++r) {
            for (size_t c = 0; c < order; ++c) {
                int v = bo[iso[r]][iso[c]];
                compress_str(v, 1, cms);
            }
        }
        //while (cms[cms.size()-1] == unassigned)
        //    cms.erase(cms.length()-1);
        remove_unassigned(cms);
        //cms.push_back(op_end);
    }
    for (auto uo : un_ops) {
        for (size_t r = 0; r < order; ++r ) {
            int v = get_cell_value(inv, uo[iso[r]]);
            compress_str(v, el_fixed_width, cms);
        }
        //while (cms[cms.size()-1] == unassigned)
        //    cms.erase(cms.length()-1);
        remove_unassigned(cms);
        //cms.push_back(op_end);
    }
    for (auto cst : constants) {
        int v = get_cell_value(inv, cst);
        compress_str(v, el_fixed_width, cms);
        //while (cms[cms.size()-1] == unassigned)
        //    cms.erase(cms.length()-1);
        //cms.push_back(op_end);
        remove_unassigned(cms);
    }
    if (!cms.empty())
        cms.pop_back();
    return cms;
}

