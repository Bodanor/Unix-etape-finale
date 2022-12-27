#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include <string>
using namespace std;

#include "protocole.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>

extern WindowClient *w;

int idQ, idShm;
bool logged;
char *pShm;
ARTICLE articleEnCours;
float totalCaddie = 0.0;

void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);

#define REPERTOIRE_IMAGES "images/"

WindowClient::WindowClient(QWidget *parent) : QMainWindow(parent), ui(new Ui::WindowClient)
{
    ui->setupUi(this);

    // Configuration de la table du panier (ne pas modifer)
    ui->tableWidgetPanier->setColumnCount(3);
    ui->tableWidgetPanier->setRowCount(0);
    QStringList labelsTablePanier;
    labelsTablePanier << "Article"
                      << "Prix à l'unité"
                      << "Quantité";
    ui->tableWidgetPanier->setHorizontalHeaderLabels(labelsTablePanier);
    ui->tableWidgetPanier->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetPanier->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetPanier->horizontalHeader()->setVisible(true);
    ui->tableWidgetPanier->horizontalHeader()->setDefaultSectionSize(160);
    ui->tableWidgetPanier->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetPanier->verticalHeader()->setVisible(false);
    ui->tableWidgetPanier->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr, "(CLIENT %d) Recuperation de l'id de la file de messages\n", getpid());

    if ((idQ = msgget(CLE, 0)) == -1)
    {
        fprintf(stderr, "Erreur de recuperatoin de la cle\n");
        exit(1);
    }
    fprintf(stderr, "(CLIENT %d) Id de la file : %d\n", getpid(), idQ);

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr, "(CLIENT %d) Recuperation de l'id de la mémoire partagée\n", getpid());
    // TO DO

    // Attachement à la mémoire partagée
    if ((idShm = shmget(idQ, 0, 0)) == -1)
    {
        perror("Erreur de shmget");
        exit(1);
    }

    // Armement des signaux
    struct sigaction sigusr1;
    sigusr1.sa_flags = SA_NODEFER;
    sigusr1.sa_handler = handlerSIGUSR1;
    sigemptyset(&sigusr1.sa_mask);
    sigaddset(&sigusr1.sa_mask, SIGUSR1);
    sigaction(SIGUSR1, &sigusr1, NULL);

    struct sigaction User2;

    User2.sa_flags = 0;
    sigemptyset(&User2.sa_mask);
    User2.sa_handler = handlerSIGUSR2;

    if (sigaction(SIGUSR2, &User2, NULL) == -1)
    {
        perror("Erreur de siguser2 : ");
        exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    // TO DO
    MESSAGE m;
    m.expediteur = getpid();
    m.requete = CONNECT;
    m.type = 1;

    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
    printf("Client %d) Envoi requête, attente réponse...\n", getpid());
}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char *Text)
{
    if (strlen(Text) == 0)
    {
        ui->lineEditNom->clear();
        return;
    }
    ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char *WindowClient::getNom()
{
    strcpy(nom, ui->lineEditNom->text().toStdString().c_str());
    return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char *Text)
{
    if (strlen(Text) == 0)
    {
        ui->lineEditMotDePasse->clear();
        return;
    }
    ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char *WindowClient::getMotDePasse()
{
    strcpy(motDePasse, ui->lineEditMotDePasse->text().toStdString().c_str());
    return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char *Text)
{
    if (strlen(Text) == 0)
    {
        ui->lineEditPublicite->clear();
        return;
    }
    ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setImage(const char *image)
{
    // Met à jour l'image
    char cheminComplet[80];
    sprintf(cheminComplet, "%s%s", REPERTOIRE_IMAGES, image);
    QLabel *label = new QLabel();
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setScaledContents(true);
    QPixmap *pixmap_img = new QPixmap(cheminComplet);
    label->setPixmap(*pixmap_img);
    label->resize(label->pixmap()->size());
    ui->scrollArea->setWidget(label);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauClientChecked()
{
    if (ui->checkBoxNouveauClient->isChecked())
        return 1;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setArticle(const char *intitule, float prix, int stock, const char *image)
{
    ui->lineEditArticle->setText(intitule);
    if (prix >= 0.0)
    {
        char Prix[20];
        sprintf(Prix, "%.2f", prix);
        ui->lineEditPrixUnitaire->setText(Prix);
    }
    else
        ui->lineEditPrixUnitaire->clear();
    if (stock >= 0)
    {
        char Stock[20];
        sprintf(Stock, "%d", stock);
        ui->lineEditStock->setText(Stock);
    }
    else
        ui->lineEditStock->clear();
    setImage(image);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getQuantite()
{
    return ui->spinBoxQuantite->value();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTotal(float total)
{
    if (total >= 0.0)
    {
        char Total[20];
        sprintf(Total, "%.2f", total);
        ui->lineEditTotal->setText(Total);
    }
    else
        ui->lineEditTotal->clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
    ui->pushButtonLogin->setEnabled(false);
    ui->pushButtonLogout->setEnabled(true);
    ui->lineEditNom->setReadOnly(true);
    ui->lineEditMotDePasse->setReadOnly(true);
    ui->checkBoxNouveauClient->setEnabled(false);

    ui->spinBoxQuantite->setEnabled(true);
    ui->pushButtonPrecedent->setEnabled(true);
    ui->pushButtonSuivant->setEnabled(true);
    ui->pushButtonAcheter->setEnabled(true);
    ui->pushButtonSupprimer->setEnabled(true);
    ui->pushButtonViderPanier->setEnabled(true);
    ui->pushButtonPayer->setEnabled(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
    ui->pushButtonLogin->setEnabled(true);
    ui->pushButtonLogout->setEnabled(false);
    ui->lineEditNom->setReadOnly(false);
    ui->lineEditMotDePasse->setReadOnly(false);
    ui->checkBoxNouveauClient->setEnabled(true);

    ui->spinBoxQuantite->setEnabled(false);
    ui->pushButtonPrecedent->setEnabled(false);
    ui->pushButtonSuivant->setEnabled(false);
    ui->pushButtonAcheter->setEnabled(false);
    ui->pushButtonSupprimer->setEnabled(false);
    ui->pushButtonViderPanier->setEnabled(false);
    ui->pushButtonPayer->setEnabled(false);

    setNom("");
    setMotDePasse("");
    ui->checkBoxNouveauClient->setCheckState(Qt::CheckState::Unchecked);

    setArticle("", -1.0, -1, "");

    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du panier (ne pas modifier) /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteArticleTablePanier(const char *article, float prix, int quantite)
{
    char Prix[20], Quantite[20];

    sprintf(Prix, "%.2f", prix);
    sprintf(Quantite, "%d", quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetPanier->rowCount();
    nbLignes++;
    ui->tableWidgetPanier->setRowCount(nbLignes);
    ui->tableWidgetPanier->setRowHeight(nbLignes - 1, 10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetPanier->setItem(nbLignes - 1, 0, item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetPanier->setItem(nbLignes - 1, 1, item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetPanier->setItem(nbLignes - 1, 2, item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::videTablePanier()
{
    ui->tableWidgetPanier->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetPanier->selectionModel()->selectedRows();
    if (liste.size() == 0)
        return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue (ne pas modifier ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char *titre, const char *message)
{
    QMessageBox::information(this, titre, message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char *titre, const char *message)
{
    QMessageBox::critical(this, titre, message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
    // TO DO (étape 1)
    // Envoi d'une requete DECONNECT au serveur
    MESSAGE m;

    /* Si on est déja logé, il faut d'abord envoyer une requete de LOGOUT
     * Ici on simule l'appuie sur le bouton LOGOUT qui va envoyer la requete pour nous
     */

    if (logged == true)
    {
        on_pushButtonLogout_clicked();
    }

    printf("Client %d) Envoi d'une requête de deconnexion...\n", getpid());
    m.expediteur = getpid();
    m.requete = DECONNECT;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

    // Envoi d'une requete de deconnexion au serveur

    exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    // Envoi d'une requete de login au serveur
    // TO DO

    MESSAGE m;
   

    printf("Client %d) Envoi d'une requête de login...\n", getpid());
    m.expediteur = getpid();
    m.requete = LOGIN;
    m.type = 1;
    m.data1 = isNouveauClientChecked() ? 1 : 0;
    strcpy(m.data2, getNom());
    strcpy(m.data3, getMotDePasse());
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

 
    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogout_clicked()
{
    // Envoi d'une requete CANCEL_ALL au serveur (au cas où le panier n'est pas vide)
    // TO DO
    MESSAGE m;
    

    printf("Client %d) Envoi d'une requête de cancel_all...\n", getpid());
    m.expediteur = getpid();
    m.requete = CANCEL_ALL;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

    // Envoi d'une requete de logout au serveur
    // TO DO

    printf("Client %d) Envoi d'une requête de logout...\n", getpid());
    m.expediteur = getpid();
    m.requete = LOGOUT;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

    logged = false;
    logoutOK();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSuivant_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
    MESSAGE m;

    printf("Client %d) Envoi d'une requête de consult...\n", getpid());
    m.expediteur = getpid();
    m.type = 1;
    m.requete = CONSULT;
    m.data1 = 21; // Fix pour que ca reste sur tomates
    if(articleEnCours.id<21) {
        articleEnCours.id++;
        m.data1 = articleEnCours.id;
    }
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPrecedent_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
    MESSAGE m;

    printf("Client %d) Envoi d'une requête de consult...\n", getpid());
    m.expediteur = getpid();
    m.type = 1;
    m.requete = CONSULT;
    m.data1 = 1;
    if (articleEnCours.id > 1)
    {
        articleEnCours.id--;
        m.data1 = articleEnCours.id;
    }
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonAcheter_clicked()
{
    
    // TO DO (étape 5)
    // Envoi d'une requete ACHAT au serveur
    MESSAGE m;

    printf("Client %d) Envoi d'une requête d'achat...\n", getpid());
    m.expediteur = getpid();
    m.requete = ACHAT;
    m.type = 1;
    m.data1 = articleEnCours.id;
    sprintf(m.data2, "%d", getQuantite());
    // Ne pas faire de requete lorsque l'achat est à 0
    if (getQuantite() == 0) {
        dialogueErreur("Achat impossible", "Vous ne pouvez acheter 0 quantitées !");
    }
    else {
        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
        {
            perror("(Client) Erreur de msgsend");
            exit(1);
        }
    }
    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSupprimer_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL au serveur
    MESSAGE m;
    int ret;
    char tmp_dialogue[80];
    printf("Client %d) Envoi d'une requête de cancel...\n", getpid());
    m.expediteur = getpid();
    m.requete = CANCEL;
    m.type = 1;
    
    ret = getIndiceArticleSelectionne();
    if(ret == -1)
    {
        strcpy(tmp_dialogue,"Aucun article selectionné !!(Faites un effort svp)");
        w->dialogueMessage("Achat", tmp_dialogue);
    }
    else {
        m.data1 = ret;
        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
        {
            perror("(Client) Erreur de msgsend");
            exit(1);
        }
    }

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur

    printf("Client %d) Envoi d'une requête de caddie...\n", getpid());
    m.expediteur = getpid();
    m.requete = CADDIE;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonViderPanier_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL_ALL au serveur
    MESSAGE m;

    printf("Client %d) Envoi d'une requête de cancel_all...\n", getpid());
    m.expediteur = getpid();
    m.requete = CANCEL_ALL;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur

    printf("Client %d) Envoi d'une requête de caddie...\n", getpid());
    m.expediteur = getpid();
    m.requete = CADDIE;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPayer_clicked()
{
    // TO DO (étape 7)
    // Envoi d'une requete PAYER au serveur

    MESSAGE m;
    printf("Client %d) Envoi d'une requête de payer...\n", getpid());
    m.expediteur = getpid();
    m.requete = PAYER;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
    char tmp[100];
    sprintf(tmp, "Merci pour votre paiement de %.2f ! Votre commande sera livrée tout prochainement.", totalCaddie);
    dialogueMessage("Payer...", tmp);

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
    printf("Client %d) Envoi d'une requête de caddie...\n", getpid());
    m.expediteur = getpid();
    m.requete = CADDIE;
    m.type = 1;
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        perror("(Client) Erreur de msgsend");
        exit(1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    fprintf(stderr, "(Client %d) Signal SIGUSR1 recu !\n", getpid());
    MESSAGE m;
    int tmp;
    char tmp_dialogue[100];
    if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) != -1) // !!! a modifier en temps voulu !!!
    {
        switch (m.requete)
        {
        case LOGIN:
            printf("Client %d) Requete LOGIN reçue de Caddie ...\n", getpid());
            if (m.data1 == 1)
            {
                w->dialogueMessage("Login", m.data4);
                logged = true;
                w->loginOK();

                printf("Client %d) Envoi d'une requête de consult...\n", getpid());
                m.expediteur = getpid();
                m.requete = CONSULT;
                m.type = 1;
                articleEnCours.id = 1;
                m.data1 = articleEnCours.id;
                if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
                {
                    perror("Erreur de msgsnd");
                    exit(1);
                }
                    
            }
            else
            {
                w->dialogueErreur("Erreur", m.data4);
            }

            break;

        case CONSULT: // TO DO (étape 3)
            printf("Client %d) Requete CONSULT reçue de Caddie ...\n", getpid());
            if (m.data1 != -1) {
                tmp = atoi(m.data3);
                w->setArticle(m.data2, m.data5, tmp, m.data4);
            }
            break;

        case ACHAT: // TO DO (étape 5)
            printf("Client %d) Requete ACHAT reçue de Caddie ...\n", getpid());
            if (strcmp(m.data3, "0") != 0 && m.data1 != -1)
            {
                sprintf(tmp_dialogue, "%s unité(s) de %s achetées avec succès", m.data3, m.data2);
                w->dialogueMessage("Achat", tmp_dialogue);
            }
            else
                w->dialogueMessage("Achat", "Stock insuffisant !");

            fprintf(stderr, "Client %d) Envoi d'une requête de CADDIE ...\n", getpid());
            
            m.type = 1;
            m.expediteur = getpid();
            m.requete = CADDIE;
            m.data1 = 0;
            *m.data2 = '\0';
            *m.data3 = '\0';
            *m.data4 = '\0';
            m.data5 = 0.0f;
            
            
            if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0)) {
                perror("(Client) Erreur de msgsend");
                exit(1);
            }
            
            

            w->videTablePanier();
            totalCaddie = 0.0f;
            break;

        case CADDIE: // TO DO (étape 5)
            
            printf("Client %d) Requete CADDIE reçue de Caddie ...\n", getpid());
            w->ajouteArticleTablePanier(m.data2, m.data5, atoi(m.data3));
            totalCaddie += m.data5*atoi(m.data3);
            w->setTotal(totalCaddie);
            break;

        case TIME_OUT: // TO DO (étape 6)
            printf("Client %d) Requete TIME_OUT reçue de Caddie ...\n", getpid());
            w->logoutOK();
            w->dialogueErreur("Logout", "Cheh fallait pas réfléchir trop longtemps batard ! ");
            break;

        case BUSY: // TO DO (étape 7)
            break;

        default:
            break;
        }
    }
}

void handlerSIGUSR2(int sig)
{

    if ((pShm = (char*)shmat(idShm, NULL, 0)) == (void*)-1) {
        perror("Impossible de s'attacher à la mémoire partagée !\n");
        exit(1);
    }
    w->setPublicite(pShm);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
