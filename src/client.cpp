#include <iostream>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <pthread.h>

#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"


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
        semop(semid, &verrouP, 1);
        destroy_cloud(cloud);
        cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
        semop(semid, &verrouV, 1);
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

    Client client;
    client.id = atoi(argv[2]);
    client.port = 34000;
    strcpy(client.ip_address, "127.0.0.1");
    cout << "entrez votre nom svp : ";
    fgets(client.name, MAX_LEN_NAME_CLIENT,stdin);
    client.name[strlen(client.name)-1]='\0';
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;
    
    semop(semid, &verrouP, 1);
    cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
    Reservation* reservation = init_reservation(cloud, client);
    semop(semid, &verrouV, 1);
    print_cloud(cloud);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, wait_thread, NULL) != 0) {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    }
    fflush(stdin);
    while (1) {
        cout << "> ";
        char cmd[100];
        fgets(cmd,100,stdin);
        cmd[strlen(cmd)-1]='\0';
         
        char* ptr = strtok(cmd, " ");

        if (strcmp(ptr, "help") == 0) {
            cout << "alloc server_name cpu mem : Alloué 'cpu' cpu et 'mem' memory sur le server 'server_name'" << endl;
            cout << "free server_name : Libére tous ce qui a été alloué sur le server 'server_name'" << endl;
        } else if (strcmp(ptr, "exit") == 0){
            // exit process
            return 0;
        } else if (strcmp(ptr, "alloc") == 0){
            // allocation
            cout << "cmd : "<<cmd<<endl;
            char server_name[MAX_LEN_NAME_SITE];
            int cpu, mem;
            ptr = strtok(NULL, " ");
            if (ptr == NULL) {
                cerr << "server_name introuvable" << endl;
            } else {
                strcpy(server_name, ptr);
            }
            ptr = strtok(NULL, " ");
            if (ptr == NULL) {
                cerr << "cpu introuvable" << endl;
            } else {
                cpu = atoi(ptr);
            }
            ptr = strtok(NULL, " ");
            if (ptr == NULL) {
                cerr << "mem introuvable" << endl;
            } else  {
                mem = atoi(ptr);
            }
                cout << "server_name : "<< server_name << "cpu : "<<cpu << "memory :"<< mem << endl;
            semop(semid, &verrouP, 1);
            if (reserve_resources(cloud, reservation, server_name, {.cpu = cpu, .memory = mem}) == -1) {
                cerr << "Impossible d'allouer" << endl;
            } else {
                if (code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON) == -1) {
                    cerr << "impossible de coder le cloud en json" << endl;
                }
                semop(semid, &signalP, 1);
                semop(semid, &signalV, 1);
            }
            semop(semid, &verrouV, 1);

        } else if (strcmp(ptr, "free") == 0) {
            // free
            char server_name[MAX_LEN_NAME_SITE];
           
            ptr = strtok(NULL, " ");
            if (ptr == NULL) {
                cerr << "server_name introuvable" << endl;
            } else {
                strcpy(server_name, ptr);
            }

            semop(semid, &verrouP, 1);
            if (free_allocation(cloud,reservation, server_name) == -1) {
                cerr << "Free impossible" << endl;
            } else {
                if (code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON) == -1) {
                    cerr << "impossible de coder le cloud en json" << endl;
                }
                semop(semid, &signalP, 1);
                semop(semid, &signalV, 1);
            }
            semop(semid, &verrouV, 1);
        } else {
            // non reconnu
            cout << "Commande '" << ptr << "' non reconnu" << endl
            << "Tapez help pour plus d'information" << endl;
        }
    }
    


    return 0;
}
