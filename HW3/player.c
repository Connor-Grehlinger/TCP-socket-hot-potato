#include "potato.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// Player output

// After receiving ringmaster set-up info
//printf("Connected as player %u out of %u total players\n")

// Sending potato to another player process
//printf("Sending potato to %u\n")

// End game call from ringmaster
//printf("I'm it\n")


/* Generate a socket file descriptor */
int generate_socket_fd(struct addrinfo* player_info_list){
  if (player_info_list == NULL){
    fprintf(stderr, "Error: NULL addrinfo struct passed to generate_socket_fd()\n");
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

void search_for_ringmaster(const char * host_name, const char * port,
			struct addrinfo* target_hosts_spec,
			struct addrinfo** ringmaster_info_list){
  // Port should be specified
  if (!port){
    fprintf(stderr,
	    "Error: port number must be specified (search_for_ringmaster)\n");
    exit(EXIT_FAILURE);
  }
  // Initialize addrinfo struct
  memset(target_hosts_spec, 0, sizeof(*target_hosts_spec));
  // Set the hints parameter (target_hosts_spec) to select the following attributes
  target_hosts_spec->ai_family = AF_UNSPEC; // allows IPv4 or IPv6
  target_hosts_spec->ai_socktype = SOCK_STREAM; // use TCP
  //target_hosts_spec->ai_flags = AI_PASSIVE; // Node is NULL, this flag ignored 

  int status = getaddrinfo(host_name, port, target_hosts_spec, ringmaster_info_list);
  if (status != 0){
    fprintf(stderr, "Error: getaddrinfo() call failed\n");
    exit(EXIT_FAILURE);
  }
}



/* USAGE:
 *
 * invoke process as: ./player <machine_name> <port_num>
 * machine_name = machine running ringmaster process
 * port_num = port used by ringmaster to communicate with players
 * 
 */
int main(int argc, char* argv[]){
  // Validate command line arguments:               
  if (argc != 3){
    fprintf(stderr, "Error: usage is ./player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }
  if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 65535)){
    fprintf(stderr, "Error: please choose a valid port number\n");
    exit(EXIT_FAILURE);
  }

  const char *ringmaster_name = argv[1];
  const char * port_num = argv[2];
    
  int status;
  
  int player_ringmaster_soc;
  int player_left_soc;
  int player_right_soc;

  struct addrinfo left_player_info;
  struct addrinfo right_player_info;
  struct addrinfo player_info_f_ringmaster;
  
  struct addrinfo player_info;
  struct addrinfo *host_info_list;

  // Get information of ringmaster 
  search_for_ringmaster(ringmaster_name, port_num, &player_info, &host_info_list);

  player_ringmaster_soc = generate_socket_fd(host_info_list);

  // For development and debugging
  // traverse the linked list to print all hosts found
  struct addrinfo * temp = host_info_list;
  int count = 0;
  while (temp != NULL){
    const struct sockaddr * address_to_connect = temp->ai_addr;
    struct sockaddr_in * address_to_connect_in =
      (struct sockaddr_in *) address_to_connect;
    char * ip = inet_ntoa(address_to_connect_in->sin_addr);
    printf("Connection: %d. Address = %s\n", count, ip);
    count++;
    temp = temp->ai_next;
  }
  
  printf("Connecting to %s on port %s\n", ringmaster_name, port_num);

  
  // --- Get player_id and info from ringmaster ---

  // Set the players socket options 
  int yes = 1;
  status = setsockopt(player_ringmaster_soc, SOL_SOCKET, SO_REUSEADDR,
		      &yes, sizeof(int)); 
  if (status == -1){
    fprintf(stderr, "Error: setsockopt() call failed\n");
    exit(EXIT_FAILURE);
  }

  // Bind the players socket to the ringmaster's connection 
  status = bind(player_ringmaster_soc, host_info_list->ai_addr,
		host_info_list->ai_addrlen);
  // Player's socket is now bound to ringmaster address
  if (status == -1){
    fprintf(stderr, "Error: cannot bind to socket (bind call failed)\n");
    exit(EXIT_FAILURE);
  }    

  // Player is now listening for ringmaster in server state 
  status = listen(player_ringmaster_soc, 100);
  if (status == -1){
    fprintf(stderr, "Error: cannot listen on socket (listen() call failed\n");
    exit(EXIT_FAILURE);
  }
  printf("Listening for %s on port %s\n", ringmaster_name, port_num); 

  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  int player_ringmaster_con;
  
  player_ringmaster_con = accept(player_ringmaster_soc, (struct sockaddr *)&socket_addr, &socket_addr_len);

  // print the ip of ringmaster 
  const struct sockaddr * connected_ringmaster = (struct sockaddr *)&socket_addr;
  struct sockaddr_in * address_to_connect_in =
    (struct sockaddr_in *) connected_ringmaster;
  char * ip = inet_ntoa(address_to_connect_in->sin_addr);
  printf("Ringmaster connection address: %s\n",ip);

  // now player_ringmaster_con can be used for reading and writing
  if (player_ringmaster_con == -1){
    fprintf(stderr, "Error: accept() call failed\n");
    exit(EXIT_FAILURE);
  }
  
  player_setup player_setup_buffer[1];
  ssize_t bytes_recv;
  bytes_recv = recv(player_ringmaster_con, player_setup_buffer, sizeof(player_setup_buffer), 0);

  if (bytes_recv == -1){
    fprintf(stderr,"Error: recv() call failed\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Number of bytes received = %li\n", bytes_recv);


  const int player_id = player_setup_buffer[0].player_id;
  const int left_player_id = player_setup_buffer[0].left_player_id;
  const int right_player_id = player_setup_buffer[0].right_player_id;

  printf("This player's id = %d\n", player_id);
  printf("Right player's id = %d\n", right_player_id);
  printf("Left player's id = %d\n", left_player_id);

  left_player_info = player_setup_buffer[0].left_player_info;
  right_player_info = player_setup_buffer[0].right_player_info;;
  player_info_f_ringmaster = player_setup_buffer[0].player_info;;
    

  
  printf("Replying to ringmaster...\n");
  
  
  // Free dynamically allocated linked list of addrinfo structs
  freeaddrinfo(host_info_list);
  close(player_ringmaster_soc);




  return EXIT_SUCCESS;
}


  /*
  for (int i = 0; player_id_buffer[i] != '\0'; i++){
    printf("--- position %d is non-null ---\n", i);
  }
  */
  
  //player_id_buffer[2] = '\0';
  //printf("Received player_id = %s\n", player_id_buffer);

  // have to receive information of other players here too

  
  //potato potato_buffer[1] = {}; // used to hold passed data
  //recv(player_fd, potato_buffer, sizeof(potato), 0); // no flags, accept potato
  /*
  status = connect(player_socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1){
    fprintf(stderr, "Error: cannot connect to socket\n");
    exit(EXIT_FAILURE);
  }
  */
  //const char *message = "hi there!";
  //const potato hot_potato = {.num_hops = atoi(argv[3]),
  //			     .current_hop = atoi(argv[4])};
  //printf("Potato's num_hops = %u, current_hops = %u\n", hot_potato.num_hops, hot_potato.current_hop);
  // send the potato 
  //send(player_socket_fd, &hot_potato, sizeof(potato), 0);




  /*
   
  const char * received_num = player_id_buffer;
  printf("String to be appended = '%s'\n", received_num);
  int num_recv = player_id_buffer[0] - '0';
  printf("Coverted int of string to be appended = '%d'\n", num_recv);

  char reply_num_recv[100];
  strcpy(reply_num_recv, "Player received the number ");
  strcat(reply_num_recv, received_num);

  
  // strlen is number of characters up to NULL

  //strcat(reply,received_num); // seg faulting here 

  printf("String length of reply string = %zu\n", strlen(reply_num_recv));
  send(player_ringmaster_con, reply_num_recv, sizeof(char)*(strlen(reply_num_recv)+1), 0); 
  */
