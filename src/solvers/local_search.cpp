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
long long compute_updated_score(int id_video, InstanceData* instance) {
    long long score_diff = 0;
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

        score_diff += (long long)(best_gain - current_request_gain) * instance->requests[id_request].count;
    }

    instance->score += score_diff;

    return instance->score;
}

/// @brief Function that adds or remove the given video in the given cache depending on whether the video is already in the cache
/// @param instance
/// @param id of the cache
/// @param id of the video
/// @return updated score of the new solution or -1 if the assignment was impossible
long long add_remove(InstanceData* instance, int id_cache, int id_video) {
    bool currently_placed = instance->cache_affectation[id_video][id_cache];
    int video_size = instance->videos[id_video].vsize;

    // Check if there is the move is possible
    if (!currently_placed) {
        int remaining_space = instance->caches[id_cache].left_memory;

        if (remaining_space < video_size) return -1;
    }

    // Add the video if it wasn't already there, remove it if it was
    instance->cache_affectation[id_video][id_cache] = !currently_placed;

    // Keep the cache's remaining space in sync with the toggle
    instance->caches[id_cache].left_memory += currently_placed ? video_size : -video_size;

    // Recompute the score
    long long updated_score = compute_updated_score(id_video, instance);

    return updated_score;
}

/// @brief Function that computes the score of an add or remove without affecting the instance
/// @param instance 
/// @param id_cache 
/// @param id_video 
/// @return score of the add or remove
long long try_add_remove(InstanceData* instance, int id_cache, int id_video) {
    long long score = add_remove(instance, id_cache, id_video);

    add_remove(instance, id_cache, id_video);

    return score;
}

/// @brief Function that swaps a given video from a given cache to another given cache
/// @param instance
/// @param id of the cache 1
/// @param id of the video
/// @param id of the cache 2
/// @return updated score of the new solution or -1 if the assignment was impossible
long long swap(InstanceData* instance, int id_cache1, int id_video, int id_cache2) {
    if (instance->cache_affectation[id_video][id_cache1] == instance->cache_affectation[id_video][id_cache2] || instance->cache_affectation[id_video][id_cache1] == 0) return -1;

    add_remove(instance, id_cache1, id_video);

    long long score_second_change = add_remove(instance, id_cache2, id_video);

    // id_cache2 didn't have room: put the video back on id_cache1 so a failed
    // swap has no side effect, same as a failed add_remove
    if (score_second_change == -1) {
        add_remove(instance, id_cache1, id_video);
        return -1;
    }

    return score_second_change;
}

/// @brief Function that computes the score of a swap without affecting the instance
/// @param instance 
/// @param id_cache1 
/// @param id_video 
/// @param id_cache2 
/// @return score of the swap
long long try_swap(InstanceData* instance, int id_cache1, int id_video, int id_cache2) {
    long long score = swap(instance, id_cache1, id_video, id_cache2);

    if(score == -1) return -1;

    swap(instance, id_cache2, id_video, id_cache1);

    return score;
}

/// @brief performs one interation of local search
/// @param instance
/// @return score of best move
long long local_search_single_iteration(InstanceData* instance) {
    long long best_score = instance->score;
    Local_move best_move = {-1, -1, -1, -1}; // Initialize with invalid move identifiers

    for(int id_video = 0; id_video < instance->ip.V; id_video++) {
        for(int id_cache = 0; id_cache < instance->ip.C; id_cache++) {
            long long score_add_remove = try_add_remove(instance, id_cache, id_video);

            if (score_add_remove > best_score) {
                best_score = score_add_remove;
                best_move = {0, id_cache, id_video, -1};
            }

            if (id_cache < instance->ip.C-1) {
                for (int id_cache_swap = id_cache+1; id_cache_swap < instance->ip.C; id_cache_swap++) {
                    long long score_swap = try_swap(instance, id_cache, id_video, id_cache_swap);

                    if (score_swap > best_score) {
                        best_score = score_swap;
                        best_move = {1, id_cache, id_video, id_cache_swap};
                    }
                }
            }
        }
    }

    if (best_move.move == 0) {
        add_remove(instance, best_move.id_cache_1, best_move.id_video);
    }
    else if (best_move.move == 1) {
        swap(instance, best_move.id_cache_1, best_move.id_video, best_move.id_cache_2);
    }

    return best_score;
}

/// @brief Performs n iterations of local search
/// @param instance 
/// @param num_iterations 
/// @return final score
long long local_search(InstanceData* instance, int num_iterations) {
    long long previous_score = instance->score;
    long long score = instance->score;
    for (int i = 0; i < num_iterations; i++) {
        score = local_search_single_iteration(instance);

        if (score == previous_score) return i;

        previous_score = score;
    }

    return score;
}

/// @brief Performs num_moves random local moves to ruin the current solution
/// @param instance
/// @param num_moves
/// @return score of the ruined solution
long long ruin(InstanceData* instance, int num_moves) {
    // Use C++ standard library for randomization
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < num_moves; ++i) {
        // 1. Select a random video to modify
        std::uniform_int_distribution<> dist_video(0, instance->ip.V - 1);
        int id_video = dist_video(gen);

        // 2. Randomly decide between Add/Remove (0) and Swap (1)
        std::uniform_int_distribution<> dist_move_type(0, 1);
        int move_type = dist_move_type(gen);

        if (move_type == 0) { // Add/Remove
            // Randomly select a cache to modify the video in.
            std::uniform_int_distribution<> dist_cache(0, instance->ip.C - 1);
            int id_cache = dist_cache(gen);

            // Apply the change permanently (ruining it)
            add_remove(instance, id_cache, id_video);
        } else { // Swap
            // Randomly select two distinct caches for swapping.
            std::uniform_int_distribution<> dist_cache_1(0, instance->ip.C - 1);
            int id_cache1 = dist_cache_1(gen);

            int id_cache2;
            do {
                id_cache2 = dist_cache_1(gen);
            } while (id_cache2 == id_cache1);

            // Apply the swap permanently (ruining it)
            swap(instance, id_cache1, id_video, id_cache2);
        }
    }
    return instance->score;
}

/// @brief Performs num_rr_iterations of num_moves ruin followed by at most num_ls_iterations of local search to recreate a good solution
/// @param instance
/// @param num_rr_iterations
/// @param num_moves
/// @param num_ls_iterations
/// @return best score 