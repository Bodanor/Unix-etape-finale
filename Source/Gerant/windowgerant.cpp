#include "windowgerant.h"
#include "ui_windowgerant.h"
#include <iostream>
using namespace std;
#include <mysql.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include "protocole.h"
#include <string>
#include <cstring>

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short sem;
}arg;

int idArticleSelectionne = -1;
MYSQL *connexion;
MYSQL_RES *resultat;
MYSQL_ROW Tuple;
char requete[200];
int idSem;
int idQ;

WindowGerant::WindowGerant(QWidget *parent) : QMainWindow(parent), ui(new Ui::WindowGerant)
{
    ui->setupUi(this);

    // Configuration de la table du stock (ne pas modifer)
    ui->tableWidgetStock->setColumnCount(4);
    ui->tableWidgetStock->setRowCount(0);
    QStringList labelsTableStock;
    labelsTableStock << "Id"
                     << "Article"
                     << "Prix à l'unité"
                     << "Quantité";
    ui->tableWidgetStock->setHorizontalHeaderLabels(labelsTableStock);
    ui->tableWidgetStock->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetStock->horizontalHeader()->setVisible(true);
    ui->tableWidgetStock->horizontalHeader()->setDefaultSectionSize(120);
    ui->tableWidgetStock->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetStock->verticalHeader()->setVisible(false);
    ui->tableWidgetStock->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de la file de message
    // TO DO
    fprintf(stderr, " Recuperation de l'id de la file de messages\n");

    if ((idQ = msgget(CLE, 0)) == -1)
    {
        fprintf(stderr, "Erreur de recuperatoin de la cle\n");
        exit(1);
    }

    fprintf(stderr, "Recuperation de la cle de du sémaphore\n");
    if((idSem = semget(idQ, 0, 0)) == -1)
    {
        perror("Erreur de semget : ");
        exit(1);
    }

    // Prise blocante du semaphore
    // TO DO
    sembuf sem;
    sem.sem_op = -1;
    sem.sem_flg = SEM_UNDO;
    sem.sem_num = 0;
    if(semop(idSem, &sem, 1) == -1)
    {
        perror("Erreur de semop : ");
        exit(1);
    }

    // Connexion à la base de donnée
    connexion = mysql_init(NULL);
    fprintf(stderr, "(GERANT %d) Connexion à la BD\n", getpid());
    if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL)
    {
        fprintf(stderr, "(GERANT %d) Erreur de connexion à la base de données...\n", getpid());
        exit(1);
    }

    // Recuperation des articles en BD
    // TO DO
    char requete[200];
    MYSQL_RES *resultat;
    MYSQL_ROW ligne;

    sprintf(requete, "select * from UNIX_FINAL");
    if (mysql_query(connexion, requete) == 0)
    {
        if ((resultat = mysql_store_result(connexion)) == NULL)
        {
            fprintf(stderr, "(ACCESBD %d) Impossible d'obtenir le résultat !\n", getpid());
        }
        else
        {
            while((ligne = mysql_fetch_row(resultat)) != NULL){
                char Prix[20];
                sprintf(Prix,"%s",ligne[2]);
                string tmp(Prix);
                size_t x = tmp.find(".");
                if (x != string::npos) tmp.replace(x,1,","); // WTF ?!?!?!?
                ajouteArticleTablePanier(atoi(ligne[0]), ligne[1], atof(tmp.c_str()), atoi(ligne[3]));
            }
        }
    }
}

WindowGerant::~WindowGerant()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du stock (ne pas modifier) //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::ajouteArticleTablePanier(int id, const char *article, float prix, int quantite)
{
    char Id[20], Prix[20], Quantite[20];

    sprintf(Id, "%d", id);
    sprintf(Prix, "%.2f", prix);
    sprintf(Quantite, "%d", quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetStock->rowCount();
    nbLignes++;
    ui->tableWidgetStock->setRowCount(nbLignes);
    ui->tableWidgetStock->setRowHeight(nbLignes - 1, 10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Id);
    ui->tableWidgetStock->setItem(nbLignes - 1, 0, item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetStock->setItem(nbLignes - 1, 1, item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetStock->setItem(nbLignes - 1, 2, item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetStock->setItem(nbLignes - 1, 3, item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::videTableStock()
{
    ui->tableWidgetStock->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetStock->selectionModel()->selectedRows();
    if (liste.size() == 0)
        return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_tableWidgetStock_cellClicked(int row, int column)
{
    // cerr << "ligne=" << row << " colonne=" << column << endl;
    ui->lineEditIntitule->setText(ui->tableWidgetStock->item(row, 1)->text());
    ui->lineEditPrix->setText(ui->tableWidgetStock->item(row, 2)->text());
    ui->lineEditStock->setText(ui->tableWidgetStock->item(row, 3)->text());
    idArticleSelectionne = atoi(ui->tableWidgetStock->item(row, 0)->text().toStdString().c_str());
    // cerr << "id = " << idArticleSelectionne << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
float WindowGerant::getPrix()
{
    return atof(ui->lineEditPrix->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getStock()
{
    return atoi(ui->lineEditStock->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char *WindowGerant::getPublicite()
{
    strcpy(publicite, ui->lineEditPublicite->text().toStdString().c_str());
    return publicite;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::closeEvent(QCloseEvent *event)
{
    fprintf(stderr, "(GERANT %d) Clic sur croix de la fenetre\n", getpid());
    // TO DO
    // Deconnexion BD
    mysql_close(connexion);

    // Liberation du semaphore
    // TO DO

    exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonPublicite_clicked()
{
    fprintf(stderr, "(GERANT %d) Clic sur bouton Mettre a jour\n", getpid());
    MESSAGE m;
    m.expediteur = getpid();
    m.type = 1;
    m.requete = NEW_PUB;
    strcpy(m.data4, getPublicite());

    if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0))
    {
        fprintf(stderr, "(GERANT %d) Erreur de msgsnd : ", getpid());
        exit(1);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonModifier_clicked()
{
    fprintf(stderr, "(GERANT %d) Clic sur bouton Modifier\n", getpid());
    // TO DO
    cerr << "Prix  : --"  << getPrix() << "--" << endl;
    cerr << "Stock : --"  << getStock() << "--" << endl;

    MYSQL_ROW ligne;
    char Prix[20];
    sprintf(Prix, "%f", getPrix());
    string tmp(Prix);
    size_t x = tmp.find(",");
    if (x != string::npos)
        tmp.replace(x, 1, ".");

    char Stock[10];
    sprintf(Stock, "%d", getStock());


    fprintf(stderr, "(GERANT %d) Modification en base de données pour id=%d\n", getpid(), idArticleSelectionne);

    sprintf(requete, "update UNIX_FINAL set stock =%s, prix =%s where id=%d", Stock, tmp.c_str(), idArticleSelectionne);
    printf("%s\n", requete);
    if (mysql_query(connexion, requete) != 0)
    {
        // Impossible de mettre a jour la BD. Donc on retourne une erreur au client !
        fprintf(stderr, "(ACCESBD %d) Impossible de mettre à jour la BD !\n", getpid());
    }
    else
    {
        // Quantité OK et tout les tests de la connexion à la BD à fonctionner
        fprintf(stderr, "(ACCESBD %d) Base de données mise à jour !\n", getpid());
        sprintf(requete, "select * from UNIX_FINAL");
        if (mysql_query(connexion, requete) == 0)
        {
            if ((resultat = mysql_store_result(connexion)) == NULL)
            {
                fprintf(stderr, "(ACCESBD %d) Impossible d'obtenir le résultat !\n", getpid());
            }
            else
            {
                videTableStock();
                while((ligne = mysql_fetch_row(resultat)) != NULL){
                    char Prix[20];
                    sprintf(Prix,"%s",ligne[2]);
                    string tmp(Prix);
                    size_t x = tmp.find(".");
                    if (x != string::npos) tmp.replace(x,1,","); // WTF ?!?!?!?
                    ajouteArticleTablePanier(atoi(ligne[0]), ligne[1], atof(tmp.c_str()), atoi(ligne[3]));
                }
            }
        }
        else {
            fprintf(stderr, "Erreur fdp\n");
        }
        
    }
    // Mise a jour table BD
    // TO DO
}
