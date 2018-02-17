#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "potato.h"

/************************************************
 * ECE650 -- Homwork#3 -- Connor Grehlinger     *
 * Hot Potato -- 02/13/18                       *
 ************************************************/


/* This struct holds the player attributes necessary for
 * the ringmaster to communicate with each player */
typedef struct player_info_t{
  int player_id;
  char is_connected;
  int input_socket;   
  int output_socket;
  player_setup * player_setup;
  // host_info * player_host_info;
  
} player_info;



/* Abstracting out functionality to functions for better readability */

typedef struct addrinfo host_info;

/* Generate a socket file descriptor */
int generate_socket_fd(host_info* player_info_list){
  if (player_info_list == NULL){
    fprintf(stderr, "Error: NULL addrinfo struct passed to 'generate_socket_fd'\n");
    return -1; // return to indicate error
  }
  int socket_fd = socket(player_info_list->ai_family, 
			 player_info_list->ai_socktype, 
			 player_info_list->ai_protocol);

  if (socket_fd == -1){ // check for socket call error
    fprintf(stderr, "Error: socket() call failed\n");
    return -1;
  }
  return socket_fd;
}



// The ringmaster will have a connection to each of the players which it
// monitors for game end and uses for set up and other info

// need to create a set of file descriptors for ringmaster to monitor
// the players 


// need to send player set up information:
// - player ID
// - information of other players and how to connect to them
//    - this means sending the linked list of addrinfo structs
//    - these structs contain the ip address of the other found hosts


/* Wrapper function for the getaddrinfo call
 * This function places a linked list of addrinfo structs
 * corresponding to found player processes into the
 * player_info_list argument. This works because the getaddrinfo
 * call will return a list of all connections matching the hints
 * parameter and listening on this 
 */
void search_for_players(const char * host_name, const char * port,
			host_info* target_hosts_spec, host_info** player_info_list){
  // Port should be specified
  if (!port){
    fprintf(stderr, "Error: port number must be specified (search_for_players)\n");
    exit(EXIT_FAILURE);
  }  
  // Initialize addrinfo struct
  memset(target_hosts_spec, 0, sizeof(*target_hosts_spec));
  // Set the hints parameter (target_hosts_spec) to select the following attributes  
  target_hosts_spec->ai_family = AF_UNSPEC; // allows IPv4 or IPv6
  target_hosts_spec->ai_socktype = SOCK_STREAM; // use TCP
  //target_hosts_spec->ai_flags = AI_PASSIVE; // accept wildcard addresses 
  
  int status = getaddrinfo(host_name, port, target_hosts_spec, player_info_list);
  if (status != 0){
    fprintf(stderr, "Error: getaddrinfo() call failed\n");
    exit(EXIT_FAILURE);
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
  const int num_players = atoi(argv[2]);
  const unsigned int num_hops = atoi(argv[3]);

  // Initial output
  printf("Potato Ringmaster\n");
  printf("Players = %u\n", num_players);
  printf("Hops = %u\n", num_hops);

  // Initialize potato
  //potato hot_potato = {.num_hops = num_hops, .current_hop = 0};
  //printf("Potato's num_hops = %u, current_hops = %u\n", hot_potato.num_hops,
  //	 hot_potato.current_hop);

  // Ringmaster must set up connections with players  
  int status;
  int ringmaster_socket_setup;
  host_info ringmaster_info;
  host_info* player_info_list;
  const char* ringmaster_name = NULL;

  // Search for the players (error checking done in function)
  search_for_players(ringmaster_name, port_num, &ringmaster_info, &player_info_list);
  
  // Generate ringmaster's socket fd (error checking done in function)
  ringmaster_socket_setup = generate_socket_fd(player_info_list); 
  
  // Set the ringmaster's socket options 
  int yes = 1;
  status = setsockopt(ringmaster_socket_setup, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (status == -1){
    fprintf(stderr, "Error: setsockopt() call failed\n");
    exit(EXIT_FAILURE);
  }

  // For development and debugging
  // traverse the linked list to print all hosts found 
  host_info * temp = player_info_list;
  int count = 0;
  while (temp != NULL){
    const struct sockaddr * address_to_connect = temp->ai_addr;
    struct sockaddr_in * address_to_connect_in = (struct sockaddr_in *) address_to_connect;
    char * ip = inet_ntoa(address_to_connect_in->sin_addr);
    printf("Connection: %d. Address = %s\n", count, ip);
    count++;
    temp = temp->ai_next;
  }
  

  // EVERYTHING IN HERE MUST BE INITIALIZED!
  
  player_info player_arr[num_players];
  player_setup player_setups[num_players];
  //player_info* player_arr = malloc(num_players * sizeof(*player_arr));

  // N players, having IDs 0 - N-1
  int player_number = 0;
  struct addrinfo * current_player = player_info_list;

  // get the last found player (assumes all players have started)
  struct addrinfo * last_player = current_player;
  while (last_player->ai_next != NULL){
    last_player = last_player->ai_next;
  }
  const struct sockaddr * address_to_connect = last_player->ai_addr;
  struct sockaddr_in * address_to_connect_in = (struct sockaddr_in *) address_to_connect;
  char * ip = inet_ntoa(address_to_connect_in->sin_addr);
  printf("Last player's address = %s\n", ip);
  
  for (int i = 0; i < num_players ; i++){
    player_arr[i].player_id = i;
    player_arr[i].is_connected = 0;
    printf("Created player %u's id = %u\n", i,i);

    // socket
    player_arr[i].input_socket = socket(current_player->ai_family,
					current_player->ai_socktype,
					current_player->ai_protocol);
    
    if (player_arr[i].input_socket == -1){ // check for socket call error
      fprintf(stderr, "Error: socket() call failed\n");
      return -1;
    }

    // connect
    status = connect(player_arr[i].input_socket, current_player->ai_addr,
		     current_player->ai_addrlen);
    if (status == -1){
      fprintf(stderr, "Error: cannot connect to socket\n");
      exit(EXIT_FAILURE);
    }
    // create player set up struct
    player_setups[i].player_id = i;
    player_setups[i].player_info = *current_player;

    if (i == 0){
      player_setups[i].left_player_id = num_players -1;   
      player_setups[i].left_player_info = *last_player; // probably not a problem

    }
    else{
      player_setups[i].left_player_id = i - 1;
      player_setups[i].left_player_info = player_setups[i - 1].player_info;
    }
    
    if (i == num_players - 1){
      player_setups[i].right_player_id = 0;
      player_setups[i].right_player_info = player_setups[0].player_info;
    }
    else{
      player_setups[i].right_player_id = i + 1;
      player_setups[i].right_player_info = *(player_setups[i].player_info.ai_next);
    }
    const player_setup * player_setup_buffer = &player_setups[i];
    //const player_setup player_setup_buffer[1] = {player_setups[i]};
    printf("size of sent set up buffer = %zu\n", sizeof(player_setup_buffer));
    printf("size of sent set up buffer[0] = %zu\n", sizeof(*player_setup_buffer));
    //printf("This player's id = %d\n", player_setup_buffer[0].player_id);
    //printf("Right player's id = %d\n", player_setup_buffer[0].right_player_id);
    //printf("Left player's id = %d\n", player_setup_buffer[0].left_player_id);
    printf("pre send\n");
    send(player_arr[i].input_socket, player_setup_buffer, sizeof(*player_setup_buffer), 0);
    printf("Post send\n");
    // otherwise its because stuff isn't initialized in the addrinfo structs in LL
  } 
    
  // you're going to assign a fd to the players input, and send
  // the set up information 
  // socket, connect, send 
  
  srand(time(NULL));
  
  int start_player = rand() % num_players;
  printf("Start_player = %u\n", start_player);

  

  // Free dynamically allocated addrinfo linked list
  freeaddrinfo(player_info_list);
  		
  close(ringmaster_socket_setup); // not being used 

  
  // ------------ game start, game play and game end implementation below ------------------

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








  /* ------- old ----------------
  status = connect(ringmaster_socket_fd, player_info_list->ai_addr,
		   player_info_list->ai_addrlen);

  if (status == -1){
    fprintf(stderr, "Error: cannot connect to socket (connect call failed)\n");
    exit(EXIT_FAILURE);
  }

  const char * player_id = "1";
  // send the player ID
  printf("Size of send player id = %zu\n", strlen(player_id));
  send(ringmaster_socket_fd, player_id, strlen(player_id) + 1, 0); 


  printf("Reading player response...\n");

  char player_id_buffer[1000];
  ssize_t num_bytes = recv(ringmaster_socket_fd, player_id_buffer, sizeof(char) * 1000, 0);
  
  printf("Number of bytes recv = %zd\n", num_bytes);
  player_id_buffer[100] = '\0';
  printf("Received reply = %s\n", player_id_buffer);
  */ // ------- end old -------------------------


  /*  
  for(unsigned int i = 0;i < num_players; i++){
    
    status = connect(ringmaster_socket_fd, player_info_list->ai_addr,
    player_info_list->ai_addrlen);

    if (status == -1){
      fprintf(stderr, "Error: cannot connect to socket (connect call failed)\n");
      exit(EXIT_FAILURE);
    }

    
    char player_id[5];
    
    snprintf(player_id, 5, "%d", player_arr[i].player_id);
    // send the player ID
    
    printf("Player id = %s\n", player_id);
    
    printf("Size of sent player id = %zu\n", strlen(player_id));
    send(ringmaster_socket_fd, player_id, strlen(player_id) + 1, 0); 

    
    printf("Reading player response...\n");
    
    char player_id_buffer[1000];
    ssize_t num_bytes = recv(ringmaster_socket_fd, player_id_buffer, sizeof(char) * 1000, 0);
    
    printf("Number of bytes recv = %zd\n", num_bytes);
    player_id_buffer[100] = '\0';
    printf("Received reply = %s\n", player_id_buffer);
  }
  */
