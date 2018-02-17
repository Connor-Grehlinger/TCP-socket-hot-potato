#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>



/* ECE650 -- Homework3 -- Connor Grehlinger
 * 
 * "Potato" struct for passing and record keeping in Hot Potato game
 *
 *
 */


// There is a constraint on the number of hops
#define MAX_HOPS 512 

typedef struct potato_t{
  // number of hops in the game:
  unsigned int num_hops; 
  // array of player ids, limit size to MAX_HOPS:
  int player_ids[MAX_HOPS];
  // the current hop in the game:
  unsigned int current_hop; 
  
  // when (num_hops == current_hop) the game is over
  
} potato;


typedef struct player_setup_t{
  int player_id;
  struct addrinfo player_info;

  int right_player_id;
  int left_player_id;
  struct addrinfo left_player_info;
  struct addrinfo right_player_info;

} player_setup;
