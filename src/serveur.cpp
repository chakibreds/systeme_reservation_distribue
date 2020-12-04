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

using namespace std;

//------ Variables globales de clients
int semid;
int shmid;
char* cloud_json;
Cloud* cloud;
Reservation* reservation;
int id;

struct sembuf verrouP = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};

struct sembuf verrouV = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};

struct sembuf signalP = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};

struct sembuf signalV = {.sem_num = 1, .sem_op = 1, .sem_flg = 0};

struct sembuf signalZ = {.sem_num = 1, .sem_op = 0, .sem_flg = 0};

int init_socket(const char* port) {

    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds == -1){perror("Error socket:");exit(1);}

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((short)atoi(port));
    if (bind(ds, (struct sockaddr *)&addr, sizeof(addr)) == -1){perror("Error bind:");exit(EXIT_FAILURE);}

    return ds;
}
/*
    @return ds_client
*/
int waiting_for_client(int ds, int nb_client, struct sockaddr * addr_client, socklen_t * socklen) {
    if (listen(ds, nb_client) == -1){perror("Error listen:");exit(EXIT_FAILURE);}
    int ds_client = accept(ds, addr_client, socklen);

    if (ds_client == -1){perror("Erreur accept:");exit(EXIT_FAILURE);}
    return ds_client;
}

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

/* premier thread attends une modification pour afficher */
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
}

int manage_user(int ds, int ds_client, struct sockaddr_in addr_client, socklen_t socklen) {
    Client client;
    client.id = ++id;
    client.port = 34400; //! a modifier
    strcpy(client.ip_address, "127.0.0.1");
    cout << "entrez votre nom svp : ";
    fgets(client.name, MAX_LEN_NAME_CLIENT,stdin);
    client.name[strlen(client.name)-1]='\0';
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;
    
    semop(semid, &verrouP, 1);
    cloud = decode_cloud(cloud_json,MAX_LEN_BUFFER_JSON);
    reservation = init_reservation(cloud, client);
    semop(semid, &verrouV, 1);
    print_cloud(cloud);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, wait_thread, NULL) != 0) {
        cerr << "Can't create the waiting thread" << endl;
        return 1;
    }
    fflush(stdin);
    while (1) {
        cout << endl << "> ";
        char in_buffer[100];
        fgets(in_buffer,100,stdin);
        in_buffer[strlen(in_buffer)-1]='\0';
         
        commande cmd = interpret_cmd(in_buffer);
        
        if (execute_cmd(cmd, reservation) == -1)
            cerr << "Erreur à l'éxecution de la commande" << endl;
        if (cmd.cmd_type == CMD_EXIT)
            break;
    }
    
    //free all
    semop(semid, &verrouP, 1);
    if (pthread_cancel(thread_id) != 0) cerr << "Impossible de terminer le thread" << endl;
   

    
    if(pthread_join(thread_id,NULL) != 0)
        cout << "impossible de joiner" << endl;
    
    semop(semid, &verrouV, 1); 
    return 0;
}

/* 
    Ce programme initialise l'état des ressources et se met en écoute des demandes des clients
    Chaque client est gérer par un processus fils (fork()) qui éxecute la fonction .....();
 */
int main(int argc, char const *argv[])
{
    if (argc != 4) {
        cout << argv[0] <<" chemin_vers_fichier_shared_memory chemin_vers_fichier_json port" << endl;
        exit(1);
    }

    //recupération de la key pour les ipc
    key_t cle = ftok(argv[1], 'a');
    if (cle == -1) {
        perror("erreur ftok:");
        exit(1);
    }
    //initialistation de la shared memory
    int shmid;
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, IPC_CREAT | 0666)) == -1) {
        perror("erreur shmget");
        exit(1);
    }

    Cloud *cloud = init_cloud_json(argv[2]);
    if (cloud == NULL) {
        cerr << "Impossible de créer le Cloud" << endl;
        return 1;
    }

    char *cloud_json = (char *)shmat(shmid, NULL, 0);
    if (cloud_json == (void *)-1) {
        perror("erreur shmat");
        exit(1);
    }

    int size_str = code_cloud(cloud, cloud_json, MAX_LEN_BUFFER_JSON);
    if (size_str == -1) {
        cerr << "Impossible de coder le cloud" << endl;
        return 1;
    }

    //shmdt((void *)cloud_json);
    // init semaphore;
    int semid;
    if ((semid = semget(cle, 2, IPC_CREAT | 0666)) == -1) {
        perror("erreur semget");
        exit(1);
    }
    semun ctrl;
    ctrl.array = (unsigned short*)malloc(2*sizeof(unsigned short));
    ctrl.array[0]=1;ctrl.array[1]=1;
    if (semctl(semid,0,SETALL, ctrl) == -1) {
        perror("erreur init sem");
    }
    id = -1;
    cout << "****** Initialisation terminée ******" << endl;
    cout << "En attente des clients..."  << endl;


    int ds = init_socket(argv[3]);

    while (1) {
        // en attente d'une demande de connection
        struct sockaddr_in addr_client;
        socklen_t socklen;
        int ds_client = waiting_for_client(ds, 10,(struct sockaddr*) &addr_client, &socklen);
        int fork_return = -1;
        if ((fork_return = fork()) == -1) {cerr << "Can't fork" << endl;exit(EXIT_FAILURE);}
        else if (fork_return == 0) {
            // new process
            return manage_user(ds, ds_client, addr_client, socklen);
        } else {
            cout << "Traitement du client" << endl;
        }
    }
    return 0;
}
