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
#include <mysql.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ;

ARTICLE articles[10];
int nbArticles = 0;

int fdWpipe;
int pidClient;

MYSQL *connexion;

void handlerSIGALRM(int sig);

int main(int argc, char *argv[])
{
    // Masquage de SIGINT
    sigset_t mask;
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // Armement des signaux
    // TO DO

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr, "(CADDIE %d) Recuperation de l'id de la file de messages\n", getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
        perror("(CADDIE) Erreur de msgget");
        exit(1);
    }

    // Connexion à la base de donnée
    
    MESSAGE m;
    MESSAGE reponse;

    char requete[200];
    char newUser[20];
    int ret;
    int temp_int;
    int i;
    // Récupération descripteur écriture du pipe
     fdWpipe = atoi(argv[1]); //HOUSTON WE HAVE A PROBLEM

    while (1)
    {
        if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
        {
            perror("(CADDIE) Erreur de msgrcv");
            exit(1);
        }
        
        switch (m.requete)
        {
        case LOGIN: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete LOGIN reçue de %d\n", getpid(), m.expediteur);
            pidClient = m.expediteur;
            
            break;

        case LOGOUT: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete LOGOUT reçue de %d\n", getpid(), m.expediteur);
            mysql_close(connexion);
            exit(0);
            break;

        case CONSULT: // TO DO
        
            fprintf(stderr, "(CADDIE %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);

            m.expediteur = getpid(); // CONNARD C A CAUSE DE TOI QU ON A GALERER FDP
            if ((ret = write(fdWpipe, &m,sizeof(MESSAGE))) != sizeof(MESSAGE)) {
                fprintf(stderr, "(CADDIE %d) Erreur de write !\n", getpid());
                printf("%d != %d\n", (int)strlen(requete) + 1, ret);
                exit(1);
            }
            // Message recv de AccesBD

            if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
            {
                fprintf(stderr, "CADDIE (apres le pipe) Erreur de msgrcv : ");
                exit(1);
            }
            fprintf(stderr, "(CADDIE %d) Requete CONSULT reçue de ACCESBD %d\n", getpid(), m.expediteur);
            m.type = pidClient;
            m.requete = CONSULT;
            m.expediteur = getpid();
            fprintf(stderr, "(CADDIE %d) Envoie de la requete CONSULT à client %ld\n", getpid(), m.type);
            if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
            {
                fprintf(stderr, "(CADDIE %d) Erreur de msgsend\n", getpid());
                exit(1);
            }

            kill(pidClient, SIGUSR1);
            
            break;

        case ACHAT: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete ACHAT reçue de %d\n", getpid(), m.expediteur);

            // on transfert la requete à AccesBD

            fprintf(stderr, "(CADDIE %d) Envoie de la requete ACHAT à ACCESBD sur le pipe\n", getpid());
            m.expediteur = getpid();
            if ((ret = write(fdWpipe, &m,sizeof(MESSAGE))) != sizeof(MESSAGE)) {
                fprintf(stderr, "(CADDIE %d) Erreur de write !\n", getpid());
                printf("%d != %d\n", (int)strlen(requete) + 1, ret);
                exit(1);
            }
            // on attend la réponse venant de AccesBD

            if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
            {
                fprintf(stderr, "CADDIE (apres le pipe) Erreur de msgrcv : ");
                exit(1);
            }
            fprintf(stderr, "(CADDIE %d) Requete ACHAT reçue de ACCESBD \n", getpid());
            if (strcmp(m.data3, "0") != 0) {
                if (nbArticles < 10) {
                    // L'achat à été possible
                    articles[nbArticles].id = m.data1;
                    strcpy(articles[nbArticles].intitule, m.data2);
                    articles[nbArticles].stock = atoi(m.data3);
                    strcpy(articles[nbArticles].image, m.data4);
                    articles[nbArticles].prix = m.data5;
                    nbArticles++;
                }

            }
            else
                m.data1 = -1;

            m.type = pidClient;
            m.requete = ACHAT;
            m.expediteur = getpid();

            fprintf(stderr, "(CADDIE %d) Envoie de la requete ACHAT au client #%ld\n", getpid(), m.type);
             // Envoi de la reponse au client

            if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
            {
                fprintf(stderr, "(CADDIE %d) Erreur de msgsend\n", getpid());
                exit(1);
            }

            kill(pidClient, SIGUSR1);

            break;

        case CADDIE: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CADDIE reçue de %d\n", getpid(), m.expediteur);
            
            for (int i = 0; i < nbArticles; i++) {
                m.type = pidClient;
                m.expediteur = getpid();
                m.requete = CADDIE;
                m.data1 = articles[i].id;
                strcpy(m.data2, articles[i].intitule);
                sprintf(m.data3, "%d", articles[i].stock);
                strcpy(m.data4, articles[i].image);
                m.data5 = articles[i].prix;
                
                fprintf(stderr, "(CADDIE %d) Envoi de la requete CADDIE au client #%ld\n", getpid(), m.type);
                if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
                {
                    fprintf(stderr, "(CADDIE %d) Erreur de msgsend\n", getpid());
                    exit(1);
                }
                usleep(10000); // Fix car trop de signaux envoyer ne meme temps
                if (kill(pidClient, SIGUSR1) == -1) {
                    perror("Erreur de kill");
                }
            }
            
            break;

        case CANCEL: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CANCEL reçue de %d\n", getpid(), m.expediteur);

            // on transmet la requete à AccesBD
            fprintf(stderr, "(CADDIE %d) Envoie de la requete CANCEL à ACCESBD sur le pipe\n", getpid());
            m.expediteur = getpid();
            temp_int = m.data1; // Variable tampon car m.data1 vers ACCESBD contiendra non pas l'indice mais l'ID de l'article
            
            printf("---------------------\n");
            m.data1 = articles[temp_int].id;
            sprintf(m.data2, "%d", articles[temp_int].stock);
            if ((ret = write(fdWpipe, &m,sizeof(MESSAGE))) != sizeof(MESSAGE)) {
                fprintf(stderr, "(CADDIE %d) Erreur de write !\n", getpid());
                printf("%d != %d\n", (int)strlen(requete) + 1, ret);
                exit(1);
            }
            
            // Suppression de l'article du panier
            for(i = temp_int; i<9;i++)
                articles[i] = articles[i+1];
            
            nbArticles--;
            
            break;

        case CANCEL_ALL: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CANCEL_ALL reçue de %d\n", getpid(), m.expediteur);

            // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier

            while(nbArticles >= 0)
            {
                fprintf(stderr, "(CADDIE %d) Envoie de la requete CANCEL à ACCESBD sur le pipe\n", getpid());
                m.expediteur = getpid();
                m.requete = CANCEL;
                
                m.data1 = articles[nbArticles].id;
                sprintf(m.data2, "%d", articles[nbArticles].stock);
                if ((ret = write(fdWpipe, &m,sizeof(MESSAGE))) != sizeof(MESSAGE)) {
                    fprintf(stderr, "(CADDIE %d) Erreur de write !\n", getpid());
                    printf("%d != %d\n", (int)strlen(requete) + 1, ret);
                    exit(1);
                }
                nbArticles --;
            }

            // On vide le panier
            break;

        case PAYER: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete PAYER reçue de %d\n", getpid(), m.expediteur);
            nbArticles = 0;
            // On vide le panier
            break;
        }
    }
}

void handlerSIGALRM(int sig)
{
    fprintf(stderr, "(CADDIE %d) Time Out !!!\n", getpid());

    // Annulation du caddie et mise à jour de la BD
    // On envoie a AccesBD autant de requetes CANCEL qu'il y a d'articles dans le panier

    // Envoi d'un Time Out au client (s'il existe toujours)

    exit(0);
}