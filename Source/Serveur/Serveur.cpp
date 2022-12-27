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
#include <setjmp.h>

#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierClient.h"

int idQ, idShm, idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;

sigjmp_buf contexte;

void afficheTab();
void SIGINTHANDLER (int signum);
void SIGCHLDHANDLER (int signum);
int main()
{
    // Armement des signaux
    // TO DO

	struct sigaction sig;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sig.sa_handler = SIGINTHANDLER;
	if (sigaction(SIGINT, &sig, NULL) == -1)
    {
        perror("Erreur de sigaction : ");
        exit(1);
    }

    struct sigaction sig_chld;
    sig_chld.sa_flags = 0;
    sigemptyset(&sig_chld.sa_mask);
    sig_chld.sa_handler = SIGCHLDHANDLER;
    if (sigaction(SIGCHLD, &sig_chld, NULL) == -1) {
        perror("Erreur de sigaction : ");
        exit(1);
    }

    // Creation des ressources
    // Creation de la file de message
    fprintf(stderr, "(SERVEUR %d) Creation de la file de messages\n", getpid());
    if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0600)) == -1) // CLE definie dans protocole.h
    {
        perror("(SERVEUR) Erreur de msgget");
        exit(1);
    }

    // Creation de la memoire partagée
    fprintf(stderr, "(SERVEUR %d) Creation de la memoire partagée\n", getpid());
    int taille = 52;
    if (idShm = shmget(idQ, taille, IPC_CREAT | IPC_EXCL | 0600) == -1)
    {
        perror("Erreur de shmget");
        exit(1);
    }

    // TO BE CONTINUED

    // Creation du pipe
    // TO DO
    int fd[2];

    if(pipe(fd) == -1)
    {
        perror("Erreur de pipe");
        exit(1);
    }
    

    // Initialisation du tableau de connexions
    tab = (TAB_CONNEXIONS *)malloc(sizeof(TAB_CONNEXIONS));

    for (int i = 0; i < 6; i++)
    {
        tab->connexions[i].pidFenetre = 0;
        strcpy(tab->connexions[i].nom, "");
        tab->connexions[i].pidCaddie = 0;
    }
    tab->pidServeur = getpid();

    // Creation du processus Publicite (étape 2)

    /*
    tab->pidPublicite = fork();
    if (tab->pidPublicite == 0) {
        if (execlp("./Publicite", "Publicite", NULL) == -1) {
            perror("Impossible de créer le processus publicite !\n");
            exit(1);
        }
    }
    */
    
    afficheTab();

    // Creation du processus AccesBD (étape 4)
    // TO DO

    tab->pidAccesBD = fork();
    if (tab->pidAccesBD == 0) {
        char tmp[10];
        sprintf(tmp, "%d", fd[0]);
        if (execlp("./AccesBD", "AccesBD", tmp, NULL) == -1) {
            perror("Impossible de créer le processus AccesBD !\n");
            exit(1);
        }
    }

    MESSAGE m;
    MESSAGE reponse;
    int idCaddie;
    int logging_ok = 0;
    sigsetjmp(contexte, 1);
    char temp[10];
    
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
                    logging_ok = 1;
                }
            }

			/* Sinon, l'utilisateur essaye de se connecté */
            else {
				/* Si le nom d'utlisateur n'est pas présent */
                if ((status = estPresent(m.data2)) <= 0){
                    fprintf(stderr, "(SERVEUR %d) Client \"%s\" Inconnu !\n", getpid(), m.data2);
					m.data1 = 0;
					strcpy(m.data4, "Nom d'utilisateur inconnu ! Veuillez d'abord vous enregistrer en cochant \"Nouveau client\"");
                }
                else{
					/* Si c'est le cas, on verifie le mot de passe */

                    if (verifieMotDePasse(status, m.data3) == 1) {
                        logging_ok = 1;
					}
					else {
						fprintf(stderr, "(SERVEUR %d) Client \"%s\" à entré un mot de passe incorrect !\n", getpid(), m.data2);
						m.data1 = 0;
						strcpy(m.data4, "Le mot de passe entré est incorrect !");
					}
				}
            }

            if (logging_ok) {
                
                i = 0;
                while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                    i++;
                if (tab->connexions[i].pidFenetre == m.expediteur) {
                    strcpy(tab->connexions[i].nom, m.data2);
                    /* Creation du caddie associer au client */
                    fprintf(stderr, "(SERVEUR %d) Création du caddie pour le client #%d \"%s\" logé !\n", getpid(),m.expediteur, m.data2);
                    

                    idCaddie = fork();
                    sprintf(temp, "%d", fd[1]);
                    if (idCaddie == 0) {
                        if (execlp("./Caddie", "Caddie", temp, NULL) == -1) {
                        perror("Impossible de créer le processus Caddie !\n");
                        exit(1);
                        }
                    }
                    i = 0;

                    while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                        i++;
                    if (tab->connexions[i].pidFenetre == m.expediteur) {
                        tab->connexions[i].pidCaddie = idCaddie;
                        fprintf(stderr, "(SERVEUR %d) Caddie #%d ajouté au client #%d \"%s\" !\n", getpid(),idCaddie, m.expediteur, m.data2);
                        m.type = tab->connexions[i].pidCaddie;
                        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            fprintf(stderr, "(SERVEUR %d) Erreur de msgsnd", getpid());
                        }
                    }
                    else {
                        fprintf(stderr, "(SERVEUR %d) Impossible d'associer un caddie au client #%d \"%s\" !\n", getpid(), m.expediteur, m.data2);
                        m.data1 = 0;
                        strcpy(m.data4, "Une erreur interne au serveur est survenue !");
                    }

                    fprintf(stderr, "(SERVEUR %d) Client \"%s\" logé !\n", getpid(), m.data2);
                    if (m.data1 == 1) // Si nouveau client 
                    {
                        sprintf(m.data4, "Bonjour %s !", m.data2);
                    }
                    else
                    {
                        sprintf(m.data4, "Re-bonjour %s !", m.data2);
                    }
                    m.data1 = 1;
    
                    
                }

                /* Cas très rare mais on pourrait imaginer que une erreur interne ce soit passée */
                else {
                    fprintf(stderr, "(SERVEUR %d) Impossible d'associer l'utilisateur au tableau de connexions !\n", getpid());
                    m.data1 = 0;
                    strcpy(m.data4, "Une erreur interne au serveur est survenue !");
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
            logging_ok = 0;
            break;

        case LOGOUT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete LOGOUT reçue de %d\n", getpid(), m.expediteur);

			i = 0;

			while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
				i++;

			if (tab->connexions[i].pidFenetre == m.expediteur) {
				if (*tab->connexions[i].nom != '\0') {
					*tab->connexions[i].nom = '\0';
					fprintf(stderr, "(SERVEUR %d) Le client #%d est déconnecté !\n", getpid(), tab->connexions[i].pidFenetre);
                    fprintf(stderr, "(SERVEUR %d) Envoie d'une requete LOGOUT au caddie #%d\n", getpid(), tab->connexions[i].pidCaddie);
                    m.expediteur = 1;
                    m.type = tab->connexions[i].pidCaddie;
                    
                    if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
                    {
                        fprintf(stderr, "(SERVEUR %d) Impossible d'envoyer la requete au caddie #%ld", getpid(), m.type);
                    }
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
            fprintf(stderr, "(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n", getpid(), m.expediteur);

            
            for (unsigned int i = 0; i < 6; i++) {
                if (tab->connexions[i].pidFenetre != 0) {
                    fprintf(stderr, "(SERVEUR %d) Envoie du signal SIGUSR2(Update PUB) au client #%d\n", getpid(), m.expediteur);
                    kill(tab->connexions[i].pidFenetre, SIGUSR2);
                }
            }
            
            break;
            
        case CONSULT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);
            
            //reponse destinataire = PID du caddie
            i = 0;

            /* Cherche le cadie correspondant au client */
            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;
            
            if (tab->connexions[i].pidFenetre == m.expediteur) {
                idCaddie = tab->connexions[i].pidCaddie;
            }
            
            reponse.type = idCaddie; //pas sur mais il faut associer le id du processus au bon client apres je ne sais pas si c'est juste ce j'ai fait
            reponse.expediteur = tab->connexions[i].pidFenetre;
            reponse.data1 = m.data1;
            reponse.requete = CONSULT;
            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd");
                exit(1);
            }
            

            break;

        case ACHAT: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete ACHAT reçue de %d\n", getpid(), m.expediteur);
            
            i = 0;

            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;
            if (tab->connexions[i].pidFenetre == m.expediteur)
                idCaddie = tab->connexions[i].pidCaddie;
            
            reponse.type = idCaddie;
            reponse.expediteur = tab->connexions[i].pidFenetre;
            reponse.data1 = m.data1;
            strcpy(reponse.data2, m.data2);
            reponse.requete = ACHAT;

            fprintf(stderr, "(SERVEUR %d) Envoie de la requete ACHAT au Caddie #%ld \n", getpid(), m.type);

            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd");
                exit(1);
            }
            break;

        case CADDIE: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CADDIE reçue de %d\n", getpid(), m.expediteur);
            
            i = 0;
            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;

            if (tab->connexions[i].pidFenetre == m.expediteur)
                idCaddie = tab->connexions[i].pidCaddie;
            
            reponse.type = idCaddie;
            reponse.expediteur = tab->connexions[i].pidFenetre;
            reponse.requete = CADDIE;
            reponse.data1 = 0;
            *reponse.data2 = '\0';
            *reponse.data3 = '\0';
            *reponse.data4 = '\0';
            reponse.data5 = 0.0f;

            fprintf(stderr, "(SERVEUR %d) Envoie de la requete CADDIE au Caddie #%ld \n", getpid(), reponse.type);

            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd");
                exit(1);
            }
            break;

        case CANCEL: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CANCEL reçue de %d\n", getpid(), m.expediteur);
            
            i = 0;
            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;

            if (tab->connexions[i].pidFenetre == m.expediteur)
                idCaddie = tab->connexions[i].pidCaddie;

            reponse.type = idCaddie;
            reponse.expediteur = tab->connexions[i].pidFenetre;
            reponse.requete = CANCEL;
            reponse.data1 = m.data1;

            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd : ");
                exit(1);
            }

            break;

        case CANCEL_ALL: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n", getpid(), m.expediteur);

            i = 0;
            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;
            
            if (tab->connexions[i].pidFenetre == m.expediteur)
                idCaddie = tab->connexions[i].pidCaddie;
            
            reponse.expediteur = m.expediteur;
            reponse.requete = CANCEL_ALL;
            reponse.type = idCaddie;

            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd : ");
                exit(1);
            }

            break;

        case PAYER: // TO DO
            fprintf(stderr, "(SERVEUR %d) Requete PAYER reçue de %d\n", getpid(), m.expediteur);
            
            i = 0;
            while (i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                i++;
            
            if (tab->connexions[i].pidFenetre == m.expediteur)
                idCaddie = tab->connexions[i].pidCaddie;
            
            reponse.expediteur = m.expediteur;
            reponse.requete = PAYER;
            reponse.type = idCaddie;

            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0))
            {
                perror("Erreur de msgsnd : ");
                exit(1);
            }
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
	fprintf(stderr, "(SERVEUR %d) Reçus du signal \"SIGINT\". Nettoyage de la file...\n", getpid());

    if (msgctl(idQ, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "(SERVEUR %d) Impossible de supprimer la file de messages !\n", getpid());
	}
    fprintf(stderr, "(SERVEUR %d) File de messages supprimée !\n", getpid());

    if (shmctl(idShm, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "(SERVEUR %d) Impossible de supprimer la memoire partagée !\n", getpid());
    }
	fprintf(stderr, "(SERVEUR %d) Mémoire partagée supprimée !\n", getpid());
	exit(0);

}
void SIGCHLDHANDLER (int signum)
{
    int i;
    int id;
    int status;

    fprintf(stderr, "(SERVEUR %d) Reçus du signal \"SIGCHLD\". Nettoyage de la table de processus..\n", getpid());
    id = wait(&status);
    if (WIFEXITED(status))
        fprintf(stderr, "(SERVEUR %d) Le fils %d s'est terminé par un exit (%d)\n", getpid(), id, WEXITSTATUS(status));
    
    i = 0;
    while (i < 6 && tab->connexions[i].pidCaddie != id)
		i++;

	if (tab->connexions[i].pidCaddie == id){
        fprintf(stderr, "(SERVEUR %d) Le caddie #%d associé au client #%d est supprimé !\n", getpid(),id, tab->connexions[i].pidFenetre);
        tab->connexions[i].pidCaddie = 0;
    }
    siglongjmp(contexte, 1);

    //Fermeture du pipe 

    close(fdPipe[0]);
    close(fdPipe[1]);
}