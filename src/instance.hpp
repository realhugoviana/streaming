#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

/*
    STRUCTURES
*/

/// @brief Directory of main informations.
struct GlobalData {
    int V; // Number of video
    int E; // Number of endpoints
    int R; // Number of requests
    int C; // Number of cache server
    int X; // Allocated size to cache
};

/// @brief Substructure to represent cache connection to an Endpoint (encapsulated as part of endpoint, hence no information are provided about the endpoint).
struct EndpointCacheConnection {
    int id_cache; // Identifier of the cache
    int latency; // Associated Latency
};

/// @brief Structure defining an endpoint by its latency to the datacenter and accessible caches.
struct Endpoint {
    int id_endpoint; // Identifier of the endpoint
    int dc_latency; // Latency to the data center
    std::vector<EndpointCacheConnection> caches_connections; // List of accessible caches
};

/// @brief Structure to link the request of a video to an endpoint.
struct Request {
    int id_video; // Identifier of the video
    int id_endpoint; // Identifier of the corresponding endpoint
    int count; // Number of time the video is being requested.
};

/// @brief Structure defining a video by its size
struct Video {
    int id_video; // Identifier of the video
    int size; // Size of the video
};

using Cache = std::vector<Video>;

/// @brief Structure that hold all the data of an instance
struct InstanceData {
    GlobalData gd;
    std::vector<Video> video_sizes;
    std::vector<Endpoint> endpoints;
    std::vector<Request> requests;
    std::unordered_map<int, Cache> caches;

    // Size of a video indexed by its identifier. Kept aside from video_sizes
    // because the filters shrink that vector, which breaks id-based indexing.
    std::vector<int> size_of_video;
};




/*
    HELPER FUNCTIONS
*/

/// @brief Function for the raw parser
/// @return The instance of the problem
inline InstanceData parser() {
    //// Retrieve global data
    GlobalData gd = {0,0,0,0,0};
    std::cin >> gd.V >> gd.E >> gd.R >> gd.C >> gd.X;

    //// Retrieve video sizes
    std::vector<Video> video_sizes;
    std::vector<int> size_of_video(gd.V, 0);
    int input;

    // Traverse the input vector
    for (int i = 0; i < gd.V; i++) {
        std::cin >> input;
        Video video;

        // Configure the video and pushback
        video.id_video = i;
        video.size = input;
        video_sizes.push_back(video);
        size_of_video[i] = input;
    }

    //// Retrieve endpoints
    // Initialise the vectors
    std::vector<Endpoint> endpoints;
    std::vector<EndpointCacheConnection> cache_connections;
    std::unordered_map<int, Cache> caches;
    Cache c;

    // Initialise the input variables and loop through the endpoints
    int latency_dc, K;
    for (int e = 0; e < gd.E; e++) {
        // Retrieve the datacenter latency
        std::cin >> latency_dc >> K;

        // Loop through connected cache, get the informations and store the cache
        for (int k = 0; k < K; k++) {
            EndpointCacheConnection connection;
            std::cin >> connection.id_cache >> connection.latency;
            caches[connection.id_cache] = c;
            cache_connections.push_back(connection);
        }
        // Generate the endpoint using the endpoint id, the latency and the vector of cache, then push the endpoint
        Endpoint endpoint = {e, latency_dc, cache_connections};
        endpoints.push_back(endpoint);

        // Clear the connections for the next endpoint
        cache_connections.clear();
    }

    //// Retrieve the requests
    // Initialise the request vector
    std::vector<Request> requests;

    // Traverse the requests
    for (int r = 0; r < gd.R; r++) {
        Request request;
        // Get the information and pushback
        std::cin >> request.id_video >> request.id_endpoint >> request.count;
        requests.push_back(request);
    }

    // Return the parsed Instance
    InstanceData instance = {gd, video_sizes, endpoints, requests, caches, size_of_video};
    return instance;
}

/// @brief Read an instance from a .in file path
/// @param path Path to the instance file
/// @return The parsed instance
inline InstanceData parseFile(const std::string& path) {
    std::ifstream infile(path);
    if (!infile.is_open()) {
        std::cerr << "Error: could not open file '" << path << "'" << std::endl;
        std::exit(1);
    }

    // Redirect std::cin's buffer to read from the file instead of the
    // terminal. parser() is untouched: it just keeps reading from std::cin.
    std::streambuf* previous = std::cin.rdbuf(infile.rdbuf());
    InstanceData instance = parser();
    std::cin.rdbuf(previous);
    return instance;
}

/// @brief Function to remove from the analysis videos that are too big to fit in any cache
/// @param instance
/// @return Return the filtered instance
inline InstanceData filterLargeVideo(InstanceData instance) {
    // Get the cache size limit
    int limit = instance.gd.X;
    std::unordered_set<int> unauthorised_requests;

    // Produce a new vector of video which does not contains the videos that are too big.
    std::vector<Video> authorisedVideo;
    for (auto& v : instance.video_sizes) {
        // If the video fit, add it, if not, remove it
        if (v.size <= limit) {
            authorisedVideo.push_back(v);
        } else {
            unauthorised_requests.emplace(v.id_video);
        }
    }

    // For each request, analyse whether a video can fit in a cache, if not, remove the request (no need to analyse it, we can do nothing about it)
    std::vector<Request> authorisedRequest;
    for (auto& r : instance.requests) {
        // If the video is authorised, then conserve it, otherwise discard it and diminish the number of requests
        if (unauthorised_requests.find(r.id_video) == unauthorised_requests.end()) {
            authorisedRequest.push_back(r);
        }
    }

    // Replace the instance and return
    instance.video_sizes = authorisedVideo;
    instance.requests = authorisedRequest;
    return instance;
}

/// @brief Function to remove videos whose cache latency is inefficient compared to the central server
/// @param instance
/// @return The filtered instance
inline InstanceData filterInefficientCache(InstanceData instance) {
    // Initialise new cache connection and authorised endpoints
    std::vector<Endpoint> authorisedEndpoint;
    std::vector<EndpointCacheConnection> authorisedCaches;

    // For each endpoint, check if their cache connection are STRICTLY faster (saves cache spaces)
    for (auto& e : instance.endpoints) {
        int limit = e.dc_latency;
        for (auto& c : e.caches_connections) {
            // Keep only caches that are faster than the server
            if (c.latency < limit) {
                authorisedCaches.push_back(c);
            }
        }
        // Update authorised caches and endpoints
        e.caches_connections = authorisedCaches;
        authorisedEndpoint.push_back(e);
        authorisedCaches.clear();
    }

    // Return filtered endpoints
    instance.endpoints = authorisedEndpoint;
    return instance;
}

/// @brief Function to filter out caches that are not accessibles.
/// @param instance
/// @return Filtered instance
inline InstanceData filterUnconnectedCache(InstanceData instance) {
    // Generate the new caches
    std::unordered_map<int, Cache> authorisedCache;
    Cache nc;

    // For each endpoint, see which cache are accessible
    for (auto& e : instance.endpoints) {
        for (auto& c : e.caches_connections) {
            // For all accessible cache, add it in the map
            authorisedCache[c.id_cache] = nc;
        }
    }

    // Change the initial map and return the new instance. gd.C stays the
    // number of caches declared by the instance: cache ids are absolute and
    // the output format is expressed with them.
    instance.caches = authorisedCache;
    return instance;
}

/// @brief Apply every filter, in order
/// @param instance
/// @return The reduced instance
inline InstanceData reduce(InstanceData instance) {
    instance = filterLargeVideo(instance);
    instance = filterInefficientCache(instance);
    instance = filterUnconnectedCache(instance);
    return instance;
}

/// @brief Method to show the parameters of the instance
/// @param instance
inline void showInstance(InstanceData* instance) {
    // Global Informations
    std::printf("\n---[Global Data]---\nNumber of Video (V): %d\nNumber of Endpoint (E): %d\nNumber of Request (R): %d\nNumber of Cache Server (C): %d\nSize of EndpointCacheConnection Server (X): %dMo\n\n",
        (int)instance->video_sizes.size(),
        instance->gd.E,
        (int)instance->requests.size(),
        instance->gd.C,
        instance->gd.X
    );

    // Information on the videos
    std::cout << "---[Videos Data]---" << std::endl;
    for (auto& v : instance->video_sizes) {
        printf("Video number [%d] of size [%dMo]\n", v.id_video, v.size);
    }

    // Information on the endpoints
    std::cout << "\n---[Endpoints Data]---" << std::endl;
    for (auto& e : instance->endpoints) {
        printf("Endpoint number [%d] with datacenter latency [%dms]\n", e.id_endpoint, e.dc_latency);
        for (auto& c : e.caches_connections) {
            printf("|. Connected to Cache [%d] with latency [%dms]\n", c.id_cache, c.latency);
        }
        std::cout << std::endl;
    }

    // Information on the requests
    std::cout << "\n---[Requests Data]---" << std::endl;
    for (auto& r : instance->requests) {
        printf("The video [%d] is requested from endpoint [%d] [%d] times\n", r.id_video, r.id_endpoint, r.count);
    }

    // Information on the caches
    std::cout << "\n---[Caches Data]---" << std::endl;
    for (auto& c : instance->caches) {
        printf("Cache number [%d]\n", c.first);
        int sum = 0;
        for (auto& v : c.second) {
            sum = sum + v.size;
            printf("|. Possess video [%d] of size [%dMo]\n", v.id_video, v.size);
        }
        printf("|.Total: [%dMo]\n", sum);
    }
}
