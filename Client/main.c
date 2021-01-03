#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<netinet/in.h>
#include<sys/socket.h>

int main()
{
  //client socket
  int client_socket;
  if( (client_socket = socket(AF_INET, SOCK_STREAM,0)) == -1)
  {
    printf("Error in assiging a socket\n");
    return 1;
  }

  //client's IP and port info
  struct sockaddr_in client_address;
  client_address.sin_family= AF_INET;
  client_address.sin_port= htons(8000);
  client_address.sin_addr.s_addr= INADDR_ANY;
  memset(&(client_address.sin_zero), '\0', 8);

  //connect to port
  if( connect(client_socket, (struct sockaddr*)&client_address, sizeof(client_address)) == -1)
  {
    printf("Error connecting to server\n");
    return 1;
  }

  //recieve id no from server
  int ph_num;
  recv(client_socket, &ph_num, sizeof(ph_num), 0);
  printf("Connected to server-> ID assigned= %d\n", ph_num);

  while(1)
  {
  printf("\n\n[]Reply to the subsequent message when you want to signal that you're hungry\n[]Otherwise server assumes you're thinking[]\n");
  int signal;
  printf("\n[]Enter 1 to signal: ");
  scanf("%d", &signal);

  //validation checks can be done  here
  //we'll assume user enter's valid input

  //send the signal to server
  send(client_socket, &signal, sizeof(signal), 0);
  printf("[]----\n\nHunger signal sent to server---[]\n");

  //recieve permission to eat from server
  int response;
  recv(client_socket, &response, sizeof(response), 0);
  printf("\n[]------Recievd chopsticks for eating------[]\n");

  printf("\n---[]Reply to the subsequent message when you want you signal that you have finished eating\n[]Otherwise server assumes that you're still eating\n");
  printf("[]Enter 1 to signal: ");
  scanf("%d", &signal);

  //send signal to server
  send(client_socket,&signal, sizeof(signal), 0);
  printf("---Chopsticks returned. Time to start thinking!---\n");
  

  }

  return 0;
}
