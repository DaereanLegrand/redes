/*
** selectserver.c -- a cheezy multiperson chat server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>


#define PORT "9034" // port we're listening on
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

std::map<int,std::queue<std::string>> queues;

void write_ (int socket){
    for(;;){
        if(queues[socket].empty()) continue;
        std::string buff = queues[socket].front();
        std::cout << "popped: " << buff << " from queue: " << socket << std::endl;
        queues[socket].pop();
        // elegimos el hash para a cual NA enviar y va a la cola de ese(?)
        int NAIndex = std::hash<std::string>{}(buff) % 4 + 4;
        std::cout<<"Enviando a NA: "<<NAIndex-4<<std::endl;
        write(NAIndex, buff.c_str(), buff.size()); 
    }
}

int id_counter=0;
std::vector<std::thread> thrds;

int main(void)
{
  fd_set master;                      // master file descriptor list
  fd_set read_fds;                    // temp file descriptor list for select()
  int fdmax;                          // maximum file descriptor number
  int listener;                       // listening socket descriptor
  int newfd;                          // newly accept()ed socket descriptor
  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;
  char buf[4]; // buffer for client data
  int nbytes;
  char remoteIP[INET6_ADDRSTRLEN];
  int yes = 1; // for setsockopt() SO_REUSEADDR, below
  int i, j, rv;
  struct addrinfo hints, *ai, *p;
  FD_ZERO(&master); // clear the master and temp sets
  FD_ZERO(&read_fds);
  // get us a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
  {
    fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
    exit(1);
  }
  
  for (p = ai; p != NULL; p = p->ai_next)
  {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0)
    {
      continue;
    }
    // lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
    {
      close(listener);
      continue;
    }
    break;
  }
  // if we got here, it means we didn't get bound
  if (p == NULL)
  {
    fprintf(stderr, "selectserver: failed to bind\n");
    exit(2);
  }
  freeaddrinfo(ai); // all done with this
  // listen
  if (listen(listener, 10) == -1)
  {
    perror("listen");
    exit(3);
  }
  // add the listener to the master set
  FD_SET(listener, &master);
  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one
 
  std::vector<std::thread> thrds;

  // main loop

  for (;;)
  {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select");
      exit(4);
    }
    // run through the existing connections looking for data to read
    for (i = 0; i <= fdmax; i++)
    {
      if (FD_ISSET(i, &read_fds))
      { // we got one!!
        if (i == listener)
        {
          // handle new connections
          addrlen = sizeof remoteaddr;
          newfd = accept(listener,
                         (struct sockaddr *)&remoteaddr,
                         &addrlen);
          if (newfd == -1)
          {
            perror("accept");
          }
          else
          {
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax)
            { // keep track of the max
              fdmax = newfd;
            }
            printf("selectserver: new connection from %s on "
                   "socket %d\n",
                   inet_ntop(remoteaddr.ss_family,
                             get_in_addr((struct sockaddr *)&remoteaddr),
                             remoteIP, INET6_ADDRSTRLEN),
                   newfd);

                   //thread aquí
            queues[newfd].push("");
            thrds.push_back(std::thread(write_,newfd));            
          }
        }
        else
        {
          // handle data from a client
          if ((nbytes = read(i,buf,4)) <= 0)
          {
            // got error or connection closed by client
            if (nbytes == 0)
            {
            }
            else
            {
              perror("recv");
            }
            close(i);           // bye!
            FD_CLR(i, &master); // remove from master set
          }
          else
          {
            // we got some data from a client
            std::string line="";
            int totalBytes=nbytes;
            int idFirst;
            buf[4]='\0';
            printf("buf: %s\n",buf);
            sscanf(buf,"%d",&idFirst);
            nbytes=read(i,buf,1);
            buf[1]='\0';
            int idAction;
            sscanf(buf,"%d",&idAction);
            int Fbytes,Dbytes;
            char* Field,*Data;

            std::cout<<"Accion: "<<idAction<<std::endl;

            switch (idAction)
            {
            case 0:
            //LEEMOS DE CLIENTE A NP
              std::cout<<"-c"<<std::endl;
              line+="0000";
              line+="0";
              nbytes=read(i,buf,4);
              buf[4]='\0';
              sscanf(buf,"%d",&Fbytes);
              std::cout<<"F: "<<Fbytes<<std::endl;
              line += buf;
              Field=new char[Fbytes];
              nbytes=read(i,Field,Fbytes);
              line += Field;
              printf("Field: %s\n",Field);
              nbytes=read(i,buf,4);
              buf[4]='\0';
              sscanf(buf,"%d",&Dbytes);
              std::cout<<"D: "<<Dbytes<<std::endl;
              line += buf;
              Data=new char[Dbytes];
              nbytes=read(i,Data,Dbytes);
              printf("Data: %s\n",Data);
              line += Data;

              
              queues[i].push(line);
            //ESCRIBIMOS DE CLIENTE A NA
              break;            
            case 1:
            //LEEMOS DE NA A NP
              line = "";
              std::cout<<"-cC"<<std::endl;
              nbytes=read(i,buf,4);
              buf[4]='\0';
              sscanf(buf,"%d",&Fbytes);
              std::cout<<"F: "<<Fbytes<<std::endl;
              line += buf;
              Field=new char[Fbytes];
              nbytes=read(i,Field,Fbytes);
              line += Field;
              printf("Field: %s\n",Field);
              nbytes=read(i,buf,4);
              buf[4]='\0';
              sscanf(buf,"%d",&Dbytes);
              std::cout<<"D: "<<Dbytes<<std::endl;
              line += buf;
              Data=new char[Dbytes];
              nbytes=read(i,Data,Dbytes);
              printf("Data: %s\n",Data);
              line += Data;

              //queues[i].push(line);
              break;

            case 2:
              std::cout<<"EN case 2: "<<std::endl;
              std::string NAnumber=std::to_string(i);
              write(i,"0000",4);
              write(i,"2",1);
              write(i,NAnumber.c_str(),1);
              break;
            }
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for(;;)--and you thought it would never end!
  return 0;
}