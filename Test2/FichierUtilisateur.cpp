#include "FichierUtilisateur.h"

using namespace std;
int fd;

int estPresent(const char* nom)
{
  UTILISATEUR user;

  int i = 0;
  if ((fd=open(FICHIER_UTILISATEURS, O_RDONLY))==-1) return -1;
  while ((read(fd, &user, sizeof(UTILISATEUR)) == sizeof(UTILISATEUR)))
  {
    if (strcmp(user.nom, nom) == 0)
    {
      close (fd);
      return i;
    }
    i++;

  }
  close (fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int Hash(const char* motDePasse)
{
  UTILISATEUR user;

  int result = 0;
  int i;
  for (i=0;i<strlen(motDePasse);i++)
  {
  result = (motDePasse[i] * (i+1)) + result;
  }
  result = (result %97);
  return result;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  UTILISATEUR user;
  if ((fd=open(FICHIER_UTILISATEURS, O_WRONLY|O_APPEND))==-1)
  {
  ((fd = open(FICHIER_UTILISATEURS, O_WRONLY |O_APPEND | O_CREAT | O_EXCL, 0777))==-1);
  }
  strcpy(user.nom, nom);
  user.hash= Hash(motDePasse);
  write (fd, &user, sizeof(UTILISATEUR));
  close (fd);
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  UTILISATEUR user;

  if ((fd=open(FICHIER_UTILISATEURS, O_RDONLY))==-1) return -1;
  lseek(fd,pos * sizeof(UTILISATEUR),SEEK_SET);
  read(fd,&user, sizeof(UTILISATEUR));
  if ((user.hash) == Hash(motDePasse))
  {
    return 1;
  }
  close(fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  UTILISATEUR user;

  int j = 0;
  if ((fd=open(FICHIER_UTILISATEURS, O_RDONLY))==-1) return -1;
  while ((read(fd, &user, sizeof(UTILISATEUR)) == sizeof(UTILISATEUR)))
  {
    vecteur[j] =  user;
    j ++;

  }

  close(fd);
  return j;
}
