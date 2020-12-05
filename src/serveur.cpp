#include <iostream>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // htons
#include <unistd.h>
#include <string.h>
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"
#include "../inc/protocol.hpp"

using namespace std;

//------ Variables globales du serveur
int semid;
int shmid;
char *cloud_json;
Cloud *cloud;
Reservation *reservation;
int id;


struct sembuf verrouP = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
struct sembuf verrouV = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
struct sembuf signalP = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};
struct sembuf signalV = {.sem_num = 1, .sem_op = 1, .sem_flg = 0};
struct sembuf signalZ = {.sem_num = 1, .sem_op = 0, .sem_flg = 0};

char *execute_cmd(commande* cmd, Reservation *reservation, char* res)
{
    strcpy(res, "");
    if (cmd->cmd_type == CMD_ALLOC_WITH_NAMES)
    {
        semop(semid, &verrouP, 1);
        destroy_cloud(cloud);
        cloud = decode_cloud(cloud_json, MAX_LEN_BUFFER_JSON);
        while (check_commande(cloud, cmd) == -1)
        {
            cout << "mise en attente" << endl;
            semop(semid, &verrouV, 1);
            semop(semid, &signalZ, 1);
            semop(semid, &verrouP, 1);
            destroy_cloud(cloud);
            cloud = decode_cloud(cloud_json, MAX_LEN_BUFFER_JSON);
        }
        for (int i = 0; i < cmd->nb_server; i++)
        {
            if (reserve_resources(cloud, reservation, cmd->server_name[i], {.cpu = cmd->cpu[i], .memory = cmd->memory[i]}) == -1)
            {
                cerr << "Impossible d'allouer " << cmd->server_name[i] << endl;
            }
        }
        // envoi du signal de modification de cloud
        code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
        semop(semid, &signalP, 1);
        // wait ??
        semop(semid, &signalV, 1);
        semop(semid, &verrouV, 1);
        strcpy(res, "commande d'allocation acceptée");
    }
    else if (cmd->cmd_type == CMD_ALLOC_ALL)
    {
        cout << "CMD_ALLOC_ALL" << endl;
        cout << "non implémenté" << endl;
        strcpy(res, "non implémenter");
    }
    else if (cmd->cmd_type == CMD_FREE_WITH_NAMES)
    {
        semop(semid, &verrouP, 1);
        for (int i = 0; i < cmd->nb_server; i++)
        {
            free_allocation(cloud, reservation, cmd->server_name[i]);
        }
        code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
        // envoi du signal de modification de cloud
        semop(semid, &signalP, 1);
        semop(semid, &signalV, 1);
        semop(semid, &verrouV, 1);
        strcpy(res, "commande de free acceptée");
    }
    else if (cmd->cmd_type == CMD_EXIT || cmd->cmd_type == CMD_FREE_ALL)
    {
        semop(semid, &verrouP, 1);
        if (free_all_allocation(cloud, reservation) == -1)
        {
            cerr << "Free impossible" << endl;
        }
        else
        {
            if (code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON) == -1)
            {
                cerr << "impossible de coder le cloud en json" << endl;
            }
            semop(semid, &signalP, 1);
            semop(semid, &signalV, 1);
        }
        semop(semid, &verrouV, 1);
        strcpy(res, "free all ok");
    }
    return res;
}

/* premier thread attends une modification pour afficher */
void *wait_thread(void *params)
{
    struct recv *r = (struct recv *)params;
    int s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (s != 0)
        cerr << "Can't set cancel state" << endl;
    s = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1)
    {

        semop(semid, &signalZ, 1);
        semop(semid, &verrouP, 1);
        cout << "Threads " << pthread_self() <<": " << cloud_json << endl;

        if (cloud_json != NULL && strlen(cloud_json) > 0 && cloud_json[0] == '[' && cloud_json[strlen(cloud_json)-1] == ']') {

            int size_str = code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
            if (size_str == -1)
            {
                cerr << "Impossible de coder le cloud" << endl;
                return NULL;
            }
            strcpy(r->msg, cloud_json);
            if (r != NULL && (sendTCP(r->ds_client, r->msg, strlen(r->msg) + 1) == -1))
            {
                perror("Can't send the message:");
                break;
            }
        } else {
            cerr << "Réception d'un cloud_json vide : " << (cloud_json==NULL?"null":"notnull") << endl;
        }
        semop(semid, &verrouV, 1);
    }
    return NULL;
}

int manage_user(int ds_client, int port)
{

    Client client;
    client.id = ++id;
    client.port = port;
    strcpy(client.ip_address, "127.0.0.1");

    semop(semid, &verrouP, 1);
    cloud_json = (char *)shmat(shmid, NULL, 0);
    if (cloud_json == (void *)-1)
    {
        perror("erreur shmat");
        exit(1);
    }
    cloud = decode_cloud(cloud_json, MAX_LEN_BUFFER_JSON);
    if (cloud == NULL)
    {
        cerr << "cloud NULL" << endl;
    }
    reservation = init_reservation(cloud, client);
    semop(semid, &signalP, 1);
    semop(semid, &signalV, 1);

    semop(semid, &verrouV, 1);

    fflush(stdin);
    commande* cmd = init_commande(cloud);
    if (cmd == NULL) {cerr<<"Can't init commande"<<endl;return 1;}
    while (1)
    {
        char in_buffer[100];
        int rcv = recvCommandeTCP(ds_client, cmd);
        if (rcv == -1){perror("Error recv:");break;} else {cout << "Message recu : "<< cmd->cmd_type << endl;}

        execute_cmd(cmd, reservation, in_buffer);
        cout << "JOUZ" << endl;

        if (strcmp(in_buffer, "") == 0)
            strcpy(in_buffer, "Erreur à l'éxecution de la commande");

        if (sendTCP(ds_client, in_buffer, strlen(in_buffer)) == -1){perror("Can't send the message:");break;}
        cout << "Send '"<< in_buffer <<"' OK" << endl;
    }
    return 0;
}

/* 
    Ce programme initialise l'état des ressources et se met en écoute des demandes des clients
    Chaque client est gérer par un processus fils (fork()) qui éxecute la fonction .....();
 */
int main(int argc, char const *argv[])
{
    if (argc != 4)
    {
        cout << argv[0] << " chemin_vers_fichier_shared_memory chemin_vers_fichier_json port" << endl;
        exit(1);
    }

    //recupération de la key pour les ipc
    key_t cle = ftok(argv[1], 'a');
    if (cle == -1)
    {
        perror("erreur ftok:");
        exit(1);
    }
    //initialistation de la shared memory
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, IPC_CREAT | 0666)) == -1)
    {
        perror("erreur shmget");
        exit(1);
    }

    cloud = init_cloud_json(argv[2]);
    if (cloud == NULL)
    {
        cerr << "Impossible de créer le Cloud" << endl;
        return 1;
    }

    char *cloud_json = (char *)shmat(shmid, NULL, 0);
    if (cloud_json == (void *)-1)
    {
        perror("erreur shmat");
        exit(1);
    }

    int size_str = code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
    if (size_str == -1)
    {
        cerr << "Impossible de coder le cloud" << endl;
        return 1;
    }

    //shmdt((void *)cloud_json);
    // init semaphore;
    if ((semid = semget(cle, 2, IPC_CREAT | 0666)) == -1)
    {
        perror("erreur semget");
        exit(1);
    }
    semun ctrl;
    ctrl.array = (unsigned short *)malloc(2 * sizeof(unsigned short));
    ctrl.array[0] = 1;
    ctrl.array[1] = 1;
    if (semctl(semid, 0, SETALL, ctrl) == -1)
    {
        perror("erreur init sem");
    }
    id = -1;

    pthread_t thread_main;
    cout << cloud_json << endl;
    if (pthread_create(&thread_main, NULL, wait_thread, NULL) != 0) {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    }
    cout << "IDTHREADMAIN: " << thread_main << endl;
    cout << "****** Initialisation terminée ******" << endl;
    int ds = init_socket(argv[3]);
    cout << "En attente des clients..." << endl;


    while (1)
    {
        struct sockaddr_in addr_client;
        socklen_t socklen;
        // en attente d'une demande de connection
        int ds_client = waiting_for_client(ds, 10, (struct sockaddr *)&addr_client, &socklen);
        int fork_return = -1;

        if ((fork_return = fork()) == -1)
        {
            cerr << "Can't fork" << endl;
            exit(EXIT_FAILURE);
        }
        else if (fork_return == 0)
        {
            // new process
            cout << "fils traitant le client" << endl;
            struct recv r;
            r.ds_client = ds_client;
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, wait_thread, (void *)&r) != 0)
            {
                cerr << "Can't create the waiting thread" << endl;
                return 1;
            }
            if (manage_user(ds_client, atoi(argv[3])) != 0)
            {
                cerr<< "erreur manage_user"<<endl;
            }
            semop(semid, &verrouP, 1);
            if (pthread_cancel(thread_id) != 0)
                cerr << "Impossible de terminer le thread" << endl;

            if (pthread_join(thread_id, NULL) != 0)
                cout << "impossible de joiner" << endl;

            semop(semid, &verrouV, 1);
        }
        else
        {
            cout << "Traitement du client" << endl;
        }
    }

    
    return 0;
}
