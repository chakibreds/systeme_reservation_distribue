#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"
#include "../inc/protocol.hpp"

using namespace std;

int ds_client;

//------ Variables globales de clients
Cloud *cloud;
void *listen_modif(void *params)
{

    struct recv_msg *r = (struct recv_msg *)params;
    int s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (s != 0)
        cerr << "Can't set cancel state" << endl;
    s = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1)
    {
        int rcv = recvTCP(r->ds_client, r->msg, strlen(r->msg));
        if (rcv == -1)
        {
            perror("Error recv:");
            break;
        }
        else if (rcv == 0)
        {
            printf("server Deconnected\n");
            kill(getpid(), SIGINT);
            break;
        }
        else
        {
            r->msg[rcv] = '\0';
            if (r->msg != NULL && strlen(r->msg) > 0 && r->msg[0] == '[' && r->msg[strlen(r->msg)-1] == ']') {
                system("clear");
                cloud = decode_cloud(r->msg,MAX_LEN_BUFFER_JSON);
                print_cloud(cloud);

            } else if (r->msg != NULL) {
                cout << "< " << r->msg << endl;
            } else {
                cerr << "R->msg == NULL" << endl;
            }
        }
    }
    return NULL;
}


void server_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGQUIT) {
        cout << "SIGINT received" << endl;
        commande cmd;
        cmd.cmd_type = CMD_EXIT;
        if (sendCommandeTCP(ds_client, &cmd) == -1) {
            perror("Can't send message to server");
        }
        close(ds_client);
        cout << "Ending client" << endl;
        exit(0);
    }
    return;
}