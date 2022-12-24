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
MYSQL *connexion;
int temp;

int main(int argc, char *argv[])
{
    // Masquage de SIGINT
    sigset_t mask;
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr, "(ACCESBD %d) Recuperation de l'id de la file de messages\n", getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
        perror("(ACCESBD) Erreur de msgget");
        exit(1);
    }

    // Récupération descripteur lecture du pipe
    int fdRpipe = atoi(argv[1]);

    // Connexion à la base de donnée
    connexion = mysql_init(NULL);
    if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL)
    {
        fprintf(stderr, "(SERVEUR) Erreur de connexion à la base de données...\n");
        exit(1);
    }

    MESSAGE m;
    int ret;
    char requete[200];
    int nbChamps;
    MYSQL_RES *resultat;
    MYSQL_ROW ligne;
    while (1)
    {
        // Lecture d'une requete sur le pipe
        if ((ret = read(fdRpipe, &m, sizeof(MESSAGE))) != sizeof(MESSAGE))
            fprintf(stderr, "Erreur de read pour le pipe");

        fprintf(stderr, "(ACCESBD %d) Lu(%d)\n", getpid(), ret);

        // TO DO

        switch (m.requete)
        {
        case CONSULT: // TO DO
            fprintf(stderr, "(ACCESBD %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);
            sprintf(requete, "select * from UNIX_FINAL WHERE id = %d", m.data1);
            // Acces BD
            m.type = m.expediteur;
            m.expediteur = getpid();
            m.requete = CONSULT;
            temp = m.data1;
            m.data1 = -1;//On initialise à -1 car -1->erreur, si data1 est change cela veut dire que la requete a fonctionné
            if (mysql_query(connexion, requete) == 0)
            {

                if ((resultat = mysql_store_result(connexion)) == NULL)
                {
                    fprintf(stderr, "(ACCESBD %d) Impossible d'obtenir le résultat !\n", getpid());
                }
                else
                {
                    nbChamps = mysql_num_fields(resultat);
                    if ((ligne = mysql_fetch_row(resultat)) != NULL)
                    {
                        m.data1 = temp;
                        strcpy(m.data2, ligne[1]);
                        strcpy(m.data3, ligne[3]);
                        strcpy(m.data4, ligne[4]);
                        m.data5 = atof(ligne[2]);
                    }
                    else
                        fprintf(stderr, "(ACCESBD %d) Une erreur interne à la requete est survenue !\n", getpid());
                }
            }
            else
            {
                fprintf(stderr, "(ACCESBD %d) Impossible d'envoyer la requete !\n", getpid());
            }
            if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
                fprintf(stderr, "(ACCESBD %d) Apres Pipe : Erreur de msgsend", getpid());
            
            
            break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD
                       sprintf(requete, "select * from UNIX_FINAL WHERE id = %d", m.data1);
                      // Acces BD
                      m.type = m.expediteur;
                      m.expediteur = getpid();
                      m.requete = ACHAT;
        
                      if (mysql_query(connexion, requete) == 0)
                      {
                        if ((resultat = mysql_store_result(connexion)) == NULL)
                        {
                            fprintf(stderr, "(ACCESBD %d) Impossible d'obtenir le résultat !\n", getpid());
                        }
                        else
                        {
                            printf("Data2 = %s||Ligne[3] = %s\n", m.data2, ligne[3]);
                            if(strcmp(m.data2, ligne[3]) <= 0)
                            {
                                // L'achat est possible
                                nbChamps = mysql_num_fields(resultat);
                                if ((ligne = mysql_fetch_row(resultat)) != NULL)
                                {
                                    // Update de la base de donnée

                                    sprintf(requete, "update UNIX_FINAL set stock =stock-%d where id=%d", atoi(m.data2), m.data1);
                                    printf("%s\n", requete);
                                    if (mysql_query(connexion, requete) != 0)
                                    {
                                        // Impossible de mettre a jour la BD. Donc on retourne une erreur au client !
                                        fprintf(stderr, "(ACCESBD %d) Impossible de mettre à jour la BD !\n", getpid());
                                        sprintf(m.data3, "%s", "0");
                                    }
                                    else
                                    { 
                                        //Quantité OK et tout les tests de la connexion à la BD à fonctionner
                                        fprintf(stderr, "(ACCESBD %d) Base de données mise à jour !\n", getpid());
                                        m.data1 = temp;
                                        strcpy(m.data3, m.data2);
                                        strcpy(m.data2, ligne[1]);
                                        strcpy(m.data4, ligne[4]);
                                        m.data5 = atof(ligne[2]);
                                    }
                                }
                                else
                                    fprintf(stderr, "(ACCESBD %d) Une erreur interne à la requete est survenue !\n", getpid());
                            }
                            else
                            {
                                sprintf(m.data3, "%s", "0");
                            }  
                          }
                        }
                        else
                        {
                            fprintf(stderr, "(ACCESBD %d) Impossible d'envoyer la requete !\n", getpid());
                        }
                          

                    // Finalisation et envoi de la reponse
                    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
                            fprintf(stderr, "(ACCESBD %d) Apres Pipe : Erreur de msgsend", getpid());
                    break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Mise à jour du stock en BD
                      break;

    }
  }
}
