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
    connexion = mysql_init(NULL);
    if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL)
    {
        fprintf(stderr, "(SERVEUR) Erreur de connexion à la base de données...\n");
        exit(1);
    }

    MESSAGE m;
    MESSAGE reponse;

    char requete[200];
    char newUser[20];
    MYSQL_RES *resultat;
    MYSQL_ROW ligne;

    // Récupération descripteur écriture du pipe
    // fdWpipe = atoi(argv[1]);//HOUSTON WE HAVE A PROBLEM

    int nbChamps;
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
            break;

        case LOGOUT: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete LOGOUT reçue de %d\n", getpid(), m.expediteur);
            mysql_close(connexion);
            exit(0);
            break;

        case CONSULT: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);
            sprintf(requete, "select * from UNIX_FINAL WHERE id = %d", m.data1);
            if (mysql_query(connexion, requete) == 0)
            {

                if ((resultat = mysql_store_result(connexion)) == NULL)
                {
                    fprintf(stderr, "(CADDIE %d) Impossible d'obtenir le résultat !\n", getpid());
                }
                else
                {
                    nbChamps = mysql_num_fields(resultat);
                    if ((ligne = mysql_fetch_row(resultat)) != NULL)
                    {
                        strcpy(m.data2, ligne[1]);
                        strcpy(m.data3, ligne[3]);
                        strcpy(m.data4, ligne[4]);
                        m.data5 = atof(ligne[2]);
                    }
                    else
                        fprintf(stderr, "(CADDIE %d) Une erreur interne à la requete est survenue !\n", getpid());
                }
            }
            else
            {
                fprintf(stderr, "(CADDIE %d) Impossible d'envoyer la requete !\n", getpid());
            }
            m.type = m.expediteur;
            m.requete = CONSULT;
            if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
            {
                fprintf(stderr, "Erreur de msgsend");
                exit(1);
            }
            kill(m.expediteur, SIGUSR1);
            break;

        case ACHAT: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete ACHAT reçue de %d\n", getpid(), m.expediteur);

            // on transfert la requete à AccesBD

            // on attend la réponse venant de AccesBD

            // Envoi de la reponse au client

            break;

        case CADDIE: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CADDIE reçue de %d\n", getpid(), m.expediteur);
            break;

        case CANCEL: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CANCEL reçue de %d\n", getpid(), m.expediteur);

            // on transmet la requete à AccesBD

            // Suppression de l'aricle du panier
            break;

        case CANCEL_ALL: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete CANCEL_ALL reçue de %d\n", getpid(), m.expediteur);

            // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier

            // On vide le panier
            break;

        case PAYER: // TO DO
            fprintf(stderr, "(CADDIE %d) Requete PAYER reçue de %d\n", getpid(), m.expediteur);

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