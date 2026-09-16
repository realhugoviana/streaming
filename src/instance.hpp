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
    HELPER FUNCTIONS
*/

/// @brief Function for the raw parser
/// @return The instance of the problem
InstanceData parser() {
    // Retrieve Instance Parameter
    InstanceParameters ip;
    std::cin >> ip.V >> ip.E >> ip.R >> ip.C >> ip.X;

    // Initialise all the arrays
    Video* video_sizes = (Video*)malloc(ip.V*sizeof(Video));
    Request* requests = (Request*)malloc(ip.R*sizeof(Request));
    Endpoint* endpoints = (Endpoint*)malloc(ip.E*sizeof(Endpoint));

    // Initialise the cache affectation matrix and initialise it to 0
    bool** cache_affectation = (bool**)malloc(ip.V*sizeof(bool*));
    for (int v = 0; v < ip.V; v++) {
        cache_affectation[v] = (bool*)malloc(ip.C*sizeof(bool));
        for (int k = 0; k < ip.C; k++) {
            cache_affectation[v][k] = false;
        }
    }

    // Retrieve the video inputs
    for (int v = 0; v<ip.V; v++) {
        Video video;
        video.idV = v;
        std::cin >> video.vsize;

        video_sizes[v] = video;
    }

    // Retrieve the endpoints
    int latency_dc, K;
    for (int e = 0; e<ip.E; e++) {
        std::cin >> latency_dc >> K;
        endpoints[e].dc_latency = latency_dc;
        endpoints[e].idE = e;
        endpoints[e].K = K;
        
        /// And there connections
        EndpointCacheConnection* endpoint_connections = (EndpointCacheConnection*)malloc(K*sizeof(EndpointCacheConnection));
        for (int k = 0;  k<K; k++) {
            std::cin >> endpoint_connections[k].idC >> endpoint_connections[k].cache_latency;
        }
        endpoints[e].endpoint_connections = endpoint_connections;
    }
    
    // Retrieve the requests
    int idV, idE, count;
    for (int r = 0; r<ip.R; r++) {
        Request demand;
        std::cin >> demand.idV >> demand.idE >> demand.count;
        requests[r] = demand;
    }

    // Pack the information
    InstanceData instance = {ip, video_sizes, endpoints, requests, cache_affectation};
    
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


