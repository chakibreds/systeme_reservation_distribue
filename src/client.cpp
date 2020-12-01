#include <iostream>
#include <stdio.h>
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "define.hpp"
using namespace std;
    int semid;
    int shmid;

    struct sembuf verrouP;
    verrouP.sem_num = 0;
    verrouP.sem_op = -1;
    verrouP.sem_flg = SEM_UNDO;

    struct sembuf verrouV;
    verrouV.sem_num = 0;
    verrouV.sem_op = 1;
    verrouV.sem_flg = SEM_UNDO;

    struct sembuf signalP;
    signalP.sem_num = 1;
    signalP.sem_op = -1;
    signalP.sem_flg = SEM_UNDO;

    struct sembuf signalV;
    signalV.sem_num = 1;
    signalV.sem_op = 1;
    signalV.sem_flg = SEM_UNDO;

    struct sembuf signalZ;
    signalZ.sem_num = 1;
    signalZ.sem_op = 0;
    signalZ.sem_flg = SEM_UNDO;

/* premier thread attends une modification pour afficher */
void* wait_thread(void* params)
{
    while(1)
    {
        semop(semid,&signalZ,1);
        cout << "MAJ aprés une modification d'un autre client"<<endl;
        char *str = (char *)shmat(shmid, NULL, 0);
        if (str == (void*)-1)
        {
            perror("erreur shmat");
            exit(1);
        }
        Cloud* cloud = decode_cloud(str,MAX_LEN_BUFFER_JSON);
        print_cloud(cloud);
    }
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
        cerr << "Usage: %s chemin_vers_fichier_ipc id" << endl;
        exit(1);
    }
    Client client;
    client.id = atoi(argv[3]);
    client.port = 34000;
    strcpy(client.ip_address, "127.0.0.1");
    cout << "entrez votre nom svp : ";
    cin >> client.name;
    cout << endl;
    cout << "vous êtes : " << client.name << " et votre id est : " << client.id << endl;
    
    key_t cle = ftok(argv[1], 'a');
    if (cle == -1)
    {
        perror("erreur ftok");
        exit(1);
    }
    printf("ftok ok\n");
    //initialistation de la shared memory
    
    if ((shmid = shmget(cle, sizeof(char) * MAX_LEN_BUFFER_JSON, 0)) == -1)
    {
        perror("erreur shmget");
        exit(1);
    }
    printf("shmget ok\n");
    

    // init semaphore;
    if ((semid = semget(cle, 2, 0)) == -1)
    {
        perror("erreur semget");
        exit(1);
    }
    printf("semget ok\n");
    cout << "voici l'état du cloud à votre arrivée : " << endl;
    while (1)
    {
        char *str = (char *)shmat(shmid, NULL, 0);
        if (str == (void*)-1)
        {
            perror("erreur shmat");
            exit(1);
        }
        Cloud* cloud = decode_cloud(str,MAX_LEN_BUFFER_JSON);
        print_cloud(cloud);
        cout << "pour toute demande veuillez la saisir (pour avoir de l'aide tapez help)"<<endl;
        string str;
        cin>>str;
        if (str.split() == "help")
        {
            cout << "pour une demande voici le format : "<<endl<<"reserve/free site_name nb_cpu nb_memory"
        }
        else if (str == "exit"){
            return 0;
        }
        else{


        }
    }
    


    return 0;
}
