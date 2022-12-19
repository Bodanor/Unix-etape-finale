#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
char *pShm;
void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);
int fd;

int main()
{
    // Armement des signaux
    // TO DO
    struct sigaction User1;

    User1.sa_flags = 0;
    sigemptyset(&User1.sa_mask);
    User1.sa_handler = handlerSIGUSR1;

    struct sigaction User2;

    User2.sa_flags = 0;
    sigemptyset(&User2.sa_mask);
    User2.sa_handler = handlerSIGUSR1;

    if (sigaction(SIGUSR1, &User1, NULL) == -1)
    {
        perror("Erreur de sigaction : ");
        exit(1);
    }

    if (sigaction(SIGUSR2, &User2, NULL) == -1)
    {
        perror("Erreur de sigaction : ");
        exit(1);
    }

    // Masquage des signaux
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr, "(PUBLICITE %d) Recuperation de l'id de la file de messages\n", getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
        perror("(PUBLICITE) Erreur de msgget");
        exit(1);
    }

    // Recuperation de l'identifiant de la mémoire partagée

    // Attachement à la mémoire partagée
    pShm = (char *)malloc(52); // a supprimer et remplacer par ce qu'il faut
    int taille = strlen(pShm);
    if (idShm = shmget(idQ, taille, IPC_CREAT | IPC_EXCL | 0600))
    {
        perror("Erreur de shmget");
        exit(1);
    }
    // Mise en place de la publicité en mémoire partagée
    char pub[51];
    strcpy(pub, "Bienvenue sur le site du Maraicher en ligne !");

    for (int i = 0; i <= 50; i++)
        pShm[i] = ' ';
    pShm[51] = '\0';
    int indDebut = 25 - strlen(pub) / 2;
    for (int i = 0; i < strlen(pub); i++)
        pShm[indDebut + i] = pub[i];

    while (1)
    {
        // Envoi d'une requete UPDATE_PUB au serveur

        sleep(1);

        // Decallage vers la gauche
    }
}

void handlerSIGUSR1(int sig)
{
    fprintf(stderr, "(PUBLICITE %d) Nouvelle publicite !\n", getpid());

    // Lecture message NEW_PUB

    // Mise en place de la publicité en mémoire partagée
}

void handlerSIGUSR2(int sig)
{
}