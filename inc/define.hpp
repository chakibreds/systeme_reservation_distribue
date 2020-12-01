#ifndef DEFINE_HPP
#define DEFINE_HPP

#include "site.hpp"
#include "cloud.hpp"
#include "reservation.hpp"

union semun {
    int val;               /* cmd = SETVAL */
    struct semidds *buf;   /* cmd = IPCSTAT ou IPCSET */
    unsigned short *array; /* cmd = GETALL ou SETALL */
    struct seminfo *__buf; /* cmd = IPCINFO (sous Linux) */
};

/* 
    reserve Resource from Cloud with name 'server_name' and add it to Reservation
    @return -1 error
 */
int reserve_resources(Cloud*, Reservation* ,char*, Resource);

int free_allocation(Cloud*, Reservation*, char*);   

#endif
