#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>

using namespace std;

int NAnumber;
string fileName;

void thread_read(int SocketFD) 
{
    do{
        std::cout<<"NA waiting for NP"<<std::endl;
        int nbytes;
        char buf[4];
        std::string line="";
        std::string line2="";
        int idFirst;
        nbytes = read(SocketFD,buf,4);
        buf[4]='\0';
        line+=buf;
        sscanf(buf,"%d",&idFirst);
        nbytes=read(SocketFD,buf,1);
        buf[1]='\0';
        line+=buf;
        int idAction;
        sscanf(buf,"%d",&idAction);
        int Fbytes,Dbytes;
        char* Field,*Data;
    
        if(buf[0]=='0'){
            nbytes=read(SocketFD,buf,4);
            buf[4]='\0';
            sscanf(buf,"%d",&Fbytes);
            line += buf;
            Field=new char[Fbytes];
            nbytes=read(SocketFD,Field,Fbytes);
            line += Field;
            line2+=Field;
            nbytes=read(SocketFD,buf,4);
            buf[4]='\0';
            sscanf(buf,"%d",&Dbytes);
            line += buf;
            Data=new char[Dbytes];
            nbytes=read(SocketFD,Data,Dbytes);
            line += Data;
            line2+=" ";
            line2+=Data;

            std::cout<<"Escribiendo: "<<line2<<std::endl;

            ofstream file2(fileName);
            file2<<line2<<endl; 
        }
        else if (buf[0]=='2'){
            read(SocketFD, buf, 1);
            buf[1]='\0';
            NAnumber=int(buf[0]-48)-4;
            fileName = "file" + to_string(NAnumber) + ".txt";
            ofstream file(fileName);
            std::cout<<"SOY NA numero: "<<NAnumber<<std::endl;
        }
    } while(true);

}
 

int main()
{
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(AF_INET, SOCK_STREAM, 0); 
    int n;
    char buffer[256];

    if (-1 == SocketFD) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(9034);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);
    //Res = inet_pton(AF_INET, "172.17.1.58", &stSockAddr.sin_addr);

    if (0 > Res) {
        perror("error: first parameter is not a valid address family");
        close(SocketFD);
        exit(EXIT_FAILURE);
    } else if (0 == Res) {
        perror("char string (second parameter does not contain valid ipaddress");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    int socketClient = connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
    // Here get connect ID, then pass it to the thread
    if (-1 == socketClient) {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    write(SocketFD,"0000",4);
    write(SocketFD,"2",1);

    std::cout<<"NA iniciado"<<std::endl;;
    
    for(;;) {
            thread_read(SocketFD);
        }

    shutdown(SocketFD, SHUT_RDWR);

    close(SocketFD);
    return 0;
}
