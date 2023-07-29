
#include <sstream>
#include <iostream>
#include "isofilter.h"
#include "nauty_utils.h"

/*
interpretation( 3, [number=1, seconds=0], [
  function('(_), [0,1,2 ]),
  function(*(_,_), [
    0,1,0,
    1,0,1,
    0,1,2 ])]).
Read line-by-line from a file
*/

const std::string Model::Interpretation_label = "interpretation";
const std::string Model::Function_label = "function";
const std::string Model::Function_unary_label = "(_)";
const std::string Model::Function_binary_label = "(_,_)";
const std::string Model::Function_stopper = "])";
const std::string Model::Model_stopper = "]).";

std::string
Model::graph_to_string(sparsegraph* g) const
{
    return put_sg_str(g, false);
}


bool
Model::operator==(const Model& a) const
{
    if (!aresame_sg(this->cg, a.cg))
        return false;
    return true;
}

void
Model::print_model(std::ostream& os, bool out_cg) const
{
    os << model_str;
    if (out_cg)
        os << graph_to_string(cg);
}

void
Model::find_func_name(const std::string& func)
{
    int start = func.find("(");
    int end = func.find("(", start+1);
    op_symbols.push_back(func.substr(start+1, end-start-1));
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
debug_print(std::vector<std::vector<size_t>>& m)
{
    for (size_t row = 0; row < m.size(); row++) {
        for (size_t col = 0; col < m[row].size(); col++) {
            std::cout << m[row][col] << ",";
        } 
        std::cout << std::endl;
    }
}

int
IsoFilter::process_all_models()
{
    std::istream& fs = std::cin;
    bool  done = false;

    std::string line;
    while (!fs.eof()) {
        getline(fs, line);
        if (line[0] == '%')
            continue; 
        if (line.find("interpretation") != std::string::npos) {
            Model m;
            m.fill_meta_data(line);
            m.parse_model(fs);

            m.build_graph();
            //std::cout << "% debug cg: \n" << m.graph_to_string(cg) << std::endl;
            if (is_non_iso_hash(m))
                m.print_model(std::cout, out_cg);
        }
    }
    std::cout << "% Number of non-iso models: " << non_iso_hash.size() << std::endl;
    return 0;
}

bool
Model::parse_model(std::istream& fs)
{
    /*  The "interpretation" line is already parsed.
        Returns true if success, false otherwise.
     */
    bool done = false;
    while (!done) {
        std::string line;
        getline(fs, line);
        if (line[0] == '%')
            continue;
        if (line.find(Function_label) != std::string::npos) {
            model_str.append(line);
            model_str.append("\n");
            int arity = find_arity(line);
            find_func_name(line);
            switch (arity) {
            case 1:
                done = parse_unary(line);
                break;
            default:
                done = parse_bin(fs);
            }
        }
    }
    return true;
}

void
Model::parse_row(std::string& line, std::vector<size_t>& row)
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
Model::parse_unary(const std::string& line)
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
    std::vector<size_t> row;
    parse_row(row_str, row);
    un_ops.push_back(row); 

    return end_of_model;    
}

bool
Model::parse_bin(std::istream& fs)
{
    /* The "[" token is already seen before coming to this function 
       On return, the line containing "])" tokens will have been removed
       The binary operation will be pushed to m.bin_ops
       Lines starting with % are comments, and are ignored.
       Returns true if all functions are extracted, false otherwise */
    
    // TODO: add checking for correct # of rows etc
    bool done = false;
    bool end_model = false;
    std::vector<std::vector<size_t>>  bin_op;
    std::vector<size_t>  row;
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
        parse_row(line, row);
        bin_op.push_back(row);
        row.clear();
    }
    bin_ops.push_back(bin_op);
    return end_model;
}
 

bool
Model::find_graph_size(size_t& num_vertices, size_t& num_edges,
                       bool& has_S, bool& has_T)
{

    size_t num_ternary_ops = ternary_ops.size();
    size_t num_bin_ops = bin_ops.size();
    size_t num_unary_ops = un_ops.size();

    // vertices
    // vertices for domain elements
    num_vertices = 3 * order;  // vertices for E, R, and F

    has_S = false;    // has binary op
    has_T = false;    // has ternary op
    if (num_bin_ops > 0 || num_ternary_ops > 0) {
        num_vertices += order;       // vertices for S
        has_S = true;
    }
    // vertices for op tables
    num_vertices += num_unary_ops * order;
    num_vertices += num_bin_ops * order * order;

    // debug print
    // std::cout << "debug, find_graph_size:  Number of vertices " << num_vertices << std::endl;

    // edges
    // edges for domain elements
    num_edges = 2 * order;      // undirected edge E to R and to F
    if (num_bin_ops > 0)
        num_edges += order;     // undirected edge E to S

    // edges for op tables
    num_edges += 2 * order * num_unary_ops;
    num_edges += 3 * order * order * num_bin_ops;

    num_edges *= 2;   // directed edges
    // debug print
    // std::cout << "debug:  Number of edges " << num_edges << std::endl;

    return true;                                                                       
}

void
Model::color_graph(int* ptn, int* lab, int ptn_sz, bool has_S)
{
    // colors
    for (size_t idx=0; idx < ptn_sz; ++idx) {
        ptn[idx] = 1;
        lab[idx] = idx;
    }
    // E, R, F segments
    size_t color_end = 0;
    for (size_t idx = 0; idx < 3; ++idx) {
        color_end += order;
        ptn[color_end - 1] = 0;
    }
    // S, if exists
    if (has_S) {
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
    // debug print
    // std::cout << "color array size: " << color_end << " space allocated: " << ptn_sz << std::endl;
    // for (size_t idx=0; idx < ptn_sz; ++idx)
    //     std::cout << ptn[idx] << " ";
    // std::cout << std::endl;
}

void
Model::count_occurrences(std::vector<size_t>& R_v_count)
{
    // count number of times a domain element appears in the op tables.
    for (auto op : un_ops) {
        for (auto v : op) {
            R_v_count[v]++;
        }
    }
    for (auto op : bin_ops) {
        for (auto row : op) {
            for (auto v : row) {
                R_v_count[v]++;
            }
        }
    }
    for (auto op : ternary_ops) {  // not supported yet: 2023/07/26
        for (auto first_arg : op) {
            for (auto secord_arg : first_arg) {
                for (auto v : secord_arg)  {
                    R_v_count[v]++;
                }
            }
        }
    }
}

void
Model::build_vertices(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v, const int A_c, bool has_S)
{
    std::vector<size_t> R_v_count(order, 0);
    count_occurrences(R_v_count);

    // set up vertices for E, F, S, T and R
    // each node in E points to F, S, T and R and to each of the cells in a row
    // S and T may not exist (ie. value zero)
    const size_t num_F = un_ops.size();
    const size_t num_S = bin_ops.size();
    const size_t num_T = ternary_ops.size();     // 7/29/2023, not supported yet

    const size_t E_outd = 2 + (has_S? 1 : 0);    //num of out edges per vertex
    const size_t E_size = E_outd * order;
    const size_t F_outd = num_F + num_S * order + 1;  // out-degree of first arg
    const size_t F_size = F_outd * order;
    const size_t S_outd = num_S * order + 1;          // out-degree of second arg
    const size_t S_size = has_S? S_outd * order : 0;
    size_t R_pos = E_size + F_size + S_size;

    // debug print
    // std::cout << "debug: start R_pos: " << R_pos << "  has_S: " << has_S << std::endl;
    for (size_t idx = 0; idx < order; ++idx) {
        sg1.v[E_e+idx] = E_outd * idx;
        sg1.d[E_e+idx] = E_outd;                         // out-degree
        sg1.v[F_a+idx] = E_size + F_outd * idx;
        sg1.d[F_a+idx] = F_outd;                         // out-degree
        if (has_S) {
            sg1.v[S_a+idx] = E_size + F_size + S_outd * idx;
            sg1.d[S_a+idx] = S_outd;                     // out-degree
        }

        sg1.v[R_v+idx] = R_pos;
        sg1.d[R_v+idx] = R_v_count[idx] + 1;
        R_pos += sg1.d[R_v+idx];
    }
    // debug print
    // std::cout << "debug: end R_pos: " << R_pos << "  A_c " << A_c << std::endl;

    // set up vertices of op tables
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
    // ternary op tables - not supported, 7/26/2023
}

void
Model::build_edges(sparsegraph& sg1, const int E_e, const int F_a, const int S_a, const int R_v, const int A_c, bool has_S)
{
    // E to R, F, S, and T 
    for (size_t idx = 0; idx < order; ++idx) {
        sg1.e[sg1.v[E_e+idx]] = F_a+idx;
        sg1.e[sg1.v[F_a+idx]] = E_e+idx;
        sg1.e[sg1.v[E_e+idx]+1] = R_v+idx;
        sg1.e[sg1.v[R_v+idx]] = E_e+idx;
        if (has_S) {
            sg1.e[sg1.v[E_e+idx]+2] = S_a+idx;
            sg1.e[sg1.v[S_a+idx]] = E_e+idx;
        }
    }

    // A_abc, a*b=c ->  edges from A_abc to F_a, S_b, and R_c
    size_t A_c_el = A_c;
    std::vector<size_t> R_v_pos(order, 1);   // First position of R_v points to E_e
    std::vector<size_t> F_a_pos(order, 1);   // First position of F_a points to E_e
    std::vector<size_t> S_a_pos(order, 1);   // First position of S_a points to E_e

    for (size_t op=0; op < un_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            size_t cval = un_ops[op][f_arg];

            sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
            sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

            sg1.e[sg1.v[A_c_el]+1] = R_v + cval;
            sg1.e[sg1.v[R_v+cval]+R_v_pos[cval]] = A_c_el; 
            
            F_a_pos[f_arg]++;
            R_v_pos[cval]++;
            A_c_el++;
        }
    }
 
    for (size_t op=0; op < bin_ops.size(); ++op) {
        for (size_t f_arg=0; f_arg < order; ++f_arg) {
            for (size_t s_arg=0; s_arg < order; ++s_arg) {
                size_t cval = bin_ops[op][f_arg][s_arg];

                sg1.e[sg1.v[A_c_el]] = F_a + f_arg;
                sg1.e[sg1.v[F_a+f_arg]+F_a_pos[f_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+2] = S_a + s_arg;
                sg1.e[sg1.v[S_a+s_arg]+S_a_pos[s_arg]] = A_c_el; 

                sg1.e[sg1.v[A_c_el]+1] = R_v + cval;
                sg1.e[sg1.v[R_v+cval]+R_v_pos[cval]] = A_c_el; 
            
                F_a_pos[f_arg]++;
                S_a_pos[s_arg]++;
                R_v_pos[cval]++;
                A_c_el++;
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
        A represents the operation table
     */
    size_t num_vertices, num_edges;
    bool   has_S, has_T;
    find_graph_size(num_vertices, num_edges, has_S, has_T);
    // debug print
    // std::cout << "num_vertices: " << num_vertices << " num_edges: " << num_edges << std::endl;

    DYNALLSTAT(int,lab,lab_sz);
    DYNALLSTAT(int,ptn,ptn_sz);
    DYNALLSTAT(int,orbits,orbits_sz);
    DYNALLSTAT(int,map,map_sz);
    static DEFAULTOPTIONS_SPARSEGRAPH(options);
    statsblk stats;

    /* Declare and initialize sparse graph structures */
    SG_DECL(sg1);
    SG_DECL(cg1);

   /* Select option for canonical labelling */

    options.getcanon = TRUE;

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
    const int  E_e = 0;
    const int  R_v = E_e + order;
    const int  F_a = R_v + order;
    const int  S_a = has_S? F_a + order : 0;
    const int  A_c = std::max(F_a, S_a) + order;

    // debug print
    // std::cout << "E R F S A: " << E_e << " " << R_v << " " << F_a << " " << S_a << " " << A_c << std::endl;

    // vertices
    build_vertices(sg1, E_e, F_a, S_a, R_v, A_c, has_S);

    // color the graph
    color_graph(ptn, lab, ptn_sz, has_S);

    // edges
    build_edges(sg1, E_e, F_a, S_a, R_v, A_c, has_S);

    //debug print
    //std::cout << graph_to_string(&sg1) << std::endl;

    // compute canonical form
    sparsenauty(&sg1,lab,ptn,orbits,&options,&stats,&cg1);

    // debug print
    // std::cout << "done sparsenauty " << std::endl;
    sortlists_sg(&cg1);
    // debug print
    // std::cout << graph_to_string(&sg1) << std::endl;

    cg = copy_sg(&cg1, NULL);
    SG_FREE(sg1);
    return true;
}

bool
IsoFilter::is_non_iso(const Model& model)
{
    bool non_iso = true;
    for (auto m : non_iso_vec) {
        if (model == m) {
            non_iso = false;
            break;
        }
    }
    if (non_iso) {
        non_iso_vec.push_back(model);
    }
    return non_iso;
}


bool
IsoFilter::is_non_iso_hash(const Model& model)
{
    std::string canon_str = model.graph_to_string(model.cg);

    if (non_iso_hash.find(canon_str) == non_iso_hash.end()) {
        //std::cout << "% found non-iso " << idx << std::endl;
        non_iso_hash.insert(canon_str);
        return true;
    }
    return false;
}

