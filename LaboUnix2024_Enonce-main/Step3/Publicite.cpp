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

int idQ, idShm, pidPublicite = getpid();
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
  sigdelset(&mask, SIGUSR1);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  // Recuperation de l'identifiant de la file de messages
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    fprintf(stderr, "(PUBLICITE %d) (ERROR) Erreur de msgget()", pidPublicite);
    exit(1);
  }
  fprintf(stderr,"(PUBLICITE %d) (SUCCESS) id de la file de messages recupere\n", pidPublicite);

  // Recuperation de l'identifiant de la mémoire partagée
  if ((idShm = shmget(CLE, 0, 0)) == -1)
  {
    fprintf(stderr, "(PUBLICITE %d) (ERROR) Erreur de shmget()", pidPublicite);
    exit(1);
  }
  fprintf(stderr, "(PUBLICITE %d) (SUCCESS) id de la memoire partagee recupere\n", pidPublicite);

  // Attachement a la memoire partagee
  if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
    fprintf(stderr, "(PUBLICITE %d) (ERROR) Erreur de shmat()", pidPublicite);
    exit(1);
  }
  fprintf(stderr, "(PUBLICITE %d) (SUCCESS) Processus attache a la memoire partagee\n", pidPublicite);

  // Mise en place de la publicité en mémoire partagée
  char pub[51];
  strcpy(pub, "Bienvenue sur le site du Maraicher en ligne !");

  for (int i = 0; i < 51; i++) pShm[i] = ' ';
  pShm[50] = '\0';
  int indDebut = 25 - strlen(pub) / 2;
  for (int i = 0; i < strlen(pub); i++) pShm[indDebut + i] = pub[i];

  while(1)
  {
    // Envoi d'une requete UPDATE_PUB au serveur
    MESSAGE message;
    message.type = 1;
    message.expediteur = pidPublicite;
    message.requete = UPDATE_PUB;

    if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      fprintf(stderr, "(PUBLICITE %d) (ERROR) Erreur de msgsnd()\n", pidPublicite);
    // else
    //   fprintf(stderr, "(PUBLICITE %d) (SUCCESS) Requete UPDATE_PUB envoyee\n", pidPublicite);

    sleep(1); 

    // Decalage vers la gauche
    char caractere1 = pShm[0];
    for (int i = 0; i < 49; i++) pShm[i] = pShm[i + 1];
    pShm[49] = caractere1;
  }
}

void handlerSIGUSR1(int sig)
{
  fprintf(stderr, "(PUBLICITE %d) (INFO) Nouvelle publicite !\n", pidPublicite);

  // Lecture message NEW_PUB

  // Mise en place de la publicité en mémoire partagée
}
