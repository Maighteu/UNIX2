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
int pidCaddie = getpid();

MYSQL* connexion;

void selectionnerArticleBD(int idArticle, int pidClient);
void envoyerRequeteBD(int idArticle);
void envoyerMessage(MESSAGE& m);
void closing(int pidServeur);
void handlerSIGALRM(int sig);

int main(int argc, char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_SETMASK, &mask,NULL);

  // Armement des signaux
  // TO DO

  // Recuperation de l'identifiant de la file de messages
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de msgget", pidCaddie);
    exit(1);
  }
  fprintf(stderr, "(CADDIE %d) (SUCCESS) id de la file de messages recupere\n", pidCaddie);

  

  MESSAGE m;
  MESSAGE reponse;
  
  char requete[200];
  char newUser[20];
  MYSQL_RES  *resultat;
  MYSQL_ROW  Tuple;

  // Récupération descripteur écriture du pipe
  fdWpipe = atoi(argv[1]);

  while(1)
  {
    // fprintf(stderr, "(CADDIE %d) (PROCESS) Attente d'une requete...\n", pidCaddie);
    if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), pidCaddie, 0) == -1)
    {
      fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de msgrcv", pidCaddie);
      exit(1);
    }

    switch(m.requete)
    {
      case LOGIN :    // TO DO
                            pidClient = m.expediteur;

                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete LOGIN reçue de %d\n", pidCaddie, m.expediteur);
                      break;

      case LOGOUT :   // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete LOGOUT reçue de %d\n", pidCaddie, m.expediteur);
                      closing(m.expediteur);
                      break;

      case CONSULT :  // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete CONSULT reçue de %d : --%d--\n", pidCaddie, m.expediteur, m.data1);
                      envoyerRequeteBD(m.data1);
                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete ACHAT reçue de %d\n", pidCaddie, m.expediteur);

                      // on transfert la requete à AccesBD
                      
                      // on attend la réponse venant de AccesBD
                        
                      // Envoi de la reponse au client

                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete CADDIE reçue de %d\n", pidCaddie, m.expediteur);
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete CANCEL reçue de %d\n", pidCaddie, m.expediteur);

                      // on transmet la requete à AccesBD

                      // Suppression de l'aricle du panier
                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete CANCEL_ALL reçue de %d\n", pidCaddie, m.expediteur);

                      // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier

                      // On vide le panier
                      break;

      case PAYER :    // TO DO
                      fprintf(stderr,"(CADDIE %d) (SUCCESS) Requete PAYER reçue de %d\n", pidCaddie, m.expediteur);

                      // On vide le panier
                      break;
    }
  }
}

void selectionnerArticleBD(int idArticle, int pidClient)
{
  MESSAGE reponse;
  reponse.type = pidClient;
  reponse.requete = CONSULT;
  reponse.expediteur = pidCaddie;

  // Construction et exécution de la requête
  char requete[256];

  sprintf(requete, "select * from UNIX_FINAL where id = %d;", idArticle);
  
  if (mysql_query(connexion, requete) != 0) // Si la requete n'a pas fonctionne
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de mysql_query : %s\n", pidCaddie, mysql_error(connexion));
    return;
  }

  // Affichage du Result Set
  MYSQL_RES *ResultSet;

  if ((ResultSet = mysql_store_result(connexion)) == NULL) // Si la base de donnees n'a pas envoye de resultat
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de mysql_store_result : %s\n", pidCaddie, mysql_error(connexion));
    return;
  }

  MYSQL_ROW resultat;
  // Encodage du resultat dans le message
  // Note : mysql_fetch_row necessaire ?
  if ((resultat = mysql_fetch_row(ResultSet)) != NULL)
  {
    reponse.data1 = atoi(resultat[0]);
    strcpy(reponse.data2, resultat[1]);
    strcpy(reponse.data3, resultat[3]);
    strcpy(reponse.data4, resultat[4]);
    reponse.data5 = atof(resultat[2]);

    envoyerMessage(reponse);
  }
}

void envoyerMessage(MESSAGE& m)
{
  switch (m.requete)
  {
    case CONSULT:
      printf("(CADDIE %d) (PROCESS) Envoi d'une reponse CONSULT a %d : --%d--%s--\n", pidCaddie, m.type, m.data1, m.data2, m.data3, m.data4, m.data5);
      break;
    
    default:
      printf("(CADDIE %d) (ERROR) Envoi d'une reponse de type non prevu. Abandon de l'envoi...\n", pidCaddie);
      return;
  }

  if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de msgsnd() ici\n", pidCaddie);
    return;
  }

  if (kill(m.type, SIGUSR1) == -1)
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de kill()\n", pidCaddie);
  }
}

void closing(int pidServeur)
{
  // Fermer la connexion a la base de donnees
  mysql_close(connexion);

  // Envoie d'un signal SIGCHLD au serveur
  if (kill(pidServeur, SIGCHLD) == -1) fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de kill()\n", pidCaddie);

  exit(0);
}

void envoyerRequeteBD(int idArticle)
{
  int retour;

  MESSAGE m;
  m.type = pidCaddie;
  m.requete = CONSULT;
  m.expediteur = pidCaddie;
  m.data1 = idArticle;

  retour = write(fdWpipe, &m, sizeof(MESSAGE));

  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), pidCaddie, 0) == -1)
  {
    fprintf(stderr, "(CADDIE %d) (ERROR) Erreur de msgrcv()\n", pidCaddie);
    exit(1);
  }
  fprintf(stderr,"(CADDIE %d) (INFO) Requete CONSULT reçue de %d : --%d--\n", pidCaddie, m.expediteur, m.data1);

  if (m.data1 == -1) return;

  m.type = pidClient;
  m.expediteur = pidCaddie;

  envoyerMessage(m);
}


void handlerSIGALRM(int sig)
{
  fprintf(stderr,"(CADDIE %d) (INFO) Time Out !!!\n", pidCaddie);

  // Annulation du caddie et mise à jour de la BD
  // On envoie a AccesBD autant de requetes CANCEL qu'il y a d'articles dans le panier

  // Envoi d'un Time Out au client (s'il existe toujours)
         
  exit(0);
}
