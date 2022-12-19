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

void rotation_gauche(char *source);

void rotation_gauche(char *source)
{
    int i;
    char tmp;
    
    i = 0;
    
    tmp = source[0];
    while (i < 51) {
        source[i] = source[i+ 1];
        i++;
    }
    source[50] = tmp;
    source[51] = '\0';
    
}
int main()
{
    // Armement des signaux
    // TO DO
    struct sigaction User1;

    User1.sa_flags = 0;
    sigemptyset(&User1.sa_mask);
    User1.sa_handler = handlerSIGUSR1;


    if (sigaction(SIGUSR1, &User1, NULL) == -1)
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
    if ((idShm = shmget(idQ, 0, 0)) == -1)
    {
        perror("Erreur de shmget");
        exit(1);
    }
    if ((pShm = (char*)shmat(idShm, NULL, 0)) == (void*)-1) {
        perror("Impossible de s'attacher à la mémoire partagée !\n");
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

    MESSAGE m;

    while (1)
    {
        printf("(PUBLICITE %d) Envoi d'une requête d'update pub...\n", getpid());
        m.expediteur = getpid();
        m.requete = UPDATE_PUB;
        m.type = 1;
        if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
        {
	        perror("(Client) Erreur de msgsend");
	        exit(1);
        }
    
        sleep(1);
        rotation_gauche(pShm);
        // Decallage vers la gauche
        
    }
}

void handlerSIGUSR1(int sig)
{
    fprintf(stderr, "(PUBLICITE %d) Nouvelle publicite !\n", getpid());

    /* TODO*/
    // Lecture message NEW_PUB

    // Mise en place de la publicité en mémoire partagée
}

