#include "src/instance.hpp"

/*
    MAIN
*/
/// @brief Parser of the HashCode Challenge in C++ : reads an instance and dumps it
/// @return GlobalData gd, std::vector<Video> video_sizes, std::vector<Endpoint> endpoints and std::vector<Request> requests
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in>" << std::endl;
        return 1;
    }

    // By design, unconnect cache are not in the model
    InstanceData instance = parser();
    showInstance(&instance);
    return 0;
}
