#include <iostream>
#include <iostream>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include "../inc/reservation.hpp"
#include "../inc/define.hpp"

using namespace std;

/* 
    Ce programme initialise l'état des ressources et se met en écoute des demandes des clients
    Chaque client est gérer par un processus fils (fork()) qui éxecute la fonction .....();
 */
int main(int argc, char const *argv[])
{
    if (argc != 3) {
        cout << argv[0] <<" chemin_vers_fichier_shared_memory chemin_vers_fichier_json" << endl;
        exit(1);
    }

    //recupération de la key pour les ipc
    key_t cle = ftok(argv[1], 'a');
    if (cle == -1) {
        perror("erreur ftok:");
        exit(1);
    }
    printf("ftok ok\n");
    //initialistation de la shared memory
    int shmid;
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, IPC_CREAT | 0666)) == -1) {
        perror("erreur shmget");
        exit(1);
    }
    printf("shmget ok\n");

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

    shmdt((void *)cloud_json);
    // init semaphore;
    int semid;
    if ((semid = semget(cle, 2, IPC_CREAT | 0666)) == -1) {
        perror("erreur semget");
        exit(1);
    }
    printf("semget ok\n");
    semun ctrl;
    ctrl.array = (unsigned short*)malloc(2*sizeof(unsigned short));
    ctrl.array[0]=1;ctrl.array[1]=1;
    if (semctl(semid,0,SETALL, ctrl) == -1) {
        perror("erreur init sem");
    }
    cout << "semctl ok" << endl;
    cout << "****** Initialisation terminée ******" << endl;
    return 0;
}
