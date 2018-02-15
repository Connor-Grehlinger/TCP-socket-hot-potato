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
  
  const char * port_num = argv[2];
  
  int status;
  int player_socket_fd;
  struct addrinfo player_info;
  struct addrinfo *host_info_list;
  const char *ringmaster_name = argv[1];
  //const char *port     = "4444";
  

  memset(&player_info, 0, sizeof(player_info));
  player_info.ai_family   = AF_UNSPEC;
  player_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(ringmaster_name, port_num, &player_info, &host_info_list);
  if (status != 0) {
    fprintf(stderr, "Error: getaddrinfo() call failed\n");
    exit(EXIT_FAILURE);
  }
  player_socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (player_socket_fd == -1) {
    fprintf(stderr, "Error: socket() call failed\n");
    exit(EXIT_FAILURE);
  }
  printf("Connecting to %s on port %s\n", ringmaster_name, port_num);

  
  status = connect(player_socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1){
    fprintf(stderr, "Error: cannot connect to socket\n");
    exit(EXIT_FAILURE);
  }

  //const char *message = "hi there!";

  const potato hot_potato = {.num_hops = 65, .current_hop = 66};
  //printf("Potato's num_hops = %u, current_hops = %u\n", hot_potato.num_hops, hot_potato.current_hop);
  
  
  send(player_socket_fd, &hot_potato, sizeof(potato), 0);

  freeaddrinfo(host_info_list);
  close(player_socket_fd);


  // do subsequent checks for opening connections/etc. 


  return EXIT_SUCCESS;
}







