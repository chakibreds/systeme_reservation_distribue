#ifndef RESERVATION_HPP
#define RESERVATION_HPP

#include "../inc/site.hpp"
#include "../inc/cloud.hpp"

typedef struct {
    Resource* resources;
    char** name; // of size [MAX_LEN_NAME_SITE];
    int number_reserved;
    Client client;
} Reservation;


Reservation* init_reservation(Cloud*, Client);

int free_reservation(Reservation*);

/* 
    @return the position of name_server in resources list
 */
int get_resource_by_server_name(Reservation*, char* name_server);


Resource* get_resource_by_id(Reservation*, int id);

/* 
    Ajout d'une ressource au tableau de ressources donnée en param
    @return NULL en cas d'échec ou le pointeur vers la nouvelle adresse
 */
Reservation* alloc_resource(Reservation*, Resource, int id);

/* 
    Supprime l'élement j
 */
Reservation* free_resource(Reservation*, int id);

void print_reservation(Reservation*);

#endif