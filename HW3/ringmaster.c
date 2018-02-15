#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include "potato.h"

/************************************************
 * ECE650 -- Homwork#3 -- Connor Grehlinger     *
 * Hot Potato -- 02/13/18                       *
 ************************************************/

/* This struct holds the player attributes necessary for
 * the ringmaster to communicate with each player */

typedef struct player_info_t{
  unsigned int player_id;
  char is_connected;
  int input_fd;   
  char input_fifo[100];
  int output_fd;
  char output_fifo[100];

} player_info;



/* Initializes connections from ringmaster to players and between players */
void create_connections(player_info * player_list, unsigned int num_players){
  if (num_players <= 1){
    fprintf(stderr, "Error: num_players must be greater than or equal to 1\n");
    exit(EXIT_FAILURE);
  }
  char right_fifo_out[100];
  char left_fifo_out[100];

  for (int i = 0; i < num_players; i++){
    // create connections to ringmaster
    // input and output is relative to ringmaster
    snprintf(player_list[i].input_fifo, sizeof(player_list[i].input_fifo), 
	     "/game/p%d_to_master", i);

    snprintf(player_list[i].output_fifo, sizeof(player_list[i].output_fifo),
	     "/game/master_to_p%d", i);
    
    // unlink? 
    //unlink(player_list[i].input_fifo);
    //unlink(player_list[i].output_fifo);
    
    // making fifos for ringmaster input
    if (mkfifo(player_list[i].input_fifo, 0666) != 0){ // error checking
      fprintf(stderr, "Error: failed to make ringmaster input fifo for %s\n",
	      player_list[i].input_fifo);
      exit(EXIT_FAILURE);
    }
    // making fifos for ringmaster output
    if (mkfifo(player_list[i].output_fifo, 0666) != 0){ // error checking
      fprintf(stderr, "Error: failed to make ringmaster input fifo for %s\n",
	      player_list[i].output_fifo);
      exit(EXIT_FAILURE);
    }
    
    // create connections between players (right)
    if (i == (num_players - 1)){ // last player in the ring
      snprintf(right_fifo_out, sizeof(right_fifo_out), "/game/p%d_to_p%d", i, 0);
    }
    else{
      snprintf(right_fifo_out, sizeof(right_fifo_out), "/game/p%d_to_p%d", i, i+1);
    }
    // create connections between players (left)
    if (i == 0){ // first player in the ring
      snprintf(left_fifo_out, sizeof(left_fifo_out), "/game/p%d_to_p%d", i,  num_players-1);
    }
    else{
      snprintf(left_fifo_out, sizeof(left_fifo_out), "/game/p%d_to_p%d", i, i-1);
    }

    // unlink? 
    //unlink(right_fifo_out);
    //unlink(left_fifo_out);
    

    // making fifos for player-player communication
    if (mkfifo(right_fifo_out, 0666) != 0){ // error checking
      fprintf(stderr, "Error: failed to make fifo for %s\n", right_fifo_out);
      exit(EXIT_FAILURE);
    }
    // making fifos for ringmaster output
    if (mkfifo(player_list[i].output_fifo, 0666) != 0){ // error checking
      fprintf(stderr, "Error: failed to make fifo for %s\n", left_fifo_out);
      exit(EXIT_FAILURE);
    }
  }
}




/* USAGE:
 * 
 * invoke process as: ./ringmaster <port_num> <num_players> <num_hops>
 *
 * port_num = port for ringmaster to communicate with players 
 * num_players = number of players in hot potato game 
 * num_hops = number of hops the potato takes in game 
 *
 */
int main(int argc, char* argv[]){
  // Validate command line arguments
  if (argc != 4){
    fprintf(stderr, "Error: usage is ./ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }
  if ((atoi(argv[1]) < 0) || (atoi(argv[1]) > 65535)){
    fprintf(stderr, "Error: please choose a valid port number\n");
    exit(EXIT_FAILURE);
  }
  if(atoi(argv[2]) <= 1){
    fprintf(stderr, "Error: num_players must be greater than 1\n");
    exit(EXIT_FAILURE);
  }
  if((atoi(argv[3]) < 0) || (atoi(argv[3]) > 512)){
    fprintf(stderr, "Error: num_hops must be in the range [0-512]\n");
    exit(EXIT_FAILURE);
  }
  const char* port_num = argv[1];
  const unsigned int num_players = atoi(argv[2]);
  const unsigned int num_hops = atoi(argv[3]);

  
  // Initial output
  printf("Potato Ringmaster\n");
  printf("Players = %u\n", num_players);
  printf("Hops = %u\n", num_hops);

  // Initialize potato
  potato hot_potato = {.num_hops = num_hops, .current_hop = 0};
  //printf("Potato's num_hops = %u, current_hops = %u\n", hot_potato.num_hops,
  //	 hot_potato.current_hop);

  // Ringmaster must set up connections with players
  
  int status;
  int ringmaster_socket_fd;
  struct addrinfo ringmaster_info;
  struct addrinfo* player_info_list;
  const char* ringmaster_name = NULL;
  //const char* player_port = "4444";
  
  // Initialize ringmaster addrinfo struct
  memset(&ringmaster_info, 0, sizeof(ringmaster_info));
  
  // Set the hints parameter to select the following attributes
  
  ringmaster_info.ai_family = AF_UNSPEC; // allows IPv4 or IPv6
  ringmaster_info.ai_socktype = SOCK_STREAM; // use TCP
  //ringmaster_info.ai_flags = don't set for now, keep generic connections 

  status = getaddrinfo(ringmaster_name, port_num, &ringmaster_info, &player_info_list);
  if (status != 0){
    fprintf(stderr, "Error: getaddrinfo() call failed\n");
    exit(EXIT_FAILURE);
  }
  // now you have a list of players 
  
  ringmaster_socket_fd = socket(player_info_list->ai_family, 
				player_info_list->ai_socktype, 
				player_info_list->ai_protocol);
  // check for socket call error
  if (ringmaster_socket_fd == -1){
    fprintf(stderr, "Error: socket() call failed\n");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  
  // Set the ringmaster's socket options 
  status = setsockopt(ringmaster_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  // Bind the player's address to the ringmaster's socket (via file descriptor)
  status = bind(ringmaster_socket_fd, player_info_list->ai_addr, player_info_list->ai_addrlen);
  // now the ringmaster can listen for player communication on this socket
  if (status == -1){
    fprintf(stderr, "Error: cannot bind to socket\n");
    exit(EXIT_FAILURE);
  }

  // Listen for information from players
  status = listen(ringmaster_socket_fd, 100);
  if (status == -1){
    fprintf(stderr, "Error: cannot listen on socket\n");
    exit(EXIT_FAILURE);
  }
  printf("Listening for player info on port %s\n", port_num);

  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  int player_fd;
  // Connect to a player, the accept() call will block 
  player_fd = accept(ringmaster_socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  // now played_fd can be used for reading and writing 
  if (player_fd == -1){
    fprintf(stderr, "Error: accept() call failed\n");
    exit(EXIT_FAILURE);
  }
  
  potato potato_buffer[1] = {}; // used to hold passed data

  recv(player_fd, potato_buffer, sizeof(potato), 0); // no flags, accept potato
  
  printf("Value of potato.num_hops = %u (expected 65)\n",
	 potato_buffer[0].num_hops);

  printf("Value of potato.current_hop = %u (expected 66)\n",
	 potato_buffer[0].current_hop);
  
  
  freeaddrinfo(player_info_list);
  		
  close(ringmaster_socket_fd);



  



//------------ game start, game play and game end implementation below ------------------

  //for (int i = 0; i < num_players; i++){ // for each player
  //}


// Setting up player connections
//printf("Player %u is ready to play\n");

// Ringmaster output (remaining output)

// Starting game
//printf("Ready to start the game, sending potato to player %u\n")

// Ending game 
//printf("Trace of potato:\n")
//for (int i = 0; i < num_hops; i++){
//  walk through and print each player ID from 
//  game start to game end 


  return EXIT_SUCCESS;
}







