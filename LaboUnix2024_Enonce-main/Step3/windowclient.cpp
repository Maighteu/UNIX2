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

#include <QCloseEvent>

extern WindowClient *w;

int idQ, idShm, pidClient = getpid();
bool logged;
char* pShm;
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
  labelsTablePanier << "Article" << "Prix à l'unité" << "Quantité";
  ui->tableWidgetPanier->setHorizontalHeaderLabels(labelsTablePanier);
  ui->tableWidgetPanier->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableWidgetPanier->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableWidgetPanier->horizontalHeader()->setVisible(true);
  ui->tableWidgetPanier->horizontalHeader()->setDefaultSectionSize(160);
  ui->tableWidgetPanier->horizontalHeader()->setStretchLastSection(true);
  ui->tableWidgetPanier->verticalHeader()->setVisible(false);
  ui->tableWidgetPanier->horizontalHeader()->setStyleSheet("background-color: lightyellow");

  // Recuperation de l'identifiant de la file de messages
  // TO DO
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de recuperer l'identifiant de la file de message.", pidClient);
    exit(1);
  }
  fprintf(stderr, "(CLIENT %d) (SUCCESS) id de la file de messages recupere\n", pidClient);

  // Recuperation de l'identifiant de la mémoire partagée
  // TO DO
  if ((idShm = shmget(CLE, 0, 0)) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de recuperer l'identifiant de la memoire partagee.\n", pidClient);
    exit(1);
  }
  fprintf(stderr, "(CLIENT %d) (SUCCESS) id de la memoire partagee recupere\n", pidClient);

  // Attachement à la mémoire partagée
  // TO DO
  if ((pShm = (char*)shmat(idShm, NULL, 0)) == (char*)-1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de s'attacher à la mémoire partagée.", pidClient);
    exit(1);
  }
  fprintf(stderr, "(CLIENT %d) (SUCCESS) Attachement a la memoire partagee reussi", pidClient);

  // Armement des signaux
  // TO DO
  struct sigaction sSIGUSR1;
  sSIGUSR1.sa_handler = handlerSIGUSR1;
  sigemptyset(&sSIGUSR1.sa_mask);
  sSIGUSR1.sa_flags = 0;

  if (sigaction(SIGUSR1, &sSIGUSR1, NULL) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur d'armement du signal SIGUSR1.", pidClient);
    exit(1);
  }
  printf("(CLIENT %d) (SUCCESS) Signal SIGUSR1 arme\n", pidClient);

  struct sigaction sSIGUSR2;
  sSIGUSR2.sa_handler = handlerSIGUSR2;
  sigemptyset(&sSIGUSR2.sa_mask);
  sSIGUSR2.sa_flags = 0;

  if (sigaction(SIGUSR2, &sSIGUSR2, NULL) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur d'armement du signal SIGUSR2.", pidClient);
    exit(1);
  }
  printf("(CLIENT %d) (SUCCESS) Signal SIGUSR2 arme\n", pidClient);

  // Envoi d'une requete de connexion au serveur
  // TO DO
  MESSAGE requete;  // Structure pour la requête

  requete.type = 1;               // Type pour le serveur
  requete.expediteur = pidClient;  // PID du client
  requete.requete = CONNECT;      // Macro définie dans "protocole.h"

  if (msgsnd(idQ, &requete, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Impossible d'envoyer la requête CONNECT au serveur.", pidClient);
    exit(1);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi d'une requete CONNECT au serveur reussi\n", pidClient);

  // Exemples à supprimer
  // setPublicite("Promotions sur les concombres !!!");
  // setArticle("pommes",5.53,18,"pommes.jpg");
  // ajouteArticleTablePanier("cerises",8.96,2);
}

WindowClient::~WindowClient()
{
  delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setImage(const char* image)
{
  // Met à jour l'image
  char cheminComplet[80];
  sprintf(cheminComplet,"%s%s",REPERTOIRE_IMAGES,image);
  QLabel* label = new QLabel();
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
  if (ui->checkBoxNouveauClient->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setArticle(const char* intitule,float prix,int stock,const char* image)
{
  ui->lineEditArticle->setText(intitule);
  if (prix >= 0.0)
  {
    char Prix[20];
    sprintf(Prix,"%.2f",prix);
    ui->lineEditPrixUnitaire->setText(Prix);
  }
  else ui->lineEditPrixUnitaire->clear();
  if (stock >= 0)
  {
    char Stock[20];
    sprintf(Stock,"%d",stock);
    ui->lineEditStock->setText(Stock);
  }
  else ui->lineEditStock->clear();
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
    sprintf(Total,"%.2f",total);
    ui->lineEditTotal->setText(Total);
  }
  else ui->lineEditTotal->clear();
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

  setArticle("",-1.0,-1,"");

  w->videTablePanier();
  totalCaddie = 0.0;
  w->setTotal(-1.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du panier (ne pas modifier) /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteArticleTablePanier(const char* article,float prix,int quantite)
{
  char Prix[20],Quantite[20];

  sprintf(Prix,"%.2f",prix);
  sprintf(Quantite,"%d",quantite);

  // Ajout possible
  int nbLignes = ui->tableWidgetPanier->rowCount();
  nbLignes++;
  ui->tableWidgetPanier->setRowCount(nbLignes);
  ui->tableWidgetPanier->setRowHeight(nbLignes-1,10);

  QTableWidgetItem *item = new QTableWidgetItem;
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  item->setTextAlignment(Qt::AlignCenter);
  item->setText(article);
  ui->tableWidgetPanier->setItem(nbLignes-1,0,item);

  item = new QTableWidgetItem;
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  item->setTextAlignment(Qt::AlignCenter);
  item->setText(Prix);
  ui->tableWidgetPanier->setItem(nbLignes-1,1,item);

  item = new QTableWidgetItem;
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  item->setTextAlignment(Qt::AlignCenter);
  item->setText(Quantite);
  ui->tableWidgetPanier->setItem(nbLignes-1,2,item);
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
  if (liste.size() == 0) return -1;
  QModelIndex index = liste.at(0);
  int indice = index.row();
  return indice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue (ne pas modifier ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
  QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
  QMessageBox::critical(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
  MESSAGE message;
  message.type = 1;
  message.expediteur = pidClient;

  // Envoi d'une requete LOGOUT si logged
  if (logged)
  {
    message.requete = LOGOUT;

    if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      dialogueErreur("Erreur", "Impossible d'envoyer la requête de déconnexion du compte."); // Note : Est-ce qu'il faut avertir le client ?
      fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
      return;
    }
    printf("(CLIENT %d) (SUCCESS) Logout reussi\n", pidClient);
  }

  // Envoi d'une requete DECONNECT au serveur
  message.requete = DECONNECT;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête de déconnexion du serveur."); // Note : Est-ce qu'il faut avertir le client ?
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
    return;
  }
  printf("(CLIENT %d) (SUCCESS) Deconnexion reussie\n", pidClient);

  // Note : Si le serveur ne sait pas qu'on se déconnecte, le client se déconnecte quand même ou pas ?
  event->accept();
  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
  // Envoie d'une requête LOGIN au serveur
  // TO DO

  MESSAGE message;

  message.type = 1;
  message.requete = LOGIN;
  message.expediteur = pidClient;

  message.data1 = isNouveauClientChecked(); // 1 = nouveau client, 0 = client existant
  if (strcmp(getNom(), "") == 0 || strcmp(getMotDePasse(), "") == 0) // Si au moins un des champs est vide
  {
    dialogueErreur("Attention", "Le nom d'utilisateur et le mot de passe doivent être remplis !");
    return;
  }

  strcpy(message.data2, getNom());          // Nom du client
  strcpy(message.data3, getMotDePasse());   // Mot de passe

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête pour se connecter.");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogout_clicked()
{
  MESSAGE message;
  // Envoi d'une requete CANCEL_ALL au serveur (au cas où le panier n'est pas vide)
  // TO DO
  message.type = 1;
  message.requete = CANCEL_ALL;
  message.expediteur = pidClient;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête pour annuler le panier.");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi de la requete d'annulation du panier reussie\n", pidClient);

  // Envoi d'une requete de logout au serveur
  // TO DO
  message.type = 1;
  message.requete = LOGOUT;
  message.expediteur = pidClient;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête LOGOUT");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi de la requete LOGOUT reussi\n", pidClient);

  logged = false;
  logoutOK();
  dialogueMessage("Déconnexion", "Vous êtes maitenant déconnecté.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSuivant_clicked()
{
  // TO DO (étape 3)
  // Envoi d'une requete CONSULT au serveur
  MESSAGE message;
  message.type = 1;
  message.requete = CONSULT;
  message.expediteur = pidClient;
  message.data1 = articleEnCours.id + 1;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête CONSULT");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi de la requete CONSULT reussi\n", pidClient);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPrecedent_clicked()
{
  // TO DO (étape 3)
  // Envoi d'une requete CONSULT au serveur
  MESSAGE message;
  message.type = 1;
  message.requete = CONSULT;
  message.expediteur = pidClient;

  if (articleEnCours.id <= 1) return;

  message.data1 = articleEnCours.id - 1;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête CONSULT");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi de la requete CONSULT reussi\n", pidClient);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonAcheter_clicked()
{
    // TO DO (étape 5)
    // Envoi d'une requete ACHAT au serveur
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSupprimer_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL au serveur

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonViderPanier_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL_ALL au serveur

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPayer_clicked()
{
    // TO DO (étape 7)
    // Envoi d'une requete PAYER au serveur

    char tmp[100];
    sprintf(tmp,"Merci pour votre paiement de %.2f ! Votre commande sera livrée tout prochainement.",totalCaddie);
    dialogueMessage("Payer...",tmp);

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int signal)
{
  MESSAGE m;
  
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), pidClient, 0) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgrcv\n", pidClient);
    return;
  }
  
  switch(m.requete)
  {
    case LOGIN :
      printf("(CLIENT %d) (SUCCESS) Requete LOGIN recue : --%d--%s--\n", pidClient, m.data1, m.data4);
      if (m.data1 == 1) // Login réussi
      {
        logged = true;
        w->loginOK();
        w->dialogueMessage("Connexion réussie", m.data4);

        articleEnCours.id = 1;
        m.type = 1;
        m.requete = CONSULT;
        m.expediteur = pidClient;
        m.data1 = articleEnCours.id;

        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
        {
          w->dialogueErreur("Erreur", "Impossible d'envoyer la requête CONSULT");
          fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
        }
        printf("(CLIENT %d) (SUCCESS) Envoi de la requete CONSULT reussi\n", pidClient);
      }
      else // Login échoué
      {
        w->dialogueErreur("Echec de connexion", m.data4);
      }
      break;

    case CONSULT : 
      // TO DO (étape 3)
      printf("(CLIENT %d) (SUCCESS) Requete CONSULT recue : --%d--%s--%d--%s--%f--\n", pidClient, m.data1, m.data2, atoi(m.data3), m.data4, m.data5);

      // Note : Convertir les float de la base de donnees en float du C++ ?
      // Si la requete a echouee
      if (m.data1 == -1) return;

      // On affiche l'article.
      articleEnCours.id = m.data1;
      w->setArticle(m.data2, m.data5, stoi(m.data3), m.data4);
      break;

    case ACHAT :
      // TO DO (étape 5)
      break;

    case CADDIE :
      // TO DO (étape 5)
      break;

    case TIME_OUT :
      // TO DO (étape 6)
      break;

    case BUSY :
      // TO DO (étape 7)
      printf("(CLIENT %d) (SUCCESS) Requete BUSY recue\n", pidClient);
      w->dialogueErreur("Serveur indisponible", "Le serveur est momentanément occupé et ne peut pas traiter la requête.");
      break;

    default :
      fprintf(stderr, "(CLIENT %d) Requête inconnue reçue\n", pidClient);
      break;
  }
}

void handlerSIGUSR2(int signal)
{
  fprintf(stderr, "(CLIENT %d) (INFO) Signal SIGUSR2 recu\n", pidClient);
  w->setPublicite(pShm);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
