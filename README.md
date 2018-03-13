# TCP-socket-hot-potato
A couple of programs for playing a game of hot potato with sockets over TCP connections

The game of hot potato can be played on a local machine using localhost ip ('127.0.0.1' for ipv4 or '::1' for ipv6), on serval machines 
remotely (assuming network firewalls allow for arbitrary port selection to communicate), or on a combination of both.

The ringmaster process is responsbile for gathering player process info (each player must know about other players), coordinating the 
game, and waiting for the end game signal. Since the game ends randomly on any of the player processes, a set of file descriptors for each
player process is monitored using the select call to avoid deadlock.

The player process is responsible for its initial contact with the ringmaster and then monitors sets of file descriptors to play the game

To play hot potato:
1) Download or clone the source code to one or many machines
2) build with the included Makefile
3) Run ONE instance of the ringmaster process, syntax: './ringmaster <port_to_listen_on> <number_of_players> <number_of_hops_in_game>'
4) Run number_of_players instances of the player process, syntax: './player <IP_of_ringmaster_machine> <port_ringmaster_listing_on>'
5) When the ringmaster and all players are up and running, the game will begin, and its trace output to the console.

