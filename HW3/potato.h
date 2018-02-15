// "Potato" struct for passing and record keeping in Hot Potato game

// There is a constraint on the number of hops
#define MAX_HOPS 512
 

typedef struct potato_t{
  // number of hops in the game:
  unsigned int num_hops; 
  // array of player ids, limit size to MAX_HOPS:
  unsigned int player_ids[MAX_HOPS];
  // the current hop in the game:
  unsigned int current_hop; 
  
  // when (num_hops == current_hop) the game is over
  
} potato;
