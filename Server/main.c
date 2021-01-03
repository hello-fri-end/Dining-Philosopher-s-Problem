#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<semaphore.h>
#include<unistd.h>
#include<pthread.h>
#include<netinet/in.h>
#include<arpa/inet.h>

int counter=0; //no. of philosophers connected at any given time

#define MAX 8 //maximum 100 philosophers allowed
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define LEFT (ph_num+ MAX -1)%MAX
#define RIGHT (ph_num +1)%MAX

sem_t mutex[MAX];
sem_t S[MAX]; 

int state[MAX];
int phil_num[MAX];

void *philosopher_handler(void *ptr);

typedef struct handler_thread_args
{
  int *socket;
  int ph_num;
} handler_thread_args;

void take_fork(int);
void put_fork(int);
void test(int);

int main()
{
  int i; //iteration variable

  //initialize phil_num and semaphores
  for(i=0;i<MAX;i++)
  {
    phil_num[i]= i;
    sem_init(&mutex[i],0,1); //every philosopher is allowed to try to EAT initiallu
    sem_init(&S[i], 0, 0); //block each philosopher till he has eaten
  }

  //server socket
  int server_socket;
  if( (server_socket= socket(AF_INET, SOCK_STREAM,0) ) == -1)
  {
    printf("Unable to create a socket\n");
    return 1;
  }

  //server's IP and port information
  struct sockaddr_in server_address;
  server_address.sin_family= AF_INET;
  server_address.sin_port= htons(8000);
  server_address.sin_addr.s_addr=INADDR_ANY;
  memset(&(server_address.sin_zero), '\0', 8); //pad rest of the struct with zeros

  //bind the IP and port with the socket
  if( bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address) ) == -1)
  {
    printf("Binding Error\n");
    return 1;
  }


  pthread_t philosophers[MAX]; //one thread for each philosopher

  //listen for a maximum of max connections
  if(listen(server_socket,MAX) == -1)
  {
    printf("Can't listen of the specified port\n");
    return 1;
  }

  //keep accepting new connections
  
  int new_sockets[MAX];
  struct sockaddr_in client_address; //client's ip and port info
  memset(&(client_address.sin_zero),'\0', 8);
  socklen_t len= sizeof(client_address);

  while(1)
  {
    if( (new_sockets[counter] = accept(server_socket, (struct sockaddr*)&client_address, &len)) == -1)
    {
      printf("Error in accepting connection\n");
      close(server_socket);
      return 1;
    }

    counter++;
    printf("\n\n[]Connected to a new philosopher\n[]Total philosophers = %d\n", counter);

    //each thread will handle one client
    handler_thread_args args;
    args.socket= &new_sockets[counter-1]; args.ph_num = counter -1;
    pthread_create(&philosophers[counter-1], NULL, philosopher_handler, (void *)&args);
    printf("\n[]---Philosopher[%d] is currently thinking---[]\n", counter);

  }
  for(i=0; i<MAX; i++)
  pthread_join(&philosophers[i], NULL);
  
  close(server_socket);

  return 0;
}

void *philosopher_handler(void *ptr)
{
  int ph_num= ((handler_thread_args*)ptr)->ph_num;
  int *new_socket =((handler_thread_args*)ptr)->socket; 
   
  //send the philosopher his id no.
  send(*new_socket,&ph_num, sizeof(ph_num), 0);

  while(1)
  {
  //wait for the philosopher to signal that he's hungry
  //the philosopher should send 1 if he's hungry
  int response;
  recv(*new_socket, &response, sizeof(response), 0);

  //some error checks could be done at this point to validate the response
  //i'm going to assume the philosopher alwyas sends a valid response
  state[ph_num]= HUNGRY;
  printf("\n--------[]Philosopher[%d] is currently hungry-------[]\n", ph_num+1);

  //try to take the chopsticks
  take_fork(ph_num);

  //when take_fork() returns, that means permission has been given to the Philosopher to eat
  //inform the Philosopher
  int signal=1 ;
  send(*new_socket,&signal, sizeof(signal), 0);
  printf("\n[]-----Philosopher[%d] is eating with forks %d and %d-----[]\n", ph_num+1, LEFT+1, ph_num+1);

  //wait for Philosopher to send the signal that he has finished eating
  recv(*new_socket, &response, sizeof(response), 0);

  //again validation checks can be done here
  //but i'll assume Philosopher will send a valid response
  printf("[]---Philosopher[%d] has finishd eating and wants to return the chopsticks---[]\n", ph_num+1);

  //return the chopsticks
  put_fork(ph_num);
  }
close(*new_socket);
}


void take_fork(int ph_num)
{
  //Philosopher should only try to take the fork if the neighbours are not currenly trying/eating
  //get the value of neighbour's semaphores
  int left, right;
  sem_getvalue(&mutex[LEFT], &left); sem_getvalue(&mutex[RIGHT],&right);
  //if neighbours are not currenly blocked, block them
  if(left!=0)
    sem_wait(&mutex[LEFT]);
  if(right!=0)
    sem_wait(&mutex[RIGHT]);

  sem_wait(&mutex[ph_num]); //block own semaphore
  test(ph_num);
  sem_post(&mutex[LEFT]); sem_post(&mutex[RIGHT]); sem_post(&mutex[ph_num]);
  //if test passed, block the Philosopher's semaphore again & inform him tha he can start eating
  //else wait in blocked state for the neighbours to finish eating

  sem_wait(&S[ph_num]);
}

void test(int ph_num)
{
  if(state[ph_num]== HUNGRY && state[LEFT]!= EATING && state[RIGHT]!= EATING)
  {
    state[ph_num]= EATING;
    printf("\n[]-Philosopher[%d] is given the permission to eat-[]\n", ph_num+1);
    sem_post(&S[ph_num]); //release the lock on the philosopher's semaphore
  }
}

void put_fork(int ph_num)
{
  //for neighbours, putting and taking fork should not be allowed at the same time
  //also two neighbours should not put at the same time
  //place lock's on neighbours if not already placed
   
  int left, right;
  sem_getvalue(&mutex[LEFT], &left); sem_getvalue(&mutex[RIGHT], &right);

  if(left!=0)
    sem_wait(&mutex[LEFT]);
  if(right!=0)
    sem_wait(&mutex[RIGHT]);

  sem_wait(&mutex[ph_num]);
  state[ph_num] = THINKING;
  printf("\n[]Philosopher[%d] has put forks %d and %d down and is now thinking\n[]", ph_num +1, LEFT+1, ph_num+1);

  //test if left and right neighbours are hungry and is it possible for them to start eating
  test(LEFT);
  test(RIGHT);
  sem_post(&mutex[LEFT]); sem_post(&mutex[RIGHT]); sem_post(&mutex[ph_num]);
}
