#include <iostream>
#include <stdio.h>
#include <json-c/json.h>
#include "../inc/site.hpp"
#include "../inc/cloud.hpp"

using namespace std;

// Implémentation des fonctions de Cloud

Cloud* init_cloud_json(const char* file_name) {
    //! Implémentation temporaire ne marchera pas pour un fichier de plus de MAX_LEN_BUFFER_JSON octets
    // <https://www.google.com/search?channel=fs&client=ubuntu&q=use+json+with+c>
    //malloc
    FILE *file;
    Cloud* cloud;
	char buffer[MAX_LEN_BUFFER_JSON];
	struct json_object *parsed_json;
	struct json_object *name;
	struct json_object *cpu;
	struct json_object *memory;
	struct json_object *site;

	size_t n_sites;

	size_t i;	

	file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Can't acces file:");
        return NULL;
    }

	fread(buffer, MAX_LEN_BUFFER_JSON, 1, file);
    if (!feof(file)) {
        cerr << "Attention le fichier n'a pas été entièrement lu!"<< endl;
    }
	fclose(file);

	parsed_json = json_tokener_parse(buffer);

    n_sites = json_object_array_length(parsed_json);
    cout << "Found " << n_sites << " sites" << endl;
    cloud = (Cloud*) malloc(sizeof(Cloud));
    if (n_sites > 0) {
        cloud->sites = (Site**) malloc(n_sites * sizeof(Site*));
        if (cloud->sites == NULL) {
            perror("Erreur malloc:");
            return NULL;
        }
        cloud->size = n_sites;
    }
    for (i = 0; i < n_sites; i++) {
        site = json_object_array_get_idx(parsed_json, i);
        cout << json_object_get_string(site) << endl;

        json_object_object_get_ex(site, "name", &name);
        json_object_object_get_ex(site, "cpu", &cpu);        
        json_object_object_get_ex(site, "memory", &memory);        

        cloud->sites[i] = init_site(
            json_object_get_string(name), 
            json_object_get_int(cpu), 
            json_object_get_int(memory)
        );
        
    }
    
    return cloud;
}


int destroy_cloud(Cloud* cloud) {
    for (int i = 0; i < cloud->size; i++) {
        if (destroy_site(cloud->sites[i]) == -1) return -1;
    }
    free(cloud->sites);
    cloud->size = 0;
    return 0;
}

void print_cloud(Cloud* cloud) {
    if (cloud == NULL) {
        cout << "cloud = NULL" << endl;
        return;
    }
    for(int i = 0; i < cloud->size; i++) {
        cout << "Site " << i << ": ";
        print_site(cloud->sites[i]);
        cout << endl;
    }
    return;
}