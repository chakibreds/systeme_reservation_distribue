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
int semid;
int shmid;
char *cloud_json;
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
            cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
            print_cloud(cloud);
        }
    }
    return NULL;
}
/* struct sembuf verrouP = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};

struct sembuf verrouV = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};

struct sembuf signalP = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};

struct sembuf signalV = {.sem_num = 1, .sem_op = 1, .sem_flg = 0};

struct sembuf signalZ = {.sem_num = 1, .sem_op = 0, .sem_flg = 0};


int execute_cmd(commande cmd, Reservation* reservation) {
    if (cmd.cmd_type == CMD_ALLOC_WITH_NAMES) {
        semop(semid, &verrouP, 1);
        destroy_cloud(cloud);
        cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
        while (check_commande(cloud, cmd) == -1) {
            cout << "mise en attente" << endl;
            semop(semid, &verrouV, 1);
            semop(semid, &signalZ, 1);
            semop(semid, &verrouP, 1);
            destroy_cloud(cloud);
            cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
        }
        
        for (int i = 0; i < cmd.nb_server; i++) {
            if (reserve_resources(cloud, reservation, cmd.server_name[i], {.cpu = cmd.cpu[i], .memory = cmd.memory[i]}) == -1) {
                cerr << "Impossible d'allouer " << cmd.server_name[i] << endl;
            }
        }
        // envoi du signal de modification de cloud
        code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
        semop(semid, &signalP, 1);
        // wait ??
        semop(semid, &signalV, 1);
        semop(semid, &verrouV, 1);
        return 0;
    } else if (cmd.cmd_type == CMD_ALLOC_ALL) {
        cout << "CMD_ALLOC_ALL" << endl;
        cout << "non implémenté" << endl;
        return 0;
    } else if (cmd.cmd_type == CMD_FREE_WITH_NAMES) {
        semop(semid, &verrouP, 1);
        for (int i = 0; i < cmd.nb_server; i++) {
            free_allocation(cloud, reservation, cmd.server_name[i]);
        }
        code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
        // envoi du signal de modification de cloud
        semop(semid, &signalP, 1);
        semop(semid, &signalV, 1);
        semop(semid, &verrouV, 1);
        return 0;
    } else if (cmd.cmd_type == CMD_EXIT || cmd.cmd_type == CMD_FREE_ALL){
        semop(semid, &verrouP, 1);
        if (free_all_allocation(cloud,reservation) == -1) {
            cerr << "Free impossible" << endl;
        } else {
            if (code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON) == -1) {
                    cerr << "impossible de coder le cloud en json" << endl;
            }
             semop(semid, &signalP, 1);
            semop(semid, &signalV, 1);
        }
        semop(semid, &verrouV, 1);

        return 0;
    } else if (cmd.cmd_type == CMD_HELP){
        cout << "alloc cpu mem:                   Allouer'cpu' cpu et 'mem' memory sur n'importe quel serveur" << endl;
        cout << "alloc (server_name cpu mem)+...: Allouer 'cpu' cpu et 'mem' memory sur le serveur 'server_name'" << endl;
        cout << "free (server_name)*:             Libérer tous ce qui a été alloué sur les serveurs spécifiés" << endl;
        cout << "free:                            Libérer toutes les ressources reserver." << endl;
        cout << "exit:                            Quitter proprement" << endl;
        return 0;
    } else {
        return -1;
    }
}


/* premier thread attends une modification pour afficher 
void* wait_thread(void* params) {
    int s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (s != 0)
        cerr << "Can't set cancel state" << endl;
    s = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while(1) {
        semop(semid,&signalZ,1);
        semop(semid, &verrouP, 1);
        
        destroy_cloud(cloud);
        cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
        
        system("clear");
        print_cloud(cloud);
        print_reservation(reservation);
        cout << "> ";
        semop(semid, &verrouV, 1);
    }
}*/

/* 
    Ce programme se connecte au serveur puis 
        - le thread principal attend une demande saisi au clavier par l'utilisateur et l'envoi au serveur.
        - Un thread secondaire qui reste en écoute d'eventuels changement d'états des ressources.
 */
int main(int argc, char const *argv[])
{

    /*  if (argc != 3) {
        cerr << "Usage: "<< argv[0] <<" chemin_vers_fichier_ipc id" << endl;
        exit(1);
    }

    key_t cle = ftok(argv[1], 'a');
    if (cle == -1) {
        perror("erreur ftok");
        exit(1);
    }
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, 0)) == -1) {
        perror("erreur shmget");
        exit(1);
    }
    if ((semid = semget(cle, 2, 0)) == -1) {
        perror("erreur semget");
        exit(1);
    }
        
    cloud_json = (char *)shmat(shmid, NULL, 0);
    if (cloud_json == (void*)-1) {
        perror("erreur shmat");
        exit(1);
    } */

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ip_address port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *ip_address = argv[1], *port = argv[2];

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

    Client client;
    client.id = 1;
    client.port = atoi(port);
    strcpy(client.ip_address, ip_address);
    cout << "entrez votre nom svp : ";
    fgets(client.name, MAX_LEN_NAME_CLIENT, stdin);
    client.name[strlen(client.name) - 1] = '\0';
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;

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
        char in_buffer[100];
        fgets(in_buffer, 100, stdin);
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

        /* commande cmd = interpret_cmd(in_buffer);
        
        if (execute_cmd(cmd, reservation) == -1)
            cerr << "Erreur à l'éxecution de la commande" << endl;
        if (cmd.cmd_type == CMD_EXIT)
            break;
    }
    
    //free all
     */
        if (pthread_cancel(thread_id) != 0)
            cerr << "Impossible de terminer le thread" << endl;

        if (pthread_join(thread_id, NULL) != 0)
            cout << "impossible de joiner" << endl;
    }

    return 0;
}
