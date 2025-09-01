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
#include "Login.h"

#define FICHIER_CLIENTS "clients.dat"

typedef struct
{
  char nom[20];
  char motDePasse[20];
} CLIENT;

int idQ, idShm, idSem, pidServeur = getpid();
int fdPipe[2];
TAB_CONNEXIONS *tab;
sigjmp_buf contexte; // Variable pour stocker le contexte du programme et permettre un siglongjmp

void afficheTab();
void connecterClient(int pidClient);
void deconnecterClient(int pidClient);
void loggerClient(int pidClient, int nouveauClient, char *nomClient, char *motDePasse);
void deloggerClient(int pidClient);
void envoyerMessageConsultCaddie(MESSAGE m);
void mettreAJourPublicite();
int clientConnecte(int pidClient);
void envoyerMessage(MESSAGE& m);
void libererRessourcesEtQuitter(int codeSortie);
void handlerSIGINT(int signal);
void handlerSIGCHLD(int signal);

int main()
{
  int pidPublicite;

  // Armement des signaux
  // TO DO
  // Armement de SIGINT
      struct sigaction sig, sigC;
    sigfillset(&sig.sa_mask);
    sigdelset(&sig.sa_mask, SIGINT);
    sig.sa_handler = handlerSIGINT;
    sig.sa_flags = 0;

  if (sigaction(SIGINT, &sig, NULL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de sigaction\n", pidServeur);
    exit(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Signal SIGINT arme\n", pidServeur);

  // Armement de SIGCHLD
  sigfillset(&sigC.sa_mask);
  sigdelset(&sigC.sa_mask, SIGCHLD);
  sigC.sa_handler = handlerSIGCHLD;
  sigC.sa_flags = 0;

  if (sigaction(SIGCHLD, &sigC, NULL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de sigaction\n", pidServeur);
    exit(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Signal SIGCHLD arme\n", pidServeur);

  // Creation des ressources
  // Creation de la file de message
  if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0666)) == -1)  // CLE definie dans protocole.h
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgget()\n", pidServeur);
    exit(1);
  }
  printf("(SERVEUR %d) (SUCCESS) File de message cree\n", pidServeur);

  // Creation de la memoire partagee
  if ((idShm = shmget(CLE, 51 * sizeof(char), IPC_CREAT | IPC_EXCL | 0666)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de shmget()\n", pidServeur);
    libererRessourcesEtQuitter(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Memoire partagee cree\n", pidServeur);

  // Creation du pipe
  // TO DO

  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i = 0; i < 6; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom, "");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = pidServeur;
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite (étape 2)
  if ((pidPublicite = fork()) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de fork()\n", pidServeur);
    fprintf(stderr, "(SERVEUR %d) (WARNING) La publicite ne fonctionnera pas sur les processus clients\n");
  }

  if (pidPublicite == 0) // Code pour le fils
  {
    if (execl("./Publicite", "Publicite", NULL) == -1)
    {
      fprintf(stderr, "(PUBLICITE %d) (ERROR) Erreur de execl()\n", getpid());
      fprintf(stderr, "(PUBLICITE %d) (WARNING) La publicite ne fonctionnera pas sur les processus clients\n", getpid());
      exit(1);
    }
  }
  tab->pidPublicite = pidPublicite;

  // Creation du processus AccesBD (étape 4)
  // TO DO

  int retour; // Stocke la valeur de retour de siglongjmp
  MESSAGE m;
  MESSAGE reponse;
  bool afficher = true; // Sert pour ne pas afficher la table apres chaque reception de requete UPDATE_PUB

  // Initialise le retour en cas de saut
  if ((retour = sigsetjmp(contexte, 1)) != 0)
    fprintf(stderr, "\n(SERVEUR %d) (INFO) Retour du saut %d\n", pidServeur, retour);

  afficheTab();

  while(1)
  {
  	// fprintf(stderr,"(SERVEUR %d) (PROCESS) Attente d'une requete...\n", pidServeur);
    if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), 1, 0) == -1)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgrcv", pidServeur);
      msgctl(idQ, IPC_RMID, NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :    // TO DO
                        fprintf(stderr,"(SERVEUR %d) (SUCCESS) Requete CONNECT reçue de %d\n",pidServeur,m.expediteur);
                        connecterClient(m.expediteur);
                        break;

      case DECONNECT :  // TO DO
                        fprintf(stderr,"(SERVEUR %d) (SUCCESS) Requete DECONNECT reçue de %d\n",pidServeur,m.expediteur);
                        deconnecterClient(m.expediteur);
                        break;
      case LOGIN :      // TO DO
                        fprintf(stderr,"(SERVEUR %d) (SUCCESS) Requete LOGIN reçue de %d : --%d--%s--%s--\n",pidServeur,m.expediteur,m.data1,m.data2,m.data3);
                        loggerClient(m.expediteur, m.data1, m.data2, m.data3);
                        break; 

      case LOGOUT :     // TO DO
                        fprintf(stderr,"(SERVEUR %d) (SUCCESS) Requete LOGOUT reçue de %d\n",pidServeur,m.expediteur);
                        deloggerClient(m.expediteur);
                        break;

      case UPDATE_PUB : // TO DO
                        // fprintf(stderr,"(SERVEUR %d) (SUCCESS) Requete UPDATE_PUB reçue de %d\n",pidServeur,m.expediteur); // Note : A enlever
                        mettreAJourPublicite();
                        afficher = false;
                        break;

      case CONSULT :    // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete CONSULT reçue de %d\n", pidServeur, m.expediteur);
                        envoyerMessageConsultCaddie(m); // On copie le message car on reutilise ses champs sans modifier le message
                        break;

      case ACHAT :      // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete ACHAT reçue de %d\n", pidServeur, m.expediteur);
                        break;

      case CADDIE :     // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete CADDIE reçue de %d\n", pidServeur, m.expediteur);
                        break;

      case CANCEL :     // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete CANCEL reçue de %d\n", pidServeur, m.expediteur);
                        break;

      case CANCEL_ALL : // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete CANCEL_ALL reçue de %d\n", pidServeur, m.expediteur);
                        break;

      case PAYER :      // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete PAYER reçue de %d\n", pidServeur, m.expediteur);
                        break;

      case NEW_PUB :    // TO DO
                        fprintf(stderr, "(SERVEUR %d) (SUCCESS) Requete NEW_PUB reçue de %d\n", pidServeur, m.expediteur);
                        break;
    }

    if (afficher == true) afficheTab();

    afficher = true;
  }
}

// Affiche la table des connexions
void afficheTab()
{
  fprintf(stderr,"\nPid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}

// Ajoute le PID de la fenetre du client dans la table des connexions
void connecterClient(int pidClient)
{
  int i = 0;
  MESSAGE m;

  // Verifie si le serveur n'est pas plein
  while (i < 6 && tab->connexions[i].pidFenetre != 0)
  {
    i++;
  }

  // Si le serveur est plein, envoyer une requete BUSY au client
  if (i >= 6)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le serveur n'a plus assez de place pour accepter le client\n", pidServeur);

    // Note : Nécessaire d'envoyer un message BUSY ?
    m.type = pidClient;
    m.requete = BUSY;
    m.expediteur = pidServeur;

    envoyerMessage(m);

    if (kill(pidClient, SIGUSR1) == -1)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    }
    return;
  }

  tab->connexions[i].pidFenetre = pidClient;
}

// Supprime le PID du client de la table des connexions
void deconnecterClient(int pidClient)
{
  int posClient = 0;

  // Verifie si le client est connecte
  if ((posClient = clientConnecte(pidClient)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le client a deconnecter n'a pas pu etre trouve dans la table des connexions\n", pidServeur);
    return;
  }

  tab->connexions[posClient].pidFenetre = 0;
}

// Note : Trop long ? Et que faire si le client n'est pas connecté ?
// Log le client au serveur ou envoie un message d'erreur explicatif au client, et cree un processus Caddie pour le client
void loggerClient(int pidClient, int nouveauClient, char *identifiant, char *password)
{
  int fd, retour, posClient, pidCaddie;
  bool connexionReussie = false;
  MESSAGE m;
  m.type = pidClient;
  m.expediteur = pidServeur;
  m.requete = LOGIN;

  if ((posClient = clientConnecte(pidClient)) == -1)
  {
    strcpy(m.data4, "Vous n'êtes pas connecté au serveur. Veuillez redémarrer votre application.");

    envoyerMessage(m);

    if (kill(pidClient, SIGUSR1) == -1)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    }

    return;
  }

  // Le client veut créer un compte
  if (nouveauClient)
  {
    

    if (rechercheUser(identifiant) > 0)
    {
      strcpy(m.data4, "Ce compte existe déjà. Veuillez entrer un autre nom d'utilisateur.");
    }
    else
    {
      addUser(identifiant, password);
      connexionReussie = true;
      strcpy(m.data4, "Votre compte a été créé avec succès. Bienvenue !");
    }
  }
  
  // Le client veut se connecter
  if (!nouveauClient)
  {
    if (authenticate(identifiant,password) ==true) // Le client existe
    {
        connexionReussie = true;
        strcpy(m.data4, "Re-bonjour cher client !");
    }
    else if (authenticate(identifiant,password) == false)
    {
      strcpy(m.data4, "Echec d'authentification.");
    }
    else // Erreur du serveur
    {
      fprintf(stderr, "Erreur d'ouverture du fichier %s\n", FICHIER_CLIENTS);
      strcpy(m.data4, "Erreur du serveur. Veuillez réessayer.");
    }
  
  }

  if (connexionReussie == true)
  {
    if ((pidCaddie = fork()) == -1)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de fork()\n", pidServeur);
        strcpy(m.data4, "Le serveur n'a pas pu créer un caddie. Veuillez réessayer.");
      connexionReussie = false;
    }
    
    if (pidCaddie == 0)
    {
      if (execl("./Caddie", "Caddie", NULL) == -1)
      {
        fprintf(stderr, "(CADDIE) (ERROR) Erreur de execl()\n");
        printf("(CADDIE) (WARNING) Le caddie du client %d ne fonctionnera pas\n", pidClient);
        exit(1);
      }
    }
  }

  // S'il n'y a pas d'erreur, on ajoute le client et son Caddie a la table des connexions
  if (connexionReussie == true)
  {
    strcpy(tab->connexions[posClient].nom, identifiant);
    tab->connexions[posClient].pidCaddie = pidCaddie;
    m.data1 = 1;
  }
  else // Sinon, on ne le le fait pas.
  {
    m.data1 = 0;
  }

  // On envoie le message de reponse au client
  envoyerMessage(m);

  // On previent le client qu'il a un message
  if (kill(pidClient, SIGUSR1) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
  }
}

// Recherche la position du client et le supprime de la table des connexions
void deloggerClient(int pidClient)
{
  int posClient;

  // Recherche la position du client.
  if ((posClient = clientConnecte(pidClient)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le client %d n'a pas ete trouve dans la table de connexion. Impossible de delogger le client.\n", pidServeur, pidClient);
    return;
  }

  // Supprime le nom du client de la table
  strcpy(tab->connexions[posClient].nom, "");

  // Envoie un message LOGOUT au processus Caddie.
  MESSAGE m;
  m.type = tab->connexions[posClient].pidCaddie;
  m.requete = LOGOUT;
  m.expediteur = pidServeur;

  envoyerMessage(m);
}

void envoyerMessageConsultCaddie(MESSAGE m)
{
  int posClient;

  // On recupere la position du client dans la table des connexions
  if ((posClient = clientConnecte(m.expediteur)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le client %d n'a pas ete trouve dans la table de connexion. Impossible de d'envoyer un message CONSULT au Caddie.\n", pidServeur, m.expediteur);
    return;
  }

  // On initialise et envoie le message au processus Caddie
  m.type = tab->connexions[posClient].pidCaddie;
  
  envoyerMessage(m);
}

// Envoie un signal SIGUSR2 a chaque client pour mettre a jour leur publicite
void mettreAJourPublicite()
{
  int i = 0;

  // Parcourt la liste des clients connectes et leur envoie un signal SIGUSR2
  while (i < 6)
  {
    if (tab->connexions[i].pidFenetre != 0)
    {
      if (kill(tab->connexions[i].pidFenetre, SIGUSR2) == -1) fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    }

    i++;
  }
}

int clientConnecte(int pidClient)
{
  int i = 0;

  while (i < 6 && tab->connexions[i].pidFenetre != pidClient)
  {
    printf("%d\n", i);
    i++;
  }

  if (i >= 6)
    return -1;
  else
    return i;
}

void envoyerMessage(MESSAGE& m)
{
  if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgsnd()\n", pidServeur);
    return;
  }
}

// Supprime le processus Publicite, la file de message et la memoire partagee
void libererRessourcesEtQuitter(int codeSortie)
{
  // Tuer le processus Publicite
  if (kill(tab->pidPublicite, SIGKILL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill(). Veuillez tuer le processus Publicite manuellement.\n", pidServeur);
    codeSortie = 1;
  }
  fprintf(stderr, "(SERVEUR %d) (SUCCESS) Processus Publicite tue.\n", pidServeur);

  // Suppression de la file de message
  if (msgctl(idQ, IPC_RMID, NULL) == -1) // Si la file de message n'a pas ete supprime
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgctl(). Veuillez supprimer la file de message manuellement.\n", pidServeur);
    codeSortie = 1;
  }
  else // Si la file de message a ete supprime
  {
    printf("(SERVEUR %d) (SUCCESS) File de message supprimee\n", pidServeur);
  }

  // Demande de suppression de la memoire partagee
  if (shmctl(idShm, IPC_RMID, NULL) ==-1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de shmctl. Veuillez supprimer la memoire partagee manuellement.\n", pidServeur);
    codeSortie = 1;
  }
  else
  {
    printf("(SERVEUR %d) (SUCCESS) Memoire partagee supprimee\n", pidServeur);
  }

  exit(codeSortie);
}

// Handler du signal SIGINT
void handlerSIGINT(int signal)
{
  fprintf(stderr, "\n(SERVEUR %d) (SUCCESS) Signal %d recu\n", pidServeur, signal);

  libererRessourcesEtQuitter(0);
}

void handlerSIGCHLD(int signal)
{
  fprintf(stderr, "(SERVEUR %d) (SUCCESS) Signal %d recu\n", pidServeur, signal);

  int pidCaddie, status, posCaddie = 0;

  // Attend la fin d'un processus
  while (((pidCaddie = wait(&status))) != -1)
  {
    fprintf(stderr, "(SERVEUR %d) (INFO) Processus Caddie %d termine\n", pidServeur, pidCaddie);

    // Recherche le Caddie dans la table des connexions
    while (posCaddie < 6 && tab->connexions[posCaddie].pidCaddie != pidCaddie)
    {
      posCaddie++;
    }

    if (posCaddie >= 6)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Le processus Caddie %d a supprimer de la table des connexions n'a pas ete trouve\n", pidServeur, pidCaddie);
      return;
    }

    // Supprime le Caddie de la table des connexions
    tab->connexions[posCaddie].pidCaddie = 0;

    // Revient a l'endroit voulu
    siglongjmp(contexte, 1); // 1 est defini arbitrairement
  }
}
