#include <iostream>
#include <stdio.h>
#include <string.h>
#include "../inc/reservation.hpp"
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"

using namespace std;

Reservation* init_reservation(Cloud* cloud, Client client) {
    Reservation* res = (Reservation*) malloc(sizeof(Reservation));
    res->resources = (Resource*) malloc(sizeof(Resource) * cloud->size);
    res->name = (char**) malloc(sizeof(char*) * cloud->size);
    for (int i = 0 ; i < cloud->size; i++) {
        res->name[i] = (char*) malloc(sizeof(char) * MAX_LEN_NAME_SITE);
        strcpy(res->name[i], cloud->sites[i]->name);
    }
    res->number_server = cloud->size;
    res->client = client;
    return res;
}

int destroy_reservation(Reservation* res) {
    if (res == NULL) return -1;
    free(res);
    return 0;
}

Resource* get_resource_by_server_name(Reservation* res, char* name_server) {
    int i = 0;
    for (i = 0; i < res->number_server ; i++)
        if (strcmp(res->name[i], name_server) == 0)
            break;
    if (i >= res->number_server) return NULL;
    return res->resources + i;
}


Resource* get_resource_by_id(Reservation* res, int id) {
    if (id < 0 || id >= res->number_server) return NULL;
    return res->resources + id;
}

Reservation* save_reservation(Reservation* res, Resource resource, char* server_name) {
    Resource* to_modify = get_resource_by_server_name(res, server_name);
    if (to_modify == NULL) return NULL;
    to_modify->cpu += resource.cpu;
    to_modify->memory += resource.memory;
    return res;
}

Resource free_reservation(Reservation* res,char* server_name) {
    Resource* to_modify = get_resource_by_server_name(res, server_name);
    if (to_modify == NULL) return {.cpu = -1, .memory=-1};
    Resource resource = {.cpu = to_modify->cpu, .memory = to_modify->memory};
    to_modify->cpu = 0;
    to_modify->memory = 0;
    return resource;
}

void print_reservation(Reservation* res) {
    cout << "Les reservation de " << res->client.name << " : " << endl;
    for (int i = 0; i < res->number_server; i++) {
        cout << "{ 'name': '" << res->name[i] << "', 'cpu': " << res->resources[i].cpu << ", 'memory': " << res->resources[i].memory << "} ";
    }
    cout << endl;
    return;
}

/*/-------------------

*/
