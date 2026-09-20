#include <iostream>
#include <vector>


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
    std::vector<int> associated_requests; // Id of the associated requests
};

/// @brief Definition of a Request by the id between the video and then endpoint as well as the requested quantity 
struct Request {
    int idR; // Identifier of the request 
    int idV; // Identifier of the requested video
    int idE; // Identifier of the endpoint
    int count; // Count the number of requests
    int gain; // Latency gain
};

/// @brief Definition of a Cache by its id and its used memory
struct Cache {
    int idC; // Id of the cache
    int left_memory; // Amount of memory left to store additional videos  
};

/// @brief Structure holding all the information about the instance
struct InstanceData {
    InstanceParameters ip; // Informations on global parameters
    Video* videos; // Array containing all the videos
    Endpoint* endpoints; // Array containing all the endpoints
    Request* requests; // Array containing all the requests
    Cache* caches; // Array containing all the caches
    bool** cache_affectation; // Matrix of boolean to describe the affectation of video to cache. Shape: VxC (row: video, col: cache)
    int sum_request_count; // Sum of all count in subsequent requests
};

/*
    PARSER RELATED FUNCTIONS
*/

// Initialisation of array, matrices and instances
template <typename T> T* initialiseArray(int quantity);
bool** initialiseCacheAffectation(int V, int C);
Cache* initialiseCacheArray(int C, int X);
Request* initialiseRequestArray(int R);
InstanceData initialiseInstance(InstanceParameters ip);

// Parser 
void videoParser(InstanceData& instance);
void endpointParser(InstanceData& instance);
void requestParser(InstanceData& instance);
InstanceData parser();

// Instance viewer and out
void showInstance(InstanceData* instance);
void showVideoCacheAssociations(InstanceData* instance);
void instanceOut(InstanceData* instance);
int computeTotalGain(InstanceData* instance);











