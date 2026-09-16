#include "src/instance.hpp"

/*
    PARSER RELATED FUNCTIONS
*/

/// @brief Template (function) that given a type and a size create an array of said size of said type
/// @tparam T The type of the array
/// @param quantity The size of the array
/// @return An array of size `quantity` and of type `T`
template <typename T> T* initialiseArray(int quantity) {
    T* arr = (T*)malloc(quantity*sizeof(T));
    return arr;
}

/// @brief Function that, given number of video and cache, create the boolean association matrix between those 
/// @param V The number of videos
/// @param C The number of caches
/// @return A boolean matrix, initialised to false, of size VxC (Videos are in rows, Caches in columns)
bool** initialiseCacheAffectation(int V, int C) {
    // Allocate for the rows
    bool** cache_affectation = (bool**)malloc(V*sizeof(bool*));

    // Generates the rows, and allocates columns within each rows
    for (int v = 0; v < V; v++) {
        cache_affectation[v] = (bool*)malloc(C*sizeof(bool));
        for (int k = 0; k < C; k++) {
            // Set the cache to false
            cache_affectation[v][k] = false;
        }
    }

    // Return the caches affectations
    return cache_affectation;
}

/// @brief Function that 
/// @param C 
/// @return 
Cache* initialiseCacheArray(int C) {
    // Initialise an array of Cache
    Cache* caches = initialiseArray<Cache>(C);

    // For each cache, set the id and used memory to 0
    for (int k = 0; k<C; k++) {
        caches[k].idC = k;
        caches[k].used_memory = 0;
    }

    // Return the array of caches
    return caches;
}

/// @brief Function to initialise the instance structure.
/// @param ip The structure containing the instance parameters.
/// @return The initialised instance
InstanceData initialiseInstance(InstanceParameters ip) {
    // Declare the instance
    InstanceData instance;

    // Fill the instance
    instance.ip = ip;
    instance.video_sizes = initialiseArray<Video>(ip.V);
    instance.requests = initialiseArray<Request>(ip.R);
    instance.endpoints = initialiseArray<Endpoint>(ip.E);
    instance.cache_affectation = initialiseCacheAffectation(ip.V, ip.C);
    instance.caches = initialiseCacheArray(ip.C);

    // Return the empty instance
    return instance;
}

/// @brief Subfunction of `parser` used to parse the videos
/// @param instance The empty instance
/// @return Instance with  filled array of videos
InstanceData videoParser(InstanceData instance) {
    // For each video
    for (int v = 0; v<instance.ip.V; v++) {
        // Get the size and store the video with the corresponding id
        std::cin >> instance.video_sizes[v].vsize;
        instance.video_sizes[v].idV = v;
    }

    // Return the modified instance
    return instance;
}

/// @brief Subfunction of `parser` used to parse the endpoints
/// @param instance The empty instance
/// @return Instance with filled array of endpoints
InstanceData endpointParser(InstanceData instance) {
    // Declare number of connected caches
    int K;

    // For each endpoint
    for (int e = 0; e<instance.ip.E; e++) {
        // Get the latency, the number of connected caches and register them along with the id of the endpoint
        std::cin >> instance.endpoints[e].dc_latency >> K;
        instance.endpoints[e].K = K;
        instance.endpoints[e].idE = e;
        
        // Initialise the array of connections
        EndpointCacheConnection* endpoint_connections = initialiseArray<EndpointCacheConnection>(K);
        
        // Retrieve the informations about the connections 
        for (int k = 0;  k<K; k++) {
            std::cin >> endpoint_connections[k].idC >> endpoint_connections[k].cache_latency;
        }
        
        // Attach the endpoints connections
        instance.endpoints[e].endpoint_connections = endpoint_connections;
    }

    // Return the modified instance
    return instance;
}

/// @brief Subfunction of `parser` used to parse the requests
/// @param instance The empty instance
/// @return Instance with filled array of requests
InstanceData requestParser(InstanceData instance) {
    // For each request, get the information and store it into the instance
    for (int r = 0; r<instance.ip.R; r++) {
        std::cin >> instance.requests[r].idV >> instance.requests[r].idE >> instance.requests[r].count;
    }

    // Return the modified instance
    return instance;
}

/// @brief Function for the raw parser
/// @return The instance of the problem
InstanceData parser() {
    // Retrieve Instance Parameter
    InstanceParameters ip;
    std::cin >> ip.V >> ip.E >> ip.R >> ip.C >> ip.X;

    // Initialise the instance
    InstanceData instance = initialiseInstance(ip);

    // Fill the instance
    instance = videoParser(instance);
    instance = endpointParser(instance);
    instance = requestParser(instance);
    
    // Return the instance
    return instance;
}
     
/// @brief Method to show the parameters of the instance
/// @param instance 
void showInstance(InstanceData* instance) {
    // Global information
    std::printf("\n---[Global Data]---\nNumber of Video (V): %d\nNumber of Endpoint (E): %d\nNumber of Request (R): %d\nNumber of Cache Server (C): %d\nSize of Cache Server (X): %dMo\n\n", 
        instance->ip.V,
        instance->ip.E,
        instance->ip.R,
        instance->ip.C,
        instance->ip.X
    );

    // Information on the Videos
    std::cout << "---[Videos Data]---" << std::endl;
    for (int v = 0; v<instance->ip.V; v++) {
        printf("Video number [%d] of size [%dMo]\n", 
            instance->video_sizes[v].idV,
            instance->video_sizes[v].vsize
        );
    }
    std::cout << std::endl;

    // Information on the Endpoints
    for (int e = 0; e<instance->ip.E; e++) {
        Endpoint endpoint = instance->endpoints[e];
        printf("Endpoint number [%d] with datacenter latency [%dms]\n", 
            endpoint.idE,
            endpoint.dc_latency
        );

        /// Information about their connections
        for (int k = 0; k<endpoint.K; k<k++) {
            printf("|. Connected to Cache [%d] with latency [%dms]\n", 
                endpoint.endpoint_connections[k].idC, 
                endpoint.endpoint_connections[k].cache_latency
            );
        }
        std::cout << std::endl;
    }

    // Information on the requests
    std::cout << "\n---[Requests Data]---" << std::endl;
    for (int r = 0; r<instance->ip.R; r++) {
        printf("The video [%d] is requested from endpoint [%d] [%d] times\n", 
            instance->requests[r].idV,
            instance->requests[r].idE,
            instance->requests[r].count
        );
    }

    // Information on the caches
    std::cout << "\n---[Caches Data]---" << std::endl;
    for (int k = 0; k<instance->ip.C; k++) {
        printf("Cache number [%d]\n", k);
        for (int v = 0; v<instance->ip.V; v++) {
            if (instance->cache_affectation[v][k]) {
                printf("|. Possess video [%d] of size [%dMo]\n", v, instance->video_sizes[v].vsize);
            }
        }
    }
}

/*
    MAIN
*/

/// @brief Parser of the HashCode Challenge in C++ : reads an instance and dumps it
/// @return GlobalData gd, std::vector<Video> video_sizes, std::vector<Endpoint> endpoints and std::vector<Request> requests
int main(int argc, char* argv[]) {
    //if (argc < 2) {
    //    std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in>" << std::endl;
    //    return 1;
    //}

    // By design, unconnect cache are not in the model
    InstanceData instance = parser();
    showInstance(&instance);
    return 0;
}
