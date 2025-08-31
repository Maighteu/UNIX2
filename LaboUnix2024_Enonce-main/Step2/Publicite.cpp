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
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm, pidPub = getpid();
char *pShm;
void handlerSIGUSR1(int sig);
int fd;

int main()
{
  // Armement des signaux
  // TO DO

  // Masquage des signaux
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask,SIGUSR1);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(PUBLICITE) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  idShm = shmget(CLE, 0, 0);

  // Attachement à la mémoire partagée
  pShm = (char*)shmat(idShm,NULL,0);

  // Mise en place de la publicité en mémoire partagée
  char pub[51];
  strcpy(pub,"Bienvenue sur le site du Maraicher en ligne !");
  for (int i=0 ; i<=50 ; i++) pShm[i] = ' ';
  pShm[50] = '\0';
  int indDebut = 25 - strlen(pub)/2;
  for (int i=0 ; i<strlen(pub) ; i++) pShm[indDebut + i] = pub[i];
    printf("%s \n", pub);
  while(1)
  {
    MESSAGE msg;
    msg.type = 1;
    msg.expediteur = pidPub;
    msg.requete = UPDATE_PUB;
    msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0);
    sleep(1); 

    char c = pShm[0];
    for (int i = 0; i<50;i++)
    {
      pShm[i] = pShm[i+1];
    }
    pShm[49] = c;
    pShm[50] = '\0';

  }
}

void handlerSIGUSR1(int sig)
{
  fprintf(stderr,"(PUBLICITE %d) Nouvelle publicite !\n",getpid());

  // Lecture message NEW_PUB

  // Mise en place de la publicité en mémoire partagée
}
