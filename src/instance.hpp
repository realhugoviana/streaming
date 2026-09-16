#include <iostream>


/*
    STRUCTURES
*/

/// @brief The parameter of the instance
struct InstanceParameters {
    int V; // Number of video
    int E; // Number of endpoints
    int R; // Number of requests
    int C; // Number of cache server
    int X; // Allocated size to cache
};

/// @brief Definition of a link between an endpoint and the caches
struct EndpointCacheConnection {
    int idC; // Identifier of the cache
    int cache_latency; // Associated Latency
};

/// @brief Definition of an endpoint by its id, its latency and its connections
struct Endpoint {
    int idE; // identifier of the endpoint
    int dc_latency; // Latency of the datacenter
    int K; // Number of connections
    EndpointCacheConnection* endpoint_connections; // "Array" of connection to cache
};

/// @brief Definition of the video by its size and id
struct Video {
    int idV; // Identifier of the video
    int vsize; // Size of the video in Mo
};

/// @brief Definition of a Request by the id between the video and then endpoint as well as the requested quantity 
struct Request {
    int idV; // Identifier of the requested video
    int idE; // Identifier of the endpoint
    int count; // Count the number of requests
};

/// @brief Structure holding all the information about the instance
struct InstanceData {
    InstanceParameters ip; // Informations on global parameters
    Video* video_sizes; // Vector containing all the videos
    Endpoint* endpoints; // Vector containing all the endpoints
    Request* requests; // Vector containing all the requests
    bool** cache_affectation; // Matrix of boolean to describe the affectation of video to cache. Shape: VxC (row: video, col: cache)
};

/*
    PARSER RELATED FUNCTIONS
*/

// Initialisation of array, matrices and instances
template <typename T> T* initialiseArray(int quantity);
bool** initialiseCacheAffectation(int V, int C);
InstanceData initialiseInstance(InstanceParameters ip);

// Parser 
InstanceData videoParser(InstanceData instance);
InstanceData endpointParser(InstanceData instance);
InstanceData requestParser(InstanceData instance);
InstanceData parser();

// Instance viewer
void showInstance(InstanceData* instance);











