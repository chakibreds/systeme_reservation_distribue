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
    res->number_reserved = cloud->size;
    res->client = client;
    return res;
}

int free_reservation(Reservation* res) {
    if (res == NULL) return -1;
    free(res);
    return 0;
}

int get_resource_by_server_name(Reservation* res, char* name_server) {
    int i = 0;
    for (i = 0; i < res->number_reserved ; i++)
        if (strcmp(res->name[i], name_server) == 0)
            break;
    if (i >= res->number_reserved) return -1;
    return i;
}


Resource* get_resource_by_id(Reservation* res, int id) {
    if (id < 0 || id >= res->number_reserved) return NULL;
    return res->resources + id;
}

Reservation* alloc_resource(Reservation* res, Resource resource, int id) {
    Resource* to_modify = get_resource_by_id(res, id);
    if (to_modify == NULL) return NULL;
    to_modify->cpu += resource.cpu;
    to_modify->memory += resource.memory;
    return res;
}

Reservation* free_resource(Reservation* res, int id) {
    Resource* to_modify = get_resource_by_id(res, id);
    if (to_modify == NULL) return NULL;
    to_modify->cpu = 0;
    to_modify->memory = 0;
    return res;
}

void print_reservation(Reservation* res) {
    cout << "Les reservation de " << res->client.name << " : " << endl;
    for (int i = 0; i < res->number_reserved; i++) {
        cout << "{ 'name': '" << res->name[i] << "', 'cpu': " << res->resources[i].cpu << ", 'memory': " << res->resources[i].memory << "} ";
    }
    cout << endl;
    return;
}

/*/-------------------
int alloc_resource(Client client, Site* site, int cpu, int memory) {
    if (get_cpu_available(site) >= cpu && get_memory_available(site) >= memory) {
        int pos_client = exist_client(site->clients, site->number_reserved, client.id);
        if (pos_client == -1) {
            site->reserved = add_resource(site->reserved, {.cpu= cpu, .memory= memory}, site->number_reserved);
            site->clients = add_client(site->clients, client, site->number_reserved);
            site->number_reserved++;
            if (site->clients == NULL || site->reserved == NULL)
                return -1;
        } else {
            site->reserved[pos_client].cpu += cpu;
            site->reserved[pos_client].memory += memory;
        }
        site->ressource_available.cpu -= cpu;
        site->ressource_available.memory -= memory;
        return 0;
    } else 
        return -1;
}

int free_client_resource(Client client, Site* site) {
    int cl = -1;
    for (int i = 0; i < site->number_reserved; i++)
        if (site->clients[i].id == client.id)
            {
                cl = i;
                break;
            }

    if (cl == -1) {
        cerr << "Impossible de libérer la ressource" << endl << "Client num°" << client.id << " Introuvable dans la liste des réservarion" << endl;
        return -1;
    }

    site->reserved = rm_resource(site->reserved, cl, site->number_reserved);
    site->number_reserved -= 1;
    return 0;
}
*/
