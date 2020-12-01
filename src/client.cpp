#include <iostream>
#include <stdio.h>
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <pthread.h>
#include "define.hpp"

using namespace std;
int semid;
int shmid;
char* cloud_json;
Cloud* cloud;

struct sembuf verrouP = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};

struct sembuf verrouV = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};

struct sembuf signalP = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};

struct sembuf signalV = {.sem_num = 1, .sem_op = 1, .sem_flg = 0};

struct sembuf signalZ = {.sem_num = 1, .sem_op = 0, .sem_flg = 0};

/* premier thread attends une modification pour afficher */
void* wait_thread(void* params) {
    while(1) {
        semop(semid,&signalZ,1);
        destroy_cloud(cloud);
        cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
        system("clear");
        cout << "MAJ aprés une modification d'un autre client"<<endl;
        print_cloud(cloud);
        cout << endl;
    }
}

/* 
    Ce programme se connecte au serveur puis 
        - le thread principal attend une demande saisi au clavier par l'utilisateur et l'envoi au serveur.
        - Un thread secondaire qui reste en écoute d'eventuels changement d'états des ressources.
 */
int main(int argc, char const *argv[])
{

    if (argc != 3) {
        cerr << "Usage: "<< argv[0] <<" chemin_vers_fichier_ipc id" << endl;
        exit(1);
    }
    Client client;
    client.id = atoi(argv[2]);
    client.port = 34000;
    strcpy(client.ip_address, "127.0.0.1");
    cout << "entrez votre nom svp : ";
    cin >> client.name;
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;
    
    key_t cle = ftok(argv[1], 'a');
    if (cle == -1)
    {
        perror("erreur ftok");
        exit(1);
    }
    //initialistation de la shared memory
    
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, 0)) == -1)
    {
        perror("erreur shmget");
        exit(1);
    }
    

    // init semaphore;
    if ((semid = semget(cle, 2, 0)) == -1)
    {
        perror("erreur semget");
        exit(1);
    }
        
    cloud_json = (char *)shmat(shmid, NULL, 0);
    if (cloud_json == (void*)-1) {
        perror("erreur shmat");
        exit(1);
    }
    
    cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
    print_cloud(cloud);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, wait_thread, NULL) != 0) {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    }

    while (1) {
        cout << "> ";
        char cmd[100];
        cin >> cmd;

        char* ptr = strtok(cmd, " ");

        if (strcmp(ptr, "help") == 0) {
            cout << "alloc server_name cpu mem : Alloué 'cpu' cpu et 'mem' memory sur le server 'server_name'" << endl;
            cout << "free server_name : Libére tous ce qui a été alloué sur le server 'server_name'" << endl;
        } else if (strcmp(ptr, "exit") == 0){
            // exit process
            return 0;
        } else if (strcmp(ptr, "alloc") == 0){
            // allocation

        } else if (strcmp(ptr, "free") == 0) {
            // free
        } else {
            // non reconnu
            cout << "Commande '" << ptr << "' non reconnu" << endl
            << "Tapez help pour plus d'information" << endl;
        }
    }
    


    return 0;
}
