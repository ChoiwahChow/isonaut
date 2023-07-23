
#include <sstream>
#include <iostream>
#include "isofilter.h"
#include "nauty_utils.h"

/*
interpretation( 4, [number=1, seconds=0], [
  function(*(_,_), [
    0,1,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0 ])]).
Read line-by-line from a file
*/

const std::string Model::Interpretation_label = "interpretation";
const std::string Model::Function_label = "function";
const std::string Model::Function_stopper = "])";
const std::string Model::Model_stopper = "]).";

std::string
Model::canon_graph_to_string() const
{
    return put_sg_str(canon[0], false);
}

std::string
Model::stringify() const
{
    std::string mstr;

    return mstr;
}

bool
Model::operator==(const Model& a) const
{
    for (size_t idx=0; idx < this->canon.size(); ++idx)
        if (!aresame_sg(this->canon[idx], a.canon[idx]))
            return false;
    return true;
}

void
Model::print_model(std::ostream& os) const
{
    os << model_str;
}

void
IsoFilter::find_name(const std::string& func, std::string& name)
{
    int start = func.find("(");
    int end = func.find("(", start+1);
    name = func.substr(start+1, end-start-1);
}

void
IsoFilter::fill_meta_data(const std::string& interp, Model& m)
{
    int start = interp.find("( ");
    int end = interp.find(", ", start+1);
    std::stringstream ss(interp.substr(start+1, end-start-1));
    ss >> m.order;
}

int
IsoFilter::find_arity(const std::string& func)
{
    size_t arity = 2;
    if (func.find("(_,_)") != std::string::npos)
        arity = 2;
    if (func.find("(_)") != std::string::npos)
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
            fill_meta_data(line, m);
            m.model_str.append(line);
            parse_model(fs, m, m.model_str);
            // std::cout << m.model_str;
            // debug_print(m.bin_ops[0]);
            build_binop_graph(m, 0);
            // std::cout << "% " << m.stringify_canon_graph() << std::endl;
            if (is_non_iso_hash(m))
                m.print_model(std::cout);
        }
    }
    std::cout << "% Number of models (using hashing): " << non_iso_hash.size() << std::endl;
    return 0;
}

bool
IsoFilter::parse_model(std::istream& fs, Model& m, std::string& m_str)
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
        if (line.find(Model::Function_label) != std::string::npos) {
            m_str.append("\n");
            m_str.append(line);
            int arity = find_arity(line);
            switch (arity) {
            default:
                done = parse_bin(fs, m, m_str);
            }
        }
    }
    return true;
}

bool
IsoFilter::parse_bin(std::istream& fs, Model& m, std::string& m_str)
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
        m_str.append("\n");
        m_str.append(line);
        if (line.find(Model::Function_stopper) != std::string::npos) {
            done = true;
            if (line.find(Model::Model_stopper) != std::string::npos)
                end_model = true;
        }
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
        bin_op.push_back(row);
        row.clear();
    }
    m.bin_ops.push_back(bin_op);
    m_str.append("\n");
    return end_model;
}
 

bool
IsoFilter::build_binop_graph(Model& m, size_t op)
{
    /* There are 4 classes of vertices, with different colours.
       Say the ground set is X and operation is *.  
       Vertices are as follows, n^2 + 3n altogether:

       A_a  :  a in X
       B_b  :   b in X
       C_c  :  c in X
       E_ab :  a,b in X

       For each triple a*b=c, add edges from E_ab to A_a, B_b, and C_c.
       Also, for each x in X, add a triangle of edges A_x,B_x,C_x.
    */
    /*  A vertex of A, B, or C has 1 out edge, and a vertex in E_ab has 3.
        For an order k binary operation, there are k^2 + 3k vertices
        and 3k^2 + 3k undirected edges.  Thus, there are 2(3k^2 + 3k) directed edges.

        In the following: A is row, B is column, C is cell values, E_ab
        are cells.
        Edges: C->A, C->B, and B->A.  (Triangle of edges, but not cyclic).
               ab->a, ab->b, and ab->c.
     */

    int num_vertices = (m.order + 3) * m.order;            //order k:  k^2 + 3k
    int num_edges = 2 * 3 * m.order * (m.order + 1);       //directed edges: 2 * (3k^2 + 3k)

    //debug print
    //std::cout << "num vertices: " << num_vertices << " num directed edges: " << num_edges << std::endl;

    DYNALLSTAT(int,lab1,lab1_sz);
    DYNALLSTAT(int,ptn,ptn_sz);
    DYNALLSTAT(int,orbits,orbits_sz);
    DYNALLSTAT(int,map,map_sz);
    static DEFAULTOPTIONS_SPARSEGRAPH(options);
    statsblk stats;
    // static DEFAULTOPTIONS_TRACES(options);
    // TracesStats stats;

    /* Declare and initialize sparse graph structures */
    SG_DECL(sg1); 
    SG_DECL(cg1);

   /* Select option for canonical labelling */

    options.getcanon = TRUE;

    int mx = SETWORDSNEEDED(num_vertices);
    nauty_check(WORDSIZE,mx,num_vertices,NAUTYVERSIONID);
    //debug print
    //std::cout << "WORDSIZE " << WORDSIZE << " return value for SETWORDSNEEDED(num_vertices) " << mx << std::endl;

    DYNALLOC1(int,lab1,lab1_sz,num_vertices,"malloc");
    DYNALLOC1(int,ptn,ptn_sz,num_vertices,"malloc");
    DYNALLOC1(int,orbits,orbits_sz,num_vertices,"malloc");
    DYNALLOC1(int,map,map_sz,num_vertices,"malloc");

    //debug print
    //std::cout << "lab1_sz " << lab1_sz  << " ptn_sz " << ptn_sz << " orbits_sz " << orbits_sz << " map_sz " << map_sz << std::endl;

    // make the graph
    SG_ALLOC(sg1,num_vertices,num_edges,"malloc");
    sg1.nv = num_vertices;     // Number of vertices 
    sg1.nde = num_edges;       // Number of directed edges = twice of # undirected edges here

    // vertices, offsets by domain element number to make them unique
    // E.g. A_a+2 represents row 2, B_b+2 represents the same 2, but column 2,
    // C_c+2 represents the same 2 in the op table cell
    // E_ab+2 represents the cell (0, 2) (the cell position, not the value in the cell).
    const int  A_a = 0;
    const int  B_b = m.order;
    const int  C_c = 2 * m.order;
    const int  E_ab = 3 * m.order;

    size_t C_c_count[m.order];
    std::fill_n(C_c_count, m.order, 0);
    for (size_t idx = 0; idx < m.order; ++idx)
        for (size_t jdx = 0; jdx < m.order; ++jdx) {
            C_c_count[m.bin_ops[op][idx][jdx]]++;
        }

    // vertices for A, B, and C
    // each node in A points to B and C and to each of the cells in a row
    size_t A_size = 2*m.order + m.order * m.order;
    size_t C_pos = 2 * A_size;
    for (size_t idx = 0; idx < m.order; ++idx) {
        sg1.v[A_a+idx] = (m.order + 2) * idx;
        sg1.d[A_a+idx] = m.order + 2;            // out-degree
        sg1.v[B_b+idx] = A_size + (m.order + 2) * idx;
        sg1.d[B_b+idx] = m.order + 2;
        sg1.v[C_c+idx] = C_pos;
        sg1.d[C_c+idx] = C_c_count[idx] + 2;
        C_pos += sg1.d[C_c+idx];
    }

    // vertices for E_ab
    size_t E_ab_vstart = 3*m.order*m.order + 6*m.order;
    for (size_t idx=0; idx < m.order; ++idx) {
        const size_t row_pos = E_ab + idx * m.order;
        for (size_t jdx=0; jdx < m.order; ++jdx) {
            sg1.v[row_pos+jdx] = E_ab_vstart + idx * m.order * 3 + 3 * jdx;
            sg1.d[row_pos+jdx] = 3;       // out-degree of vertex 
        }
    }
    // colors
    for (size_t idx=0; idx < ptn_sz; ++idx) {
        ptn[idx] = 1;
        lab1[idx] = idx;
    }
    ptn[A_a+m.order-1] = 0;    // A_a
    ptn[B_b+m.order-1] = 0;    // B_b
    ptn[C_c+m.order-1] = 0;    // C_c
    ptn[num_vertices-1] = 0;   // E_ab

    // edges
    // triangle of edges A_x <-> B_x, B_x <-> C_x, C_x <-> A_x.
    for (size_t idx = 0; idx < m.order; ++idx) {
        sg1.e[sg1.v[A_a+idx]] = B_b+idx;
        sg1.e[sg1.v[A_a+idx]+1] = C_c+idx;
        sg1.e[sg1.v[B_b+idx]] = A_a+idx;
        sg1.e[sg1.v[B_b+idx]+1] = C_c+idx;
        sg1.e[sg1.v[C_c+idx]] = A_a+idx;
        sg1.e[sg1.v[C_c+idx]+1] = B_b+idx;
    }

    // E_ab, a*b=c ->  edges from E_ab to A_a, B_b, and C_c
    size_t C_c_pos[m.order];
    std::fill_n(C_c_pos, m.order, 2);
    for (size_t idx = 0; idx < m.order; ++idx) {
        const size_t row_pos = E_ab + idx * m.order;
        for (size_t jdx = 0; jdx < m.order; ++jdx) {
            size_t c = m.bin_ops[op][idx][jdx];

            sg1.e[sg1.v[row_pos+jdx]] = A_a + idx; 
            sg1.e[sg1.v[A_a+idx]+2+jdx] = row_pos+jdx; 

            sg1.e[sg1.v[row_pos+jdx]+1] = B_b + jdx; 
            sg1.e[sg1.v[B_b+jdx]+2+idx] = row_pos+jdx; 

            sg1.e[sg1.v[row_pos+jdx]+2] = C_c + c; 
            sg1.e[sg1.v[C_c+c]+C_c_pos[c]] = row_pos+jdx;
            C_c_pos[c]++;
        }
    }
    /* debug print of edges 
    std::cout << "Edge array:" << std::endl;
    for (size_t idx = 0; idx < num_edges; ++idx) {
        std::cout << idx << ": " << sg1.e[idx] << std::endl;
    }
    */

    /* debug print of lab1 and ptn - color scheme
    std::cout << "Color arrays" << std::endl;
    for (size_t idx = 0; idx < lab1_sz; ++idx) 
        std::cout << lab1[idx] << " ";
    std::cout << std::endl << lab1_sz << std::endl;

    for (size_t idx = 0; idx < ptn_sz; ++idx) 
        std::cout << ptn[idx] << " ";
    std::cout << std::endl << ptn_sz << std::endl;
    */
 
    /* Label sg1, result in cg1 and labelling in lab1.
       It is not necessary to pre-allocate space in cg1, but
       they have to be initialised as we did above.  */

    /*debug print
    { FILE *fp = fopen("sg1.txt", "a");
    put_sg(fp, &sg1, true, 120);
    fputc('\n', fp);
    fclose(fp); }
    std::cout << "Printed graph in sg1.txt" << std::endl;
    */
            
    sparsenauty(&sg1,lab1,ptn,orbits,&options,&stats,&cg1);
    sortlists_sg(&cg1);

    /* debug print
    std::cout << std::endl << "after" << std::endl;
    FILE *fp = fopen("cg1.txt", "a");
    put_sg(fp, &cg1, true, 120);
    fputc('\n', fp);
    fclose(fp);
    */

    m.canon.push_back(copy_sg(&cg1, NULL));
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
    std::string canon_str = model.canon_graph_to_string();

    if (non_iso_hash.find(canon_str) == non_iso_hash.end()) {
        //std::cout << "% found non-iso " << idx << std::endl;
        non_iso_hash.insert(canon_str);
        return true;
    }
    return false;
}

