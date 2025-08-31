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
#include "Login.h"
#include "protocole.h" // contient la cle et la structure d'un message

int idQ,idShm,idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;
int pidPublicite;

void afficheTab();
void connectClient();
void handlerSIGINT(int sig);

int main()
{
  // Armement des signaux
    struct sigaction sig;
    sigfillset(&sig.sa_mask);
    sigdelset(&sig.sa_mask, SIGINT);
    sigdelset(&sig.sa_mask, SIGUSR1);
    sig.sa_handler = handlerSIGINT;
    sig.sa_flags = 0;
    sigaction(SIGINT, &sig, NULL);
  // Creation des ressources

  // Creation de la file de message
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // TO BE CONTINUED
  (idShm = shmget(CLE, 51 * sizeof(char), IPC_CREAT | IPC_EXCL | 0666));
  // Creation du pipe
  // TO DO

  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = getpid();
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite (étape 2)
  system("ps -ef | grep Publicite | grep -v grep | awk '{print $2}' | xargs kill -9");

   if ((pidPublicite = fork()) == -1)
  {
    // Note : Supprimer la file de message.
    exit(1);
  }

  if (pidPublicite == 0)
  {
    if (execl("./Publicite", "Publicite", NULL) == -1)
    {
      exit(1);
    }
  }
  tab->pidPublicite = pidPublicite;


  // Creation du processus AccesBD (étape 4)
  // TO DO

  MESSAGE m;
  MESSAGE reponse;
  while(1)
  {
      int i = 0;

  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  
                      printf("on fait le connect\n");

                      while(i<6)
                      {
                        if (tab->connexions[i].pidFenetre == 0)
                        {
                          tab->connexions[i].pidFenetre = m.expediteur;
                          break;
                        }
                        else i++;
                      }
                      if (i>=6)
                      {
                        
                        reponse.requete = BUSY;
                      }

                      else reponse.requete = CONNECT;
                      reponse.type = m.expediteur;
                      reponse.expediteur = tab->pidServeur;
                      msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0);
                      kill(m.expediteur, SIGUSR1);

                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case DECONNECT :
                    while(i<6)
                    {
                      if (tab->connexions[i].pidFenetre == m.expediteur)
                      { tab->connexions[i].pidFenetre = 0;
                        break;}
                      else i++;
                    }
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;
      case LOGIN :    
                      printf("on fait le login\n");
                      while(i<6)
                      {

                        if (tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          printf("notre pid est trouvé\n");
                          //CREATE
                          if(m.data1 == 1)
                          {
                            printf("enter create\n");
                            if(rechercheUser(m.data2)<0)
                            {
                            printf("recherche ne trouve pas de client donc le crée\n");

                              addUser(m.data2,m.data3);
                              reponse.data1 = 1;
                              strcpy(reponse.data4,"Client créé et connecté");

                            }

                            else
                            {
                              printf("compte impossible à créer\n");
                              reponse.data1 = 0;
                              strcpy(reponse.data4,"Un compte existe deja avec cet identifiant");
                            }
                          }


                        //Login
                          else if(m.data1 == 0)
                          {
                            printf("enter login\n");
                            bool exist;
                            exist = authenticate(m.data2,m.data3);
                            if (exist == true)
                            {
                              printf("authenticate reussi\n");
                              reponse.data1 = 1;
                              strcpy(reponse.data4,"Client Connecté");
                              strcpy(tab->connexions[i].nom, m.data2);
                            }
                            else
                            {
                              printf("authenticate failed\n");
                              reponse.data1 = 0;
                              strcpy(reponse.data4,"Login incorrect");
                            }
                          }
                          break;
                        }

                        else i++;
                      }
                      if (i>=6)
                      {

                        reponse.requete = BUSY;
                      }
                      else 
                      {
                          reponse.requete = LOGIN;
                      }
                      reponse.expediteur = tab->pidServeur;

                      reponse.type = m.expediteur;
                      printf("%d \n", reponse.requete);
                      msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0);
                      kill(m.expediteur, SIGUSR1);
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.data3);
                      break; 

      case LOGOUT :   
                    printf("logout recu\n");
                      while(i<6)
                    {
                      if (tab->connexions[i].pidFenetre == m.expediteur) 
                        {
                          strcpy(tab->connexions[i].nom , "");
                          tab->connexions[i].pidCaddie = 0;
                          break;
                        }
                      else i++;
                    }
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case UPDATE_PUB :  
                      while (i < 6)
                      {
                        if (tab->connexions[i].pidFenetre != 0)
                        {
                          kill(tab->connexions[i].pidFenetre, SIGUSR2);
                        }

                        i++;
                      }
                      break;

      case CONSULT :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);
                      break;

      case PAYER : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}

void handlerSIGINT(int sig)
{
  printf("\nFermeture propre du serveur\n");
  msgctl(idQ, IPC_RMID, NULL);
}

