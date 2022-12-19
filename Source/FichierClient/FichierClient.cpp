#include "FichierClient.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

int estPresent(const char* nom)
{
    int fd, position = 0;
    CLIENT client; 
    int test;
    if ((fd = open(FICHIER_CLIENTS, O_RDONLY)) == -1)
    {
        return -1;
 }
    while ((test = read(fd, &client, sizeof(CLIENT))) == sizeof(CLIENT)){
        if (strcmp(nom, client.nom) == 0){
            close(fd);
            return position + 1;
        }
        position++;
    }
    close(fd);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
    int i;
    int hash = 0;

    for (i = 0; motDePasse[i] != '\0'; i++)
    {
        hash += (i + 1)* motDePasse[i];
    }
    return hash % 97;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteClient(const char* nom, const char* motDePasse)
{
    int fd;
    CLIENT cli;
    
    strcpy(cli.nom, nom);
    cli.hash = hash(motDePasse);
    fd = open(FICHIER_CLIENTS, O_CREAT | O_APPEND | O_WRONLY, 0644);
    write(fd, &cli, sizeof(CLIENT));
    close(fd);
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
    int fd;
    CLIENT client;

    if ((fd = open(FICHIER_CLIENTS, O_RDONLY)) == -1)
        return -1;
    
    lseek(fd, sizeof(CLIENT)*(pos-1), SEEK_SET);

    read(fd, &client, sizeof(CLIENT));
    
    if (hash(motDePasse) == client.hash)
        return 1;
    close(fd);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int listeClients(CLIENT *vecteur) // le vecteur doit etre suffisamment grand
{
    int clients = 0;
    int fd = 0;

    if ((fd = open(FICHIER_CLIENTS, O_RDONLY)) == -1)
        return -1;
    
    while ((read(fd, &vecteur[clients], sizeof(CLIENT))) == sizeof(CLIENT))
            clients++;

    return clients;
}
