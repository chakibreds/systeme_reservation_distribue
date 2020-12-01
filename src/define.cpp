#include "../inc/define.hpp"
using namespace std;
int reserve_resources(Cloud* cloud, Reservation* reservation,char* server_name, Resource resource) {
    if (cloud == NULL || 
        reservation == NULL || 
        server_name == NULL || 
        resource.cpu < 0 || 
        resource.memory < 0)
        return -1;
    
    if (alloc_resource(get_site_by_name(cloud, server_name), resource.cpu, resource.memory) == -1 ) {
        return -1;
    }

    if (save_reservation(reservation , resource, server_name) == NULL) {
        cerr << "Impossible de trouver le server" << endl;
    }

    return 0;
}

int free_allocation(Cloud* cloud, Reservation* reservation, char* server_name) {
    if (cloud == NULL || 
        reservation == NULL || 
        server_name == NULL)
        return -1;
    
    Resource r;
    if ((r = free_reservation(reservation, server_name)).cpu == -1) return -1;
    return free_resource(get_site_by_name(cloud, server_name),r.cpu, r.memory);

}
