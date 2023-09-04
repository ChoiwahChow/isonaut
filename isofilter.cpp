
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
    const bool use_std = opt.file_name == "-";
    std::istream* fp = &std::cin;
    std::ifstream filep;
    std::string   check_sym = opt.check_sym;

    if (!use_std) {
        filep.open(opt.file_name.c_str());
        // debug print: std::cout << opt.file_name << std::endl;
        fp = &filep;
    }
    if (!check_sym.empty()) {
        check_sym.insert(0, ",");
        check_sym.append(",");
    }
    std::istream& fs = *fp;
    bool   done = false;
    size_t models_count = 0;

    double start_cpu_time = read_cpu_time();
    unsigned start_wall_clock = read_wall_clock();
    std::string line;
    while (!fs.eof()) {
        getline(fs, line);
        if (line[0] == '%')
            continue; 
        if (line.find("interpretation") != std::string::npos) {
            models_count++;
            Model m;
            m.fill_meta_data(line);
            m.parse_model(fs, check_sym);

            if (!m.build_graph())  // is it empty graph?
                continue; 

            std::string canon_str;
            if (is_non_iso_hash(m, canon_str))
                m.print_model(std::cout, canon_str, opt.out_cg);
        }
    }
    if (!use_std)
        filep.close();
    double total_cpu_time = read_cpu_time() - start_cpu_time;
    unsigned elapsed_time = read_wall_clock() - start_wall_clock;
    std::cout << "% Number of models processed: " << models_count << std::endl;
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
    if (non_iso && 
        (opt.max_cache < 0 || non_iso_hash.size() < opt.max_cache)) {
        non_iso_vec.push_back(model);
    }
    return non_iso;
}


bool
IsoFilter::is_non_iso_hash(const Model& model, std::string& canon_str)
{
    canon_str = model.graph_to_string(model.cg);

    if (opt.compress) {
        canon_str = compress(canon_str);
    }

    if (non_iso_hash.find(canon_str) == non_iso_hash.end()) {
        // std::cout << "% found non-iso max_cache: " << opt.max_cache << std::endl;   // debug print
        if (opt.max_cache < 0 || non_iso_hash.size() < opt.max_cache)
            non_iso_hash.insert(canon_str);
        // std::cout << "% found non-iso cache size: " << non_iso_hash.size() << std::endl;   // debug print
        return true;
    }
    return false;
}


bool
IsoFilter::is_non_isomorphic(Model& m)
{
    std::string canon_str;
    bool is_non_iso = is_non_iso_hash(m, canon_str);
    return is_non_iso;
}


std::string 
IsoFilter::compress(const std::string& str, int compressionlevel) 
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK)
        throw(std::runtime_error("deflateInit failed while compressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[10240];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}
