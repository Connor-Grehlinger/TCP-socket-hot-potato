#include "potato.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
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



/* USAGE:
 *
 * invoke process as: ./player <machine_name> <port_num>
 * machine_name = machine running ringmaster process
 * port_num = port used by ringmaster to communicate with players
 * 
 */
int main(int argc, char* argv[]){
  // Validate command line arguments:
  /*               
  if (argc != 3){
    fprintf(stderr, "Error: usage is ./player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }
  */
  if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 65535)){
    fprintf(stderr, "Error: please choose a valid port number\n");
    exit(EXIT_FAILURE);
  }
  
  const char * port_num = argv[2];
  
  int status;
  int player_ringmaster_soc;
  struct addrinfo player_info;
  struct addrinfo *host_info_list;
  const char *ringmaster_name = NULL;//argv[1];
  
  memset(&player_info, 0, sizeof(player_info));
  player_info.ai_family   = AF_UNSPEC;
  player_info.ai_socktype = SOCK_STREAM;

  // Get information of ringmaster and other players 
  status = getaddrinfo(ringmaster_name, port_num, &player_info, &host_info_list);
  if (status != 0) {
    fprintf(stderr, "Error: getaddrinfo() call failed\n");
    exit(EXIT_FAILURE);
  }

  player_ringmaster_soc = generate_socket_fd(host_info_list);

  
  printf("Connecting to ringmaster on port %s\n",  port_num);


  // --- Get player_id and info from ringmaster ---

  // Set the players socket options 
  int yes = 1;
  status = setsockopt(player_ringmaster_soc, SOL_SOCKET, SO_REUSEADDR,
		      &yes, sizeof(int)); 


  status = bind(player_ringmaster_soc, host_info_list->ai_addr,
		host_info_list->ai_addrlen);
  // Player's socket is now bound to ringmaster address
  if (status == -1){
    fprintf(stderr, "Error: cannot bind to socket (bind call failed)\n");
    exit(EXIT_FAILURE);
  }    

  status = listen(player_ringmaster_soc, 100);
  if (status == -1){
    fprintf(stderr, "Error: cannot listen on socket (listen() call failed\n");
    exit(EXIT_FAILURE);
  }
  printf("Listening for ringmaster on port %s\n", port_num); 

  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  int player_ringmaster_con;
  
  player_ringmaster_con = accept(player_ringmaster_soc, (struct sockaddr *)&socket_addr, &socket_addr_len);
  
  // now player_ringmaster_con can be used for reading and writing
  if (player_ringmaster_con == -1){
    fprintf(stderr, "Error: accept() call failed\n");
    exit(EXIT_FAILURE);
  }
  
  char player_id_buffer[1000];
  ssize_t bytes_recv;
  bytes_recv = recv(player_ringmaster_con, player_id_buffer, sizeof(char) * 1000, 0);

  printf("Number of bytes received = %li, this is currently set to the index of the null character for the received id\n", bytes_recv);

  for (int i = 0; player_id_buffer[i] != '\0'; i++){
    printf("position %d is before the null character in received ringmaster number\n", i);
  }

  // make sure num is not negative!!!
  player_id_buffer[2] = '\0';
  printf("Received player_id = %s\n", player_id_buffer);
  
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

  printf("Replying to ringmaster...\n");


  //const char * reply_to_copy = "I got the number ";
   
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
  
  // Free dynamically allocated linked list of addrinfo structs
  freeaddrinfo(host_info_list);
  close(player_ringmaster_soc);

  // do subsequent checks for opening connections/etc. 


  return EXIT_SUCCESS;
}






