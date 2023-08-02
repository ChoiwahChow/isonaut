
#include <sstream>
#include <iostream>
#include "nauty_utils.h"
#include "isofilter.h"

/*
interpretation( 3, [number=1, seconds=0], [
  function('(_), [0,1,2 ]),
  function(*(_,_), [
    0,1,0,
    1,0,1,
    0,1,2 ])]).
Read line-by-line from a file
*/


int
IsoFilter::process_all_models()
{
    std::istream& fs = std::cin;
    bool  done = false;

    double start_cpu_time = read_cpu_time();
    unsigned start_wall_clock = read_wall_clock();
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

            if (is_non_iso_hash(m))
                m.print_model(std::cout, opt.out_cg);
        }
    }
    double total_cpu_time = read_cpu_time() - start_cpu_time;
    unsigned elapsed_time = read_wall_clock() - start_wall_clock;
    std::cout << "% Number of non-iso models: " << non_iso_hash.size() << std::endl;
    std::cout << "% Total CPU time: " << total_cpu_time << " seconds." << std::endl;
    std::cout << "% Elapsed time: " << elapsed_time << " seconds." << std::endl;
    return 0;
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

