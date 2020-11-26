#include <iostream>
#include "../inc/site.hpp"
#include <string.h>

using namespace std;

// --- Client

int add_client(Client* clients, Client c, int old_size) {
    //! ça ne marchera pas
    clients = (Client*) realloc(clients, old_size+1);
    if (clients == NULL) return -1;

    clients[old_size].id = c.id;
    strcpy(clients[old_size].name, c.name);
    strcpy(clients[old_size].ip_address, c.ip_address);
    clients[old_size].port = c.port;
    return 0;
}

int rm_client(Client* c, int j, int size) {
    //! ça ne marchera pas
    if (size == 1)
        free(c);
    for (int i = j; i < size - 1; i++)
        c[i] = c[i+1];
    c = (Client*) realloc(c, size-1);
    return c==NULL?-1:0;
}

// --- Resource

int add_resource(Resource* resources, Resource r, int old_size) {
    //! ça ne marchera pas
    resources = (Resource*) realloc(resources, old_size+1);
    if (resources == NULL) return -1;
    resources[old_size].cpu = r.cpu;
    resources[old_size].memory = r.memory;
    return 0;
}

int rm_resource(Resource* r, int j , int size) {
    //! ça ne marchera pas
    if (size == 1)
        free(r);
    for (int i = j; i < size - 1; i++)
        r[i] = r[i+1];
    r = (Resource*) realloc(r, size-1);
    return r==NULL?-1:0;
}

// --- Site

Site* init_site(const char* name, int cpu, int memory) {
    if (strlen(name) > MAX_LEN_NAME_SITE) {
        fprintf(stderr, "Nom trop long\n");
        return NULL;
    }
    Site* site = (Site*) malloc(sizeof(Site));

    strcpy(site->name, name);
    site->ressource_available = {.cpu = cpu, .memory = memory};
    site->number_reserved = 0;
    site->reserved = NULL;
    site->clients = NULL;
    
    return site;
}

void print_site(Site* site) {
    cout << "{'" << site->name << "': cpu: " << site->ressource_available.cpu << ", mem: " << site->ressource_available.memory <<"}";
    return;
}

int destroy_site(Site* site) {
    //! ça ne marchera pas
    if (site != NULL) {    
        free(site);
        return 0;
    } else {
        return -1;
    }
}

int get_cpu_available(Site* site) {
    if (site == NULL)
        return -1;
    else 
        return site->ressource_available.cpu;
}

int get_memory_available(Site* site) {
    if (site == NULL)
        return -1;
    else 
        return site->ressource_available.memory;
}
int alloc_ressource(Client client, Site* site, int cpu, int memory) {
    if (get_cpu_available(site) >= cpu && get_memory_available(site) >= memory) {
        site->ressource_available.cpu -= cpu;
        site->ressource_available.memory -= memory;

        if (add_resource(site->reserved, {.cpu= cpu, .memory= memory}, site->number_reserved) == -1) return -1;
        if (add_client(site->clients, client, site->number_reserved) == -1) return -1;
        site->number_reserved++;
        return 0;
    } else 
        return -1;
}

int free_client_ressource(Client client, Site* site) {
    int i = 0;
    for (i = 0; i < site->number_reserved; i++)
        if (site->clients[i].id == client.id)
            break;

    if (i >= site->number_reserved) {
        cerr << "Impossible de libérer la ressource" << endl << "Client num°" << client.id << " Introuvable dans la liste des réservarion" << endl;
        return -1;
    }

    if (rm_resource(site->reserved, i, site->number_reserved) == 0)
        (site->number_reserved)--;
    else 
        return -1;
    return 0;
}
