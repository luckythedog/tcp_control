#define _GNU_SOURCE
#include <stdio.h>
#include <iostream>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <time.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>

#include <sstream>
#include <fstream>

#include <poll.h>
#include "server_socket.h"

#define PORT 6666
#define BUFFSIZE 255
#define ROOM_SIZE 2
#define ROOM_TIMEOUT 10
#define ROOM_ROUND_TIME 90
#define DATABASE_NAME "drawdown_db"


using namespace std;
server master_socket(PORT);
bool server_online = false;

struct client_file{
    int socket;
    ofstream *data;
};
vector<client_file> files;

void* connectionThread(void*) {
        vector<pollfd> poll_sockets;
        pollfd* poll_sockets_ptr;
        int poll_activity;
        pollfd add_socket;
        add_socket.fd = master_socket.fd;
        add_socket.events = POLLIN;
        poll_sockets.push_back(add_socket);
        while (server_online) {
                poll_sockets_ptr = &poll_sockets[0];
                poll_activity = poll(poll_sockets_ptr, poll_sockets.size(), -1);
                if (poll_activity < 0) {
                        perror("poll");
                }
                if (poll_sockets[0].revents & POLLIN) {
                        int new_socket;
                        struct sockaddr_in client_address;
                        int client_addrlen = sizeof(client_address);

                        if ((new_socket = accept(master_socket.fd,
                                        (struct sockaddr*) &client_address,
                                        (socklen_t*) &client_addrlen)) < 0) {
                                perror("accept");
                        } else {
                                pollfd add_socket;
                                add_socket.fd = new_socket;
                                add_socket.events = POLLIN;
                                poll_sockets.push_back(add_socket);
                                cout << "Client " << new_socket << " has connected! ("
                                                << inet_ntoa(client_address.sin_addr) << ":"
                                                << ntohs(client_address.sin_port) << ")" << endl;
                                char* send_message = "SUCCESSFUL_CONNECTION";
                                if (send(new_socket, send_message, strlen(send_message), 0)
                                                != strlen(send_message)) {
                                        perror("send");
                                }

                        }

                }
                for (int i = 0; i < poll_sockets.size(); i++) {
                        if (i != 0) {
                                int s = poll_sockets[i].fd;
                                if (poll_sockets[i].revents & POLLIN) {
                                        char buffer[BUFFSIZE];
                                        if ((read(poll_sockets[i].fd, buffer, BUFFSIZE)) != 0) {
                                                bool is_file_already_there = false;
                                                int file_index;
                                                   for(int b=0; b<files.size(); b++){
                                                       if(files[b].socket == poll_sockets[i].fd){
                                                           is_file_already_there = true;
                                                           file_index = b;
                                                       }
                                                   }
                                               if(memcmp(buffer, "FILE", 4) == 0){


                                                   if(!is_file_already_there){
                                                       client_file new_file;
                                                       new_file.socket = poll_sockets[i].fd;
                                                       new_file.data = new ofstream("test_file.png");
                                                       char* send_message = "FILE_START";
                                                   send(poll_sockets[i].fd, send_message, strlen(send_message),0);
                                                        files.push_back(new_file);
                                                   }else{
                                                       char* send_message = "FILE_IN_PROGRESS";
                                                   send(poll_sockets[i].fd, send_message, strlen(send_message),0);
                                                   cout << "You already have a file in progress. Please send done to finish it. Socket " << poll_sockets[i].fd << endl;
                                                   }

                                               }else if (memcmp(buffer, "DONE", 4) == 0){
                                                   if(is_file_already_there){
                                                   char* send_message = "FILE_COMPLETE";
                                                   send(poll_sockets[i].fd, send_message, strlen(send_message),0);
                                                   cout << "File completed" << endl;
                                                   files[file_index].data->close();

                                                    files.erase(files.begin()+file_index);
                                               }else{
                                                 char* send_message = "NO_FILE";
                                                   send(poll_sockets[i].fd, send_message, strlen(send_message),0);
                                               }
                                               }else{
                                                   cout << "Receiving data buffer" << endl;
                                                   if(is_file_already_there){
                                                        files[file_index].data->write(buffer, BUFFSIZE);
                                                   }else{
                                                 char* send_message = "NO_FILE_STOP_SENDING_DATA";
                                                   send(poll_sockets[i].fd, send_message, strlen(send_message),0);
                                                   }
                                               }
                                        } else {
                                                for(int b=0; b<files.size(); b++){
                                                    if(files[b].socket == poll_sockets[i].fd){
                                                    files.erase(files.begin()+b);
                                                    }
                                                }
                                                cout << "Client " << s << " has disconnected!" << endl;
                                                poll_sockets.erase(poll_sockets.begin() + i);
                                                close(s);
                                        }
                                }
                        }
                }

        }
        return 0;
}

bool close_server() {

        close(master_socket.fd);
        cout << "Server is now offline!" << endl;
        return true;
}

int main(int argc, char *argv[]) {
        if (!master_socket.start()) {
                perror("start");
                exit(EXIT_FAILURE);
        }
        // if(!loadDrawingData()){
        //   exit(EXIT_FAILURE);

        //}
        pthread_t t1, t2, t3, t4, t5;
        cout << "Server is now online!" << endl;
        server_online = true;

        pthread_create(&t1, NULL, connectionThread, NULL);
        pthread_join(t1, NULL);

        if (close_server()) {
                return 0;
        }
}
