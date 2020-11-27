#ifndef SITE_HPP
#define SITE_HPP

#include <iostream>

/* 
    struct Site:
        Un site à un nom (exemple "Montpellier")
        Un site propose un nombre de processeurs et un volume de stockage

 */

#define MAX_LEN_NAME_SITE 50
#define MAX_LEN_NAME_CLIENT 50

typedef struct {
    int id;
    char name[MAX_LEN_NAME_CLIENT];
    char ip_address[16];
    int port;
} Client;

/* 
    Ajout d'un client au tableau de clients
    @return NULL en cas d'échec ou le pointeur vers la nouvelle adresse
 */
Client* add_client(Client*, Client, int old_size);

int exist_client(Client*, int, int);

/* 
    Supprime l'élement j
    @return NULL en cas d'échec ou le pointeur vers la nouvelle adresse
*/
Client* rm_client(Client*, int, int);

typedef struct {
    int cpu;
    int memory;
} Resource;

/* 
    Ajout d'une ressource au tableau de ressources donnée en param
    @return NULL en cas d'échec ou le pointeur vers la nouvelle adresse
 */
Resource* add_resource(Resource*, Resource, int old_size);

/* 
    Supprime l'élement j
 */
Resource* rm_resource(Resource*, int, int);


typedef struct {
    char name[MAX_LEN_NAME_SITE];
    Resource ressource_available;
    int number_reserved; // nombre de reservation en cours
    Resource* reserved;  // liste des reservation
    Client* clients; // clients en cours de reservations
} Site;

/* 
    Création d'une structure Site
    @param name le nom du site
    @return un poiteur vers une nouvelle structure alloué dynamiquement ou NULL si echec
 */
Site* init_site(const char* name, int cpu, int memory);

/* 
    Affiche les ressources dispo
 */
void print_site(Site*);

/* 
    Désalloue l'espace alloué par site (utilisation d'un free())
 */
int destroy_site(Site*);

int get_cpu_available(Site*);

int get_memory_available(Site*);

/* 
    @return 0 si l'allocation s'est faite sans problème -1 sinon
 */
int alloc_resource(Client, Site*, int cpu, int memory);

/* 
    Libére les ressources alloué par 'client'
 */
int free_client_resource(Client client, Site*);

/* 
    pour envoyer ces données il faut trouver un moyen de serializer la struct avant de l'envoyer

    // methode code()
    // methode decode()
 */

#endif
