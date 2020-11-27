#ifndef CLOUD_HPP   
#define CLOUD_HPP

#include <iostream>
#include "../inc/site.hpp"

// temporaire
#define MAX_LEN_BUFFER_JSON 1024


/* 
    struct Ressource:
        Une instance de ressources represente plusieurs sites géographiquement distribués

 */
typedef struct {
    Site** sites;
    int size;
} Cloud;

/* 
    Création d'une structure Cloud à partir d'un fichier json
    Attention: malloc utilisé
    @param file_name nom du fichier json
    @return l'adresse du cloud créer
 */
Cloud* init_cloud_json(const char* file_name);

/* 
    Désaloue l'esapce alloué par une struct cloud
    @param cloud le cloud à detruire
    @return 0 si ok -1 si erreur
 */
int destroy_cloud(Cloud*);

/* 
    Ajoute un site au cloud
    @return 0 si OK -1 sinon
 */
int add_site(Cloud*, Site*);

/* 
    Retourne l'adresse du site à la position pos
    @param pos position du site
    @return l'adresse du site ou NULL en cas d'erreur

 */
Site* get_site(Cloud*, int pos);

/* 
    Supprime le site à la position pos du cloud
    @param pos position du site
    @return 0 si OK -1 sinon
 */
int rm_site(Cloud*, int pos);

/* 
    Affiche un cloud
 */
void print_cloud(Cloud*);

/* 
    pour envoyer ces données il faut trouver un moyen de serializer la struct avant de l'envoyer

    // methode code()
    // methode decode()
 */

#endif