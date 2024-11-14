// -----------------------------------------------------------------------
// GLOBAL VARIABLES (!!!! WE USE #define and capital letters only !!!!)
// -----------------------------------------------------------------------

#define MAX_ENTITY_COUNT 1024
#define MAX_LIGHTS 40

#define PLAYABLE_WIDTH 400
#define GRID_WIDTH 13
#define GRID_HEIGHT 13

// Here we can define configuration (setup) variables for certain game designs

// OBSTACLE CONFIGURATION
#define SPAWN_RATE_ALL_OBSTACLES 0.1 // (0.0 - 1.0 == 0% - 100%)

// These below need to be less then 1.0
#define SPAWN_RATE_DROP_OBSTACLE    0.025
#define SPAWN_RATE_BLOCK_OBSTACLE   0.03
#define SPAWN_RATE_HARD_OBSTACLE    0.00
#define SPAWN_RATE_BEAM_OBSTACLE    0.05
#define SPAWN_RATE_INVERTED_BEAM_OBSTACLE   0.05

#define DROP_RATE_POSITIVE_EFFECT 0.05