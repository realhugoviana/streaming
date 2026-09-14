#include <fstream>
#include <sstream>
#include <string>

#include "../instance.hpp"
#include "../solution.hpp"

/*
    SCORER

    Reads an instance and a solution file, checks the solution and prints its
    score. Useful to double check anything a solver wrote.
*/
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> <path_to_solution.txt>" << std::endl;
        return 1;
    }

    InstanceData instance = parseFile(argv[1]);

    std::ifstream in(argv[2]);
    if (!in.is_open()) {
        std::cerr << "Error: could not open file '" << argv[2] << "'" << std::endl;
        return 1;
    }

    // Read the number of described caches, then one line per cache
    int N;
    in >> N;
    Solution solution = emptySolution(instance);
    for (int i = 0; i < N; i++) {
        int id_cache;
        in >> id_cache;
        if (id_cache < 0 || id_cache >= instance.gd.C) {
            std::cerr << "Error: unknown cache " << id_cache << std::endl;
            return 1;
        }

        // The rest of the line is the list of stored videos
        std::string line;
        std::getline(in, line);
        std::istringstream videos(line);
        int id_video;
        while (videos >> id_video) solution[id_cache].push_back(id_video);
    }

    return report("scorer", instance, solution, "") < 0 ? 1 : 0;
}
