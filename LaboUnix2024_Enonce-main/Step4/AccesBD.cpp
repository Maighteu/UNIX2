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

int idQ, fdRpipe, pidAccesBD = getpid();
MYSQL* connexion;
void envoyerRequeteBD(MESSAGE& m);
void envoyerMessage(MESSAGE& m);
void closing();


int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(ACCESBD %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(ACCESBD) Erreur de msgget");
    exit(1);
  }

  // Récupération descripteur lecture du pipe
   fdRpipe = atoi(argv[1]);

  // Connexion à la base de donnée
    connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL)
  {
    fprintf(stderr,"(ACCESBD %d) (ERROR)acces bd echoue\n",getpid());
    exit(1);  
  }
    fprintf(stderr,"(ACCESBD %d) (SUCCESS)acces bd Reussi\n",getpid());

  MESSAGE m;

  while(1)
  {
    // Lecture d'une requete sur le pipe
    if (read(fdRpipe, &m, sizeof(MESSAGE)) == 0)
    {
      closing();
    }

    switch(m.requete)
    {
      case CONSULT :  
                      fprintf(stderr,"(ACCESBD %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD
                      envoyerRequeteBD(m);
                      // Preparation de la reponse

                      // Envoi de la reponse au bon caddie
                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Finalisation et envoi de la reponse
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Mise à jour du stock en BD
                      break;

    }
  }
}

void envoyerRequeteBD(MESSAGE& m)
{
  // Construction et exécution de la requête
  m.requete = CONSULT;
  m.type = m.expediteur;
  m.expediteur = pidAccesBD;

  char requete[256];

  sprintf(requete, "select * from UNIX_FINAL where id = %d;", m.data1);
  
  if (mysql_query(connexion, requete) != 0)
  {
    fprintf(stderr,"(ACCESBD %d) (ERROR)envoi requete bd echoue\n",getpid());
    return;
  }

  // Affichage du Result Set
  MYSQL_RES *ResultSet;

  if ((ResultSet = mysql_store_result(connexion)) == NULL) // Si la base de donnees n'a pas envoye de resultat
  {
    fprintf(stderr,"(ACCESBD %d) (ERROR)retour bd echoue\n",getpid());
    return;
  }

  MYSQL_ROW resultat;

  if ((resultat = mysql_fetch_row(ResultSet)) != NULL)
  {
    m.data1 = atoi(resultat[0]);
    strcpy(m.data2, resultat[1]);
    strcpy(m.data3, resultat[3]);
    strcpy(m.data4, resultat[4]);
    m.data5 = atof(resultat[2]);
  }
  else
  {
    m.data1 = -1;
  }

  envoyerMessage(m);
}
void envoyerMessage(MESSAGE& m)
{
  switch (m.requete)
  {
    case CONSULT:
      fprintf(stderr, "(ACCESBD %d) (PROCESS) Envoi d'une reponse CONSULT a %d : --%d--\n", pidAccesBD, m.type, m.data1);
      break;
    
    default:
    printf("requete autre que consult send\n");
      return;
  }
  msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0);
}

void closing()
{
  if (close(fdRpipe) == -1)
  {
    fprintf(stderr,"(ACCESBD %d) (ERROR)erreur fermeture pipe\n",getpid());
  }
    fprintf(stderr,"(ACCESBD %d) (SUCCESS)Pipe ferme\n",getpid());

  mysql_close(connexion);
  exit(0);
}