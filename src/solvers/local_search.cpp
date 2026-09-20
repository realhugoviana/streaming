#include <chrono>
#include <cstring>
#include <random>
#include <string>

#include "../instance.hpp"

/// @brief Structure holding a move inside the neighborhood
struct Local_move {
    int move; // Swap with another cache if 1, add or remove depending on the current state if 0, no move if -1
    int id_cache_1; // Cache from which the move is made
    int id_video; // Video from which the move is made
    int id_cache_2; // Cache with which the swap is made if the move is swap
};


/// @brief Compute the updated score after a move
/// @param id_video 
/// @param instance 
/// @return updated score
int compute_updated_score(int id_video, InstanceData* instance) {
    int score_diff = 0;
    for(const auto& id_request : instance->videos[id_video].associated_requests) {
        int id_endpoint = instance->requests[id_request].idE;
        int current_request_gain = instance->requests[id_request].gain;
        int best_gain = 0;
        int num_caches = instance->endpoints[id_endpoint].K;

        for (int i = 0; i < num_caches; i++) {
            int id_c = instance->endpoints[id_endpoint].endpoint_connections[i].idC;
            int dc_latency = instance->endpoints[id_endpoint].dc_latency;
            int cache_latency = instance->endpoints[id_endpoint].endpoint_connections[i].cache_latency;

            int gain = dc_latency - cache_latency;

            if (instance->cache_affectation[id_video][id_c] && gain > best_gain) {
                best_gain = gain;
            }
        }

        instance->requests[id_request].gain = best_gain;
        
        score_diff += best_gain - current_request_gain;
    }

    instance->score += score_diff;

    return instance->score;
}

/// @brief Function that adds or remove the given video in the given cache depending on whether the video is already in the cache
/// @param instance
/// @param id of the cache
/// @param id of the video
/// @return updated score of the new solution or -1 if the assignment was impossible
int add_remove(InstanceData* instance, int id_cache, int id_video) {
    // Check if there is the move is possible
    if (!instance->cache_affectation[id_video][id_cache]) {
        int video_size = instance->videos[id_video].vsize;
        int remaining_space = instance->caches[id_cache].left_memory;

        if (remaining_space < video_size) return -1;
    }

    // Add the video if it wasn't already there, remove it if it was
    instance->cache_affectation[id_video][id_cache] = !instance->cache_affectation[id_video][id_cache];

    // Recompute the score
    int updated_score = compute_updated_score(id_video, instance);

    return updated_score;
}

/// @brief Function that computes the score of an add or remove without affecting the instance
/// @param instance 
/// @param id_cache 
/// @param id_video 
/// @return score of the add or remove
int try_add_remove(InstanceData* instance, int id_cache, int id_video) {
    int score = add_remove(instance, id_cache, id_video);

    int a = add_remove(instance, id_cache, id_video);

    return score;
}

/// @brief Function that swaps a given video from a given cache to another given cache
/// @param instance
/// @param id of the cache 1
/// @param id of the video
/// @param id of the cache 2
/// @return updated score of the new solution or -1 if the assignment was impossible
int swap(InstanceData* instance, int id_cache1, int id_video, int id_cache2) {
    if (instance->cache_affectation[id_video][id_cache1] == instance->cache_affectation[id_video][id_cache2] || instance->cache_affectation[id_video][id_cache1] == 0) return -1;

    int score_first_change = add_remove(instance, id_cache1, id_video);

    int score_second_change = add_remove(instance, id_cache2, id_video);

    return score_second_change;
}

/// @brief Function that computes the score of a swap without affecting the instance
/// @param instance 
/// @param id_cache1 
/// @param id_video 
/// @param id_cache2 
/// @return score of the swap
int try_swap(InstanceData* instance, int id_cache1, int id_video, int id_cache2) {
    int score = swap(instance, id_cache1, id_video, id_cache2);
    
    if(score == -1) return -1;

    int a = swap(instance, id_cache2, id_video, id_cache1);

    return score;
}

/// @brief performs one interation of local search
/// @param instance
/// @return score of best move
int local_search_single_iteration(InstanceData* instance) {
    int best_score = instance->score;
    Local_move best_move = {-1, -1, -1, -1}; // Initialize with invalid move identifiers

    for(int id_video = 0; id_video < instance->ip.V; id_video++) {
        for(int id_cache = 0; id_cache < instance->ip.C; id_cache++) {
            int score_add_remove = try_add_remove(instance, id_cache, id_video);
            
            if (score_add_remove > best_score) { 
                best_score = score_add_remove;
                best_move = {0, id_cache, id_video, -1};
            }

            if (id_cache < instance->ip.C-1) {
                for (int id_cache_swap = id_cache+1; id_cache_swap < instance->ip.C; id_cache_swap++) {
                    int score_swap = try_swap(instance, id_cache, id_video, id_cache_swap);

                    if (score_swap > best_score) {
                        best_score = score_swap;
                        best_move = {1, id_cache, id_video, id_cache_swap};
                    }
                }
            }
        }
    }

    if (best_move.move == 0) {
        int score = add_remove(instance, best_move.id_cache_1, best_move.id_video);
    }
    else if (best_move.move == 1) {
        int score = swap(instance, best_move.id_cache_1, best_move.id_video, best_move.id_cache_2);
    }

    return best_score;
}

/// @brief Performs n iterations of local search
/// @param instance 
/// @param num_iterations 
/// @return final score
int local_search(InstanceData* instance, int num_iterations) {
    int previous_score = instance->score;
    int score = instance->score;
    for (int i = 0; i < num_iterations; i++) {
        score = local_search_single_iteration(instance);

        if (score == previous_score) return i;

        previous_score = score;
    }

    return score;
}