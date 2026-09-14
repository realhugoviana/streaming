#include <cstring>
#include <string>

#include "../greedy.hpp"

/*
    MAIN
*/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> [-o out.txt] [-m static|lazy] [-k gain|density]" << std::endl;
        return 1;
    }

    std::string input_path = argv[1], output_path, mode = "lazy", key = "density";
    for (int i = 2; i + 1 < argc; i += 2) {
        if (std::strcmp(argv[i], "-o") == 0) output_path = argv[i + 1];
        else if (std::strcmp(argv[i], "-m") == 0) mode = argv[i + 1];
        else if (std::strcmp(argv[i], "-k") == 0) key = argv[i + 1];
        else { std::cerr << "Unknown option '" << argv[i] << "'" << std::endl; return 1; }
    }
    bool density = (key == "density");

    // The solver uses the reduced instance. The score always uses the raw one.
    InstanceData raw = parseFile(input_path);
    InstanceData in = reduce(raw);

    Index x = buildIndex(in);
    State st = makeState(x);
    std::vector<Cand> cand = candidates(x, in, st, density);
    size_t pairs = cand.size();

    long long pops = 0;
    if (mode == "static") greedyStatic(x, st, std::move(cand));
    else pops = greedyLazy(x, st, std::move(cand), density);

    std::string name = "greedy-" + mode + "-" + key;
    std::cout << "[" << name << "] pairs: " << pairs << ", pops: " << pops << std::endl;
    return report(name, raw, st.sol, output_path) < 0 ? 1 : 0;
}
