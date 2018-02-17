





int create_player_socket(struct addrinfo * player){
  int player_socket = socket(current_player->ai_family,
			     current_player->ai_socktype,
			     current_player->ai_protocol);
  if (player_socket == -1){
    fprintf(stderr, "Error: socket() call failed\n");
    exit(EXIT_FAILURE);
  }
  return player_socket;
}




void print_address(struct addrinfo * host){
  const struct sockaddr * address_to_connect = host->ai_addr;
  struct sockaddr_in * address_to_connect_in =
    (struct sockaddr_in *) address_to_connect;
  char * ip = inet_ntoa(address_to_connect_in->sin_addr);
  printf("Address = '%s'\n", ip);
}
