#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <math.h>

#define BUFFSIZE 1551
#define MAX_SUBSCRIBERS 10
#define MAX_TCP_MESSAGE_STORE 31

struct server_message{
  int command;
  char topic[50];
  int sf;
  char id[10];
};

struct tcp_message{
  char ip[16];
  uint16_t port;
  char topic[51];
  char type[20];
  char content[1500];
};

struct udp_message{
  char topic[50];
  uint8_t type;
  char content[1500];
};

struct topic{
  char name[50];
  int sf;
  struct tcp_message *storeTcpMessages[MAX_TCP_MESSAGE_STORE];
  int lenTcpMessages;
};

struct subscriber{
  int connected;
  char id[10];
  int socket;
  struct sockaddr_in subscriberAddr;
  socklen_t lenSubscriberAddr;
  struct topic topic[16];
  int nrTopic;
};

// vector in care retin fiecare subscriber
struct subscriber subscribers[MAX_SUBSCRIBERS];
int nrSubscribers;

void addSubscriber(char id[],int subscriberSocket,struct sockaddr_in subscriberAddr,socklen_t lenSubscriberAddr){

  subscribers[nrSubscribers].connected = 1;
  memcpy(&subscribers[nrSubscribers].id,id,strlen(id));
  subscribers[nrSubscribers].socket = subscriberSocket;
  memcpy(&subscribers[nrSubscribers].subscriberAddr,&subscriberAddr,sizeof(struct sockaddr));
  subscribers[nrSubscribers].lenSubscriberAddr = lenSubscriberAddr;
  nrSubscribers++;

}

// Funtie ce imi adauga subcriber-ul in vectorul de Subscribers si care ii trimite fiecare topic ratat in perioada in care a fost deconectat, in cazul in 
// care avea functia de 'sf' activa 
void reconnectSubscriber(int indexSubscriber,int subscriberSocket,struct sockaddr_in subscriberAddr,socklen_t lenSubscriberAddr){
  int ret;
  subscribers[indexSubscriber].connected = 1;
  subscribers[indexSubscriber].socket = subscriberSocket;
  memcpy(&subscribers[indexSubscriber].subscriberAddr,&subscriberAddr,sizeof(struct sockaddr));
  subscribers[indexSubscriber].lenSubscriberAddr = lenSubscriberAddr;

  for(int indexTopic = 0; indexTopic < subscribers[indexSubscriber].nrTopic; indexTopic++){
    
    if(subscribers[indexSubscriber].topic[indexTopic].sf == 1){
      
      struct tcp_message tcpMessage;
      for(int indexTcpMessage = 0; indexTcpMessage < subscribers[indexSubscriber].topic[indexTopic].lenTcpMessages; indexTcpMessage++){
        memset(&tcpMessage,0,sizeof(struct tcp_message));
        memcpy(&tcpMessage , subscribers[indexSubscriber].topic[indexTopic].storeTcpMessages[indexTcpMessage],sizeof(struct tcp_message));
        memcpy(tcpMessage.ip, inet_ntoa(subscribers[indexSubscriber].subscriberAddr.sin_addr),16);
        tcpMessage.port = subscribers[indexSubscriber].subscriberAddr.sin_port;


        ret = send(subscribers[indexSubscriber].socket, &tcpMessage, sizeof(struct tcp_message), 0);
        // DIE(ret < 0, "Send ID to server didn't work!!");

      }

      for(int i = 0; i < subscribers[indexSubscriber].topic[indexTopic].lenTcpMessages; i++){
        memset( subscribers[indexSubscriber].topic[indexTopic].storeTcpMessages[i],0,sizeof(struct tcp_message));
      }
      subscribers[indexSubscriber].topic[indexTopic].lenTcpMessages=0;

    }

  }

}

void initServer(int *tcpSocket,int *udpSocket,struct sockaddr_in tcpAddr,struct sockaddr_in udpAddr,int port,struct pollfd listFD[]){
  int ret;

  (*tcpSocket) = socket(AF_INET, SOCK_STREAM, 0);
  // DIE((*tcpSocket) < 0, "TCP socket counldn't be created!!");

  (*udpSocket) = socket(AF_INET, SOCK_DGRAM, 0);
  // DIE((*udpSocket) < 0, "UDP socket counldn't be created!!");

  // DIE(port < 1024, "Port is incorrect!!");
  
  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(port);
  tcpAddr.sin_addr.s_addr = INADDR_ANY;

  udpAddr.sin_family = AF_INET;
  udpAddr.sin_port = htons(port);
  udpAddr.sin_addr.s_addr = INADDR_ANY;

  ret = bind((*tcpSocket), (struct sockaddr *) &tcpAddr, sizeof(struct sockaddr_in));
    // DIE(ret < 0, "TCP bind counldn't work!!");

  ret = bind((*udpSocket), (struct sockaddr *) &udpAddr, sizeof(struct sockaddr_in));
    // DIE(ret < 0, "UDP bind counldn't work!!");

  listFD[0].fd = STDIN_FILENO;
  listFD[0].events = POLLIN;

  listFD[1].fd = (*tcpSocket);
  listFD[1].events = POLLIN;

  listFD[2].fd = (*udpSocket);
  listFD[2].events = POLLIN;
  
  int flag = 1;
  int result = setsockopt((*tcpSocket), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
}

char* uint8ToString(uint8_t number) {
    // Determine the maximum number of digits required
    int maxDigits = snprintf(NULL, 0, "%hhu", number) + 1;
    
    // Allocate memory for the string representation
    char* str = (char*) malloc(maxDigits * sizeof(char));
    
    // Convert the number to a string
    snprintf(str, maxDigits, "%hhu", number);
    
    return str;
}

// In aceasta functie convertesc fiecare mesaj din Udp in Tcp si ii formatez contentul in functie de type
void convertUdpToTcp(struct udp_message *udpMessage,struct tcp_message *tcpMessage){
  
  memset(tcpMessage->topic,0,51);
  memcpy(tcpMessage->topic,udpMessage->topic,strlen(udpMessage->topic));

  if(udpMessage->type == 0){
    strcpy(tcpMessage->type,"INT");
    int nrConverted = 0, signBit = 0, nr = 0;

    memcpy(&signBit, udpMessage->content, 1);
    memcpy(&nrConverted, udpMessage->content + 1, 4);

    if(signBit == 1)
      nr = -ntohl(nrConverted);
    
    if(signBit == 0)
      nr = ntohl(nrConverted);

    sprintf(tcpMessage->content,"%d",nr);
  }

  if(udpMessage->type == 1){
    strcpy(tcpMessage->type,"SHORT_REAL");
    double nrConverted = 0;
    nrConverted = ntohs(*(uint16_t*)(udpMessage->content));
    sprintf(tcpMessage->content,"%.2f", nrConverted/100);
  }

  if(udpMessage->type == 2){
    strcpy(tcpMessage->type,"FLOAT");
    unsigned char signBit;
    memcpy(&signBit,udpMessage->content,1);
    float nrConverted = 0;

    nrConverted = ntohl(*(uint32_t*)(udpMessage->content + 1));
    nrConverted /= pow(10,udpMessage->content[5]);

    if(signBit == 1)
      nrConverted = -nrConverted;

    memset(tcpMessage->content,0,100);
    sprintf(tcpMessage->content,"%.*f",udpMessage->content[5],nrConverted);
  }

  if(udpMessage->type == 3){
    strcpy(tcpMessage->type,"STRING");
    memcpy(tcpMessage->content,udpMessage->content,strlen(udpMessage->content));
  }
}

int searchFollowTopic(struct udp_message udpMessage,struct subscriber subscriber){

  for(int i=0;i<subscriber.nrTopic;i++){
    if(!strcmp(subscriber.topic[i].name,udpMessage.topic)){
      return i;
    }
  }
  return -1;

}

void forwardUdpMessage(struct udp_message udpMessage){
  int ret;

// Iterez prin vectorul de Subcribers si verifi pentru fiecare daca urmareste topicul.
  for(int index = 0; index < nrSubscribers; index++){
    int indexTopic = searchFollowTopic(udpMessage, subscribers[index]);

    if(indexTopic > -1){
      
//      In cazul in care este conectat convertesc mesajul Udp intr-un Tcp si il trimit direct la subscriber
      if(subscribers[index].connected == 1){
        struct tcp_message tcpMessage;
        memset(&tcpMessage,0,sizeof(struct tcp_message));

        memcpy(tcpMessage.ip, inet_ntoa(subscribers[index].subscriberAddr.sin_addr),16);
        tcpMessage.port = subscribers[index].subscriberAddr.sin_port;

        convertUdpToTcp(&udpMessage,&tcpMessage);

        ret = send(subscribers[index].socket, &tcpMessage, sizeof(struct tcp_message), 0);
        // DIE(ret < 0, "Send ID to server didn't work!!");
      }

//      In cazul in care nu este conectat, dar totusi are functia de 'sf' ii convertesc mesajul Udp in Tcp si il salvez pentru a il trimite la conectare
      if(subscribers[index].connected == 0 && subscribers[index].topic[indexTopic].sf == 1){
        struct tcp_message tcpMessage;
        convertUdpToTcp(&udpMessage,&tcpMessage);

        int indexStore = subscribers[index].topic[indexTopic].lenTcpMessages++; 
        memcpy(subscribers[index].topic[indexTopic].storeTcpMessages[indexStore],&tcpMessage,sizeof(struct tcp_message));
      }

    }
  }

}

int searchSubscriber(char id[]){

  for(int index = 0; index < nrSubscribers; index++){
    if(!strcmp(id,subscribers[index].id)){
      return index;
    }
  }
  return -1;

}

void initSubscribersList(){

  memset(subscribers,0,sizeof(struct subscriber)*MAX_SUBSCRIBERS);
  for(int indexSubs = 0; indexSubs < MAX_SUBSCRIBERS; indexSubs++){
    for(int indexTopic = 0; indexTopic < 16; indexTopic++){
      for(int i=0; i < MAX_TCP_MESSAGE_STORE; i++){
        subscribers[indexSubs].topic[indexTopic].storeTcpMessages[i] = calloc(1, sizeof(struct tcp_message));
      }
    }
  }

}

int main(int argc, char *argv[]){

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  int ret;
  int tcpSocket, udpSocket, port, nrFD=0;
  struct sockaddr_in tcpAddr, udpAddr, subscriberAddr;
  socklen_t lenTcpAddr = sizeof(subscriberAddr);
  socklen_t lenUdpAddr = sizeof(subscriberAddr);
  struct pollfd listFD[1000];
  char buffer[BUFFSIZE]; 

// Initializez vectorul de Subscribers
  initSubscribersList();

// Initializez socket-ul de Tcp la care se vor conecta clientii si socket-ul de Udp de la care voi primi topicuri
  port = atoi(argv[1]);
  initServer(&tcpSocket,&udpSocket,tcpAddr,udpAddr,port,listFD);
  nrFD = 3;

  ret = listen(tcpSocket, 5);
  // DIE(ret < 0,"TCP listen couldn't work!!!");

  while(1){

    memset(buffer,0,BUFFSIZE);

    poll(listFD,nrFD,-1);

//    Ascult comenzile de la tastatura
    if((listFD[0].revents & POLLIN) != 0){ 
      fgets(buffer,BUFFSIZE,stdin);
      if(!strncmp(buffer,"exit",4)){
        exit(0);
      }else{
        printf("Command didn't find\n");
      }
    }

    // TCP connection
    if((listFD[1].revents & POLLIN) != 0){

      int newTcpSocket = accept(tcpSocket,(struct sockaddr *) &subscriberAddr, &lenTcpAddr);
      // DIE(newTcpSocket < 0, "New TCP socket couldn't be create");

      int flag = 1;
      int result = setsockopt(newTcpSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

      listFD[nrFD].fd = newTcpSocket;
      listFD[nrFD].events = POLLIN;
      nrFD++;

      ret = recv(newTcpSocket,buffer,10,0);
      // DIE(ret < 0, "Receive ID couldn't work");

      char idSubscriber[10];
      memcpy(idSubscriber,buffer,strlen(buffer));
      
//      Funtia searchSubcribe returnreaza index-ul in cazul in care a fost gasit id 
      int indexSubscriber = searchSubscriber(idSubscriber);

      if(indexSubscriber < 0){
        addSubscriber(idSubscriber,newTcpSocket, subscriberAddr, lenTcpAddr);
        printf("New client %s connected from %s:%d.\n", idSubscriber, inet_ntoa(subscriberAddr.sin_addr), ntohs(subscriberAddr.sin_port));
      }

      if(indexSubscriber >= 0 && subscribers[indexSubscriber].connected == 1){
        printf("Client %s already connected.\n",idSubscriber);
        close(newTcpSocket);
      }
      
      if(indexSubscriber >= 0 && subscribers[indexSubscriber].connected == 0){
        printf("New client %s connected from %s:%d.\n", idSubscriber, inet_ntoa(subscriberAddr.sin_addr), ntohs(subscriberAddr.sin_port));
        reconnectSubscriber(indexSubscriber, newTcpSocket, subscriberAddr, lenTcpAddr);
      }
      
    }

    // UDP Connection
    if((listFD[2].revents & POLLIN) != 0){
      struct udp_message udpMessage;
      memset(&udpMessage,0,sizeof(struct udp_message));

      ret = recvfrom(listFD[2].fd,&udpMessage,sizeof(struct udp_message),0,(struct sockaddr *) &udpAddr,&lenUdpAddr);
      // DIE(ret < 0, "New topic couldn't be receeive!!");

      forwardUdpMessage(udpMessage);   
    }

    // Subscribes Commands
    for(int indexFD = 3; indexFD < nrFD; indexFD++){
      if((listFD[indexFD].revents & POLLIN) != 0){

        struct server_message serverMessage;
        memset(&serverMessage,0,sizeof(struct server_message));

        ret = recv(listFD[indexFD].fd,&serverMessage,sizeof(struct server_message),0);
        // DIE(ret < 0, "Subscriber command couldn't me receive!!");

//        In cazul in care am dat 'receive' la un mesaj si am primit 0 bits este un semn al deconeectarii clientului de pe socket-ul respectiv
        if(ret == 0){

          for(int index = 0; index < nrSubscribers; index++){
            if(listFD[indexFD].fd == subscribers[index].socket){
              subscribers[index].connected = 0;
              printf("Client %s disconnected.\n",subscribers[index].id);
            }
            
          }

//          Inchid socket-ul si il scot din vectorul de FD de pe care ascultam
          close(listFD[indexFD].fd);
          for(int i = indexFD; i < nrFD; i++)
            memcpy(&listFD[i],&listFD[i+1],sizeof(struct pollfd ));
          nrFD--;

        }

        int indexSubscriber = searchSubscriber(serverMessage.id);

//        Adaug topicul in lista de topicuri ascultate
        if(serverMessage.command == 1){ //subscribe

          int indexTopic = subscribers[indexSubscriber].nrTopic;
          memcpy(subscribers[indexSubscriber].topic[indexTopic].name,serverMessage.topic,strlen(serverMessage.topic));
          subscribers[indexSubscriber].topic[indexTopic].sf = serverMessage.sf;
          subscribers[indexSubscriber].nrTopic++;

        }

//        Sterg tipicul din lista de topicuri ascultate
        if(serverMessage.command == 0){ //unsubscribe

          int i;
          for(;i<subscribers[indexSubscriber].nrTopic;i++)
            memcpy(&subscribers[indexSubscriber].topic[i],&subscribers[indexSubscriber].topic[i+1],sizeof(struct topic));
          subscribers[indexSubscriber].nrTopic--;

        }

      }
    }

  }

  return 0;
}