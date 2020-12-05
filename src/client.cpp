#include <iostream>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"
#include "../inc/protocol.hpp"

using namespace std;

//------ Variables globales de clients
Cloud *cloud;
Reservation *reservation;
void *listen_modif(void *params)
{

    struct recv *r = (struct recv *)params;
    int s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (s != 0)
        cerr << "Can't set cancel state" << endl;
    s = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1)
    {
        cout<< "Thread: attente" <<endl;
        int rcv = recvTCP(r->ds_client, r->msg, strlen(r->msg));
        cout<< "Thread: rcv" <<endl;
        if (rcv == -1)
        {
            perror("Error recv:");
            break;
        }
        else if (rcv == 0)
        {
            printf("< Message vide!\n");
            break;
        }
        else
        {
            if (r->msg != NULL && strlen(r->msg) > 0 && r->msg[0] == '[' && r->msg[strlen(r->msg)-1] == ']') {
                cout << "Thread: Msg recu '" << r->msg << "'" << endl;
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
/* 
    Ce programme se connecte au serveur puis 
        - le thread principal attend une demande saisi au clavier par l'utilisateur et l'envoi au serveur.
        - Un thread secondaire qui reste en écoute d'eventuels changement d'états des ressources.
 */
int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ip_address port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_address = argv[1], *port = argv[2];

    Client client;
    client.id = 1;
    client.port = atoi(port);
    strcpy(client.ip_address, ip_address);
    cout << "entrez votre nom svp : ";
    fgets(client.name, MAX_LEN_NAME_CLIENT, stdin);
    client.name[strlen(client.name) - 1] = '\0';
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;

    int ds = init_socket_client(ip_address, port);

    fflush(stdin);
    pthread_t thread_id;
    struct recv r;
    r.ds_client = ds;
    if (pthread_create(&thread_id, NULL, listen_modif, (void *)&r) != 0)
    {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    }
    while (1)
    {
        cout << endl
        << "> ";
        char in_buffer[500];
        in_buffer[499] = '\0';
        fgets(in_buffer, 500, stdin);
        in_buffer[strlen(in_buffer) - 1] = '\0';
    
        commande cmd = interpret_cmd(in_buffer);
        
        if (execute_cmd_client(cmd) == -1) {
            cerr << "Erreur à l'éxecution de la commande" << endl;
            continue;
        }
        
        if (cmd.cmd_type == CMD_HELP) continue;

        if (sendCommandeTCP(ds, &cmd) == -1) {perror("Can't send the message:");break;}

        if (cmd.cmd_type == CMD_EXIT) break;
        
        in_buffer[0] = '\0';
    }
    if (pthread_cancel(thread_id) != 0)
        cerr << "Impossible de terminer le thread" << endl;

    if (pthread_join(thread_id, NULL) != 0)
        cerr << "impossible de joindre" << endl;

    return 0;
}
