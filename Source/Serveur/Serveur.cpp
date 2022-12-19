#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierClient.h"

int idQ, idShm, idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;

void afficheTab();
void SIGINTHANDLER(int signum);

int main()
{
    // Armement des signaux
    // TO DO

	struct sigaction sig;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sig.sa_handler = SIGINTHANDLER;
	sigaction(SIGINT, &sig, NULL);

    // Creation des ressources
    // Creation de la file de message
    fprintf(stderr, "(SERVEUR %d) Creation de la file de messages\n", getpid());
    if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0600)) == -1) // CLE definie dans protocole.h
    {
        perror("(SERVEUR) Erreur de msgget");
        exit(1);
    }

    // TO BE CONTINUED

    // Creation du pipe
    // TO DO

    // Initialisation du tableau de connexions
    tab = (TAB_CONNEXIONS *)malloc(sizeof(TAB_CONNEXIONS));

    for (int i = 0; i < 6; i++)
    {
        tab->connexions[i].pidFenetre = 0;
        strcpy(tab->connexions[i].nom, "");
        tab->connexions[i].pidCaddie = 0;
    }
    tab->pidServeur = getpid();
    tab->pidPublicite = 0;

    afficheTab();

    // Creation du processus Publicite (étape 2)
    // TO DO

    // Creation du processus AccesBD (étape 4)
    // TO DO

    MESSAGE m;
    MESSAGE reponse;

    while (1)
    {
        fprintf(stderr, "(SERVEUR %d) Attente d'une requete...\n", getpid());
        if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), 1, 0) == -1)
        {
            perror("(SERVEUR) Erreur de msgrcv");
            msgctl(idQ, IPC_RMID, NULL);
            exit(1);
        }

        int i;
        int status = 0;
        switch (m.requete)
        {
        case CONNECT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CONNECT reçue de %d\n", getpid(), m.expediteur);

            i = 0;

            while (i < 6 && tab->connexions[i].pidFenetre != 0)
                i++;

            if (tab->connexions[i].pidFenetre == 0)
                tab->connexions[i].pidFenetre = m.expediteur;
            else
                fprintf(stderr, "Impossible d'ajouter plus de 6 fenetres !\n");
            break;

        case DECONNECT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete DECONNECT reçue de %d\n", getpid(), m.expediteur);
            i = 0;

            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;
            if (tab->connexions[i].pidFenetre == m.expediteur)
            {
                tab->connexions[i].pidFenetre = 0;
                fprintf(stderr, "(SERVEUR %d) Client : %d supprimé du tableau de connexions !\n", getpid(), m.expediteur);
            }
            else
                fprintf(stderr, "(SERVEUR %d) Impossible de supprimer le client #%d du tableau de connexions ! Client non trouvé\n", getpid(), m.expediteur);

            break;
			
        case LOGIN: // TO DO

            fprintf(stderr, "(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n", getpid(), m.expediteur, m.data1, m.data2, m.data3);

			/* Si nouveau client coché */
            if (m.data1 == 1) {
                if ((status = estPresent(m.data2)) >= 1) {
                    fprintf(stderr, "(SERVEUR %d) Client \"%s\" existe déja !\n", getpid(), m.data2);
					m.data1 = 0;
					strcpy(m.data4, "Nom d'utilisateur déja existant. Veuillez vous connecter avec votre mot de passe !");
				}
                else if (status == 0 || status == -1)
                {
                    /* TODO : Ajouter requete vers client pour info si c'est successfull */
                    ajouteClient(m.data2, m.data3);
                    fprintf(stderr, "(SERVEUR %d) Nouveau client \"%s\" crée !\n", getpid(), m.data2);
					m.data1 = 1;
					strcpy(m.data4, "Nouveau client crée avec succès. Vous êtes coonecté !");
                }
            }

			/* Sinon, l'utilisateur essaye de se connecté */
            else {
				/* Si le nom d'utlisateur n'est pas présent */
                if ((status = estPresent(m.data2)) == -1){
                    fprintf(stderr, "(SERVEUR %d) Client \"%s\" Inconnu !\n", getpid(), m.data2);
					m.data1 = 0;
					strcpy(m.data4, "Nom d'utilisateur inconnu ! Veuillez d'abord vous enregistrer en cochant \"Nouveau client\"");
                }
                else{
					/* Si c'est le cas, on verifie le mot de passe */

                    if (verifieMotDePasse(status, m.data3)) {

						i = 0;
						while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
							i++;
						if (tab->connexions[i].pidFenetre == m.expediteur) {
							strcpy(tab->connexions[i].nom, m.data2);
							fprintf(stderr, "(SERVEUR %d) Client \"%s\" logé !\n", getpid(), m.data2);
							m.data1 = 1;
							strcpy(m.data4, "Re-bonjour !");
						}

						/* Cas très rare mais on pourrait imaginer que une erreur interne ce soit passée */
						else {
							fprintf(stderr, "(SERVEUR %d) Impossible d'associer l'utilisateur au tableau de connexions !\n", getpid());
							m.data1 = 0;
							strcpy(m.data4, "Une erreur interne au serveur est survenue !");
						}
						
					}
					else {
						fprintf(stderr, "(SERVEUR %d) Client \"%s\" à entré un mot de passe incorrect !\n", getpid(), m.data2);
						m.data1 = 0;
						strcpy(m.data4, "Le mot de passe entré est incorrect !");
					}
				}
            }

			/* Envois de la reponse au client pour savoir si il est connecté ou pas */
			
            m.type = m.expediteur;
            m.expediteur = 1;
            m.requete = LOGIN;
            if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
            {
                fprintf(stderr, "(SERVEUR %d) Impossible d'envoyer la reponse au client #%ld", getpid(), m.type);
            }

			/* Avertir le client que une reponse à été envoyer par un SIGUSR1 */
			
            kill(m.type, SIGUSR1);
            break;

        case LOGOUT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete LOGOUT reçue de %d\n", getpid(), m.expediteur);

			i = 0;

			while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
				i++;

			if (tab->connexions[i].pidFenetre == m.expediteur) {
				if (*tab->connexions[i].nom != '\0') {
					*tab->connexions[i].nom = '\0';
					fprintf(stderr, "(SERVEUR %d) Le client associé au PID #%d est déconnecté !\n", getpid(), tab->connexions[i].pidFenetre);
				}
				else {
					fprintf(stderr, "(SERVEUR %d) Requete LOGOUT venant du client #%d ignorée, l'utilisateur n'étais pas connecté au préalable !\n", getpid(), tab->connexions[i].pidFenetre);
				}
			}
			else {
				fprintf(stderr, "(SERVEUR %d) Requete LOGOUT venant du client #%d ignorée, le client n'existe pas dans le tableau de connexions !\n", getpid(), tab->connexions[i].pidFenetre);
			}
			
            break;

        case UPDATE_PUB: // TO DO
            break;

        case CONSULT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);
            break;

        case ACHAT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete ACHAT reçue de %d\n", getpid(), m.expediteur);
            break;

        case CADDIE: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CADDIE reçue de %d\n", getpid(), m.expediteur);
            break;

        case CANCEL: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CANCEL reçue de %d\n", getpid(), m.expediteur);
            break;

        case CANCEL_ALL: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n", getpid(), m.expediteur);
            break;

        case PAYER: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete PAYER reçue de %d\n", getpid(), m.expediteur);
            break;

        case NEW_PUB: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete NEW_PUB reçue de %d\n", getpid(), m.expediteur);
            break;
        }
        afficheTab();
    }
}

void afficheTab()
{
    fprintf(stderr, "Pid Serveur   : %d\n", tab->pidServeur);
    fprintf(stderr, "Pid Publicite : %d\n", tab->pidPublicite);
    fprintf(stderr, "Pid AccesBD   : %d\n", tab->pidAccesBD);
    for (int i = 0; i < 6; i++)
        fprintf(stderr, "%6d -%20s- %6d\n", tab->connexions[i].pidFenetre,
                tab->connexions[i].nom,
                tab->connexions[i].pidCaddie);
    fprintf(stderr, "\n");
}

void SIGINTHANDLER(int signum)
{
	fprintf(stderr, "(SERVEUR %d) Reçus du signal \"SIGINT\". Nettoyer de la file...\n", getpid());

    if (msgctl(idQ, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "(SERVEUR %d) Impossible de supprimer la file de messages !\n", getpid());
	}

	fprintf(stderr, "(SERVEUR %d) File de messages supprimée !\n", getpid());
	exit(0);

}