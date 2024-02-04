#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include <netdb.h>

#define BUFFSIZE 1551

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

void initSubscriber(int *serverSocket,struct sockaddr_in *serverAddr,char *argv[],struct pollfd listFD[], char id[]){
  int ret;

  (*serverSocket) = socket(AF_INET, SOCK_STREAM, 0);
  // DIE((*serverSocket) < 0, "TCP socket counldn't be create!!");

  // DIE(port < 1024, "Port is incorrect!!");
  
  (*serverAddr).sin_family = AF_INET;
  (*serverAddr).sin_port = htons(atoi(argv[3]));
  (*serverAddr).sin_addr.s_addr = inet_addr(argv[2]);
  // DIE(ret == 0, "IP address couldn't be fill!!!");

  listFD[0].fd = STDIN_FILENO;
  listFD[0].events = POLLIN;

  listFD[1].fd = (*serverSocket);
  listFD[1].events = POLLIN;
  
  int flag = 1;
  int result = setsockopt((*serverSocket), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

  memcpy(id,argv[1],strlen(argv[1]));

}

struct server_message buildServerMessage(char buffer[],char id[], int command){

  struct server_message serverMessage;
  memset(&serverMessage,0,sizeof(struct server_message));
  char *word = strtok(buffer," \n");
  serverMessage.command = command;
  word = strtok(NULL," \n");
  memcpy(serverMessage.topic,word,strlen(word));
  word = strtok(NULL," \n");
  if(word != NULL)
    serverMessage.sf = atoi(word);

  memcpy(serverMessage.id,id,strlen(id));

  return serverMessage;
}

int main(int argc, char *argv[]){

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  int ret;
  int serverSocket, port, nrFD=0;
  struct sockaddr_in serverAddr;
  struct pollfd listFD[2];
  char buffer[BUFFSIZE];
  char id[10];

// Initializez socket-ul si introdus stdin si socket-ul de Tcp in poll
  initSubscriber(&serverSocket,&serverAddr,argv,listFD,id);

  nrFD = 2;
  // DIE(ret == 0, "IP address couldn't be fill!!!");
  ret = connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
  // DIE(ret < 0, "Conection to server didn't work!!");
  ret = send(serverSocket, argv[1], strlen(argv[1]), 0);
  // DIE(ret < 0, "Send ID to server didn't work!!");

  while(1){

    memset(buffer,0,BUFFSIZE);
    poll(listFD,nrFD,-1);

    // Stdin Command
    if((listFD[0].revents & POLLIN) != 0){ 
      int check = 0;
      fgets(buffer,BUFFSIZE,stdin);

      if(!strncmp(buffer,"exit",4)){
        ret = shutdown(serverSocket,SHUT_RDWR);
        // DIE(ret < 0, "Couldn't stop tcp client!!");
        exit(0);
        // DIE(ret < 0, "Send exit to server couldn't work!!");
        close(serverSocket);
        return 0;
      }

//     In cazul in care am primit comanda 'subscribe' construect mesajul pentru server si il trimit
      if(!strncmp(buffer,"subscribe",9)){
        struct server_message serverMessage;
        serverMessage = buildServerMessage(buffer,id,1);
        ret = send(serverSocket, (char *) &serverMessage, sizeof(struct server_message),0);
        // DIE(ret < 0, "Server message couldn't be send");

        printf("Subscribed to topic.\n");
        check = 1;
      }

//     In cazul in care am primit comanda 'unsubscribe' construesc mesajul pentru server si il trimit
      if(!strncmp(buffer,"unsubscribe",11)){
        struct server_message serverMessage;
        serverMessage = buildServerMessage(buffer,id,0);
        ret = send(serverSocket, (char *) &serverMessage, sizeof(struct server_message),0);
        // DIE(ret < 0, "Server message couldn't be send");

        printf("Unsubscribed from topic.\n");
        check = 1;
      }

      if(check == 0)
        printf("Command didn't find\n");

    }

    // TCP connection
    if((listFD[1].revents & POLLIN) != 0){
      ret = recv(serverSocket, buffer, sizeof(struct tcp_message), 0);
      // DIE(ret < 0, "Couldn't receive server message!!");

      struct tcp_message receiveMessage;
      memset(&receiveMessage,0,sizeof(struct tcp_message));
      memcpy(&receiveMessage, buffer, sizeof(struct tcp_message));

//      In cazul in care am primit 'receive' de la server si am contorizat 0 bits o sa inchid sesiunea, semn ca server-ul s-a deconenctat
      if(ret == 0){
        ret = shutdown(serverSocket,SHUT_RDWR);
        // DIE(ret < 0, "Couldn't stop tcp client!!");

        exit(0);
      }else{
//        Capturez mesajul de tip Tcp de la server si il afisez. Mesajul vine parsat in formatul corespunzator        
        printf("%s:%hu - %s - %s - %s\n", receiveMessage.ip, ntohs(receiveMessage.port), receiveMessage.topic, receiveMessage.type, receiveMessage.content);

      }
    }
  }

  close(serverSocket);
  return 0;
}