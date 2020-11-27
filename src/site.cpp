#include <iostream>
#include "../inc/site.hpp"
#include <string.h>

using namespace std;

// --- Client

Client* add_client(Client* clients, Client c, int old_size) {
    clients = (Client*) realloc(clients, old_size+1);
    if (clients == NULL) {
        if (old_size == 0)
            clients = (Client*) malloc(sizeof(Client));
        else
            return NULL;
    }

    clients[old_size].id = c.id;
    strcpy(clients[old_size].name, c.name);
    strcpy(clients[old_size].ip_address, c.ip_address);
    clients[old_size].port = c.port;
    return clients;
}

int exist_client(Client* clients, int size, int id) {
    // cheking if existing
    for (int i = 0; i < size; i++)
        if (clients[i].id == id)
            return i;
    return -1;
}

Client* rm_client(Client* c, int j, int size) {
    if (j >= size) 
        return c; // indice j n'exsite pas
    if (size <= 0)
        return NULL;
    
    if (size == 1) {
        free(c);
        return NULL;
    }
    
    for (int i = j; i < size - 1; i++)
        c[i] = c[i+1];
    c = (Client*) realloc(c, size-1);
    return c;
}

// --- Resource

Resource* add_resource(Resource* resources, Resource r, int old_size) {
    resources = (Resource*) realloc(resources, old_size+1);
    if (resources == NULL) {
        if (old_size == 0) 
            resources = (Resource*) malloc(sizeof(Resource));
        else 
            return NULL;
    }
    resources[old_size].cpu = r.cpu;
    resources[old_size].memory = r.memory;
    return resources;
}

Resource* rm_resource(Resource* r, int j , int size) {
    if (j >= size) 
        return r; // indice j n'exsite pas
    if (size <= 0) {
        return NULL;
    }
    if (size == 1) {
        free(r);
        return NULL;
    }
    for (int i = j; i < size - 1; i++)
        r[i] = r[i+1];
    r = (Resource*) realloc(r, size-1);
    return r;
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
    for(int i = 0; i< site->number_reserved ; i++)
    {
        cout << "{'"<<site->clients[i].name<<"': ressources reserved -> cpu: " << site->reserved[i].cpu << ", mem: " << site->reserved[i].memory <<"}";
        cout <<endl;
    }
    return;
}

int destroy_site(Site* site) {
    if (site != NULL) {
        if (site->number_reserved > 0 && site->reserved != NULL && site->clients != NULL) {
            free(site->reserved);
            free(site->clients);
        }
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
