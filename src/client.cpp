#include <iostream>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // htons
#include <string.h>

#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"

using namespace std;

struct recv
{
    int ds_client;
    char msg[MAX_LEN_BUFFER_JSON];
    int size_msg;
};

int sendTCP(int socket, const char *buffer, size_t length)
{
    int sent = 0;
    int total = 0;
    while (total < (int)length)
    {
        sent = send(socket, buffer + total, length - total, 0);
        if (sent <= 0)
            return sent;
        total += sent;
    }
    return total;
}

int recvTCP(int socket, char *buffer, size_t length)
{
    int received = 0, total = 0;
    do
    {
        received = recv(socket, buffer + total, length - total, 0);
        if (received <= 0)
            return received;
        total += received;
    } while (buffer[total - 1] != '\0');
    return total;
}

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
        
        int rcv = recvTCP(r->ds_client, r->msg, sizeof(r->msg));
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
            cout << r->msg << endl;
            cloud = decode_cloud(r->msg,MAX_LEN_BUFFER_JSON);
            print_cloud(cloud);
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

    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds == -1)
    {
        perror("Error socket:"),
            exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_address, &addr.sin_addr.s_addr) == -1)
    {
        perror("Error converting ip_address:");
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons((short)atoi(port));

    if (connect(ds, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("Error connecting server:");
        exit(EXIT_FAILURE);
    }

 
    /* semop(semid, &verrouP, 1);
    cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
    reservation = init_reservation(cloud, client);
    semop(semid, &verrouV, 1);
    print_cloud(cloud);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, wait_thread, NULL) != 0) {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    } */
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

        if (sendTCP(ds, in_buffer, sizeof(in_buffer)) == -1)
        {
            perror("Can't send the message:");
            break;
        }
        
        in_buffer[0] = '\0';
        int rcv = recvTCP(ds, in_buffer, sizeof(in_buffer));
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
            in_buffer[rcv] = '\0';
            printf("< %s\n", in_buffer);
        }

    }
        if (pthread_cancel(thread_id) != 0)
            cerr << "Impossible de terminer le thread" << endl;

        if (pthread_join(thread_id, NULL) != 0)
            cout << "impossible de joiner" << endl;

    return 0;
}
