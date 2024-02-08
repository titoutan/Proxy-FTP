//Titouan BRANDO et Alex PELLETIER
//Groupe A
//Pour une raison qui nous est inconnue, notre proxy ne fonctionne pas sur un serveur externe mais fonctionne avec un server ftp local comme vsftpd

#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define FTPPORT "21"                // Numéro de porte pour ftp

void printMemory(const void *ptr, size_t size); // pour le debug
void lireClient(int* ecode, int descSockCOM, char* buffer );
void ecrireClient(int descSockCOM, char* buffer );
void lireServer(int* ecode, int descSockCOM, char* buffer );
void ecrireServer(int descSockCOM, char* buffer );


int main(){
    int h1;                         //partie de l'ip de com
    int h2;                         //partie de l'ip de com
    int h3;                         //partie de l'ip de com
    int h4;                         //partie de l'ip de com
    int p1;                         //partie du port de com
    int p2;                         //partie du port de com
    int port;
    char serverCommAddr[MAXHOSTLEN]; // addresse de com dans un serveur
    char serverCommPort[MAXPORTLEN]; // port de com du server
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    int descSockFTPCTRL;             // Descripteur de socket pour la connexion de controle avec le serveur
    int descSockFTPCOM;              // Descripteur de socket pour la connexion de communication avec le serveur
    int descSockClientData;
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par
				                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN,
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

	len = sizeof(struct sockaddr_storage);
  //Boucle qui permet de gérer des connexion à la suite sans relancer le proxy
  while (true){
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client
    descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
    if (descSockCOM == -1){
       perror("Erreur accept\n");
       exit(6);
    }
    //fork qui permet de gérer plusieurs connexions en même temps, le fils exécute son rôle de proxy et le père attend une nouvelle connexion
    switch(fork()) {
      case -1: perror("erreur fork");exit(8);break;
      case 0: {
        // Echange de données avec le client connecté

        /*****
        * Testez de mettre 220 devant BLABLABLA ...
        * **/
        strcpy(buffer, "220 Connecté au proxy, entrez votre username@server\n");
        ecrireClient(descSockCOM, buffer);
        /*******
        *
        * A vous de continuer !
        *
        * *****/
        //Lecture de l'identifiant et du server
        lireClient(&ecode, descSockCOM, buffer);

        //Décomposition
        char login[50], ftpServerName[50];

        memset(login,'\0',sizeof(login));
        memset(ftpServerName,'\0',sizeof(ftpServerName));

        sscanf(buffer, "%48[^@]@%48s",login,ftpServerName);

        sprintf(login,"%s\n",login);

        //Connexion au serveur ftp
        ecode = connect2Server(ftpServerName, FTPPORT, &descSockFTPCTRL);
        if (ecode== -1) {perror("Pb connexion serveur ftp\n");exit(6);}

        //Lecture de la réponse
        lireServer(&ecode, descSockFTPCTRL, buffer);

        //Envoi de l'indentifiant
        ecrireServer(descSockFTPCTRL, login);

        //Lecture de la réponse
        lireServer(&ecode, descSockFTPCTRL, buffer);

        ecrireClient(descSockCOM, buffer);

        //Lecture du mdp
        lireClient(&ecode, descSockCOM, buffer);

        ecrireServer(descSockFTPCTRL, buffer);

        //Lecture de la réponse
        lireServer(&ecode, descSockFTPCTRL, buffer);

        ecrireClient(descSockCOM, buffer);

        //Lecture de la demande SYST
        lireClient(&ecode, descSockCOM, buffer);

        ecrireServer(descSockFTPCTRL, buffer);

        //Lecture de la réponse
        lireServer(&ecode, descSockFTPCTRL, buffer);
        ecrireClient(descSockCOM, buffer);

        //Lecture des demandes, tant que le client n'envoie pas QUIT
        for (lireClient(&ecode,descSockCOM,buffer);
        strncmp(buffer,"QUIT",4);
        lireClient(&ecode,descSockCOM,buffer)) {
          //si le client nous envoie PORT comme prévu dans le sujet
          if (strncmp(buffer,"PORT", 4) == 0){
            sscanf(buffer,"PORT %u,%u,%u,%u,%u,%u",&h1,&h2,&h3,&h4,&p1,&p2);
            sprintf(serverCommAddr,"%u.%u.%u.%u",h1,h2,h3,h4);
            p2 += p1<<8;
            sprintf(serverCommPort,"%u",p2);

            //Connexion au client pour les données
            ecode = connect2Server(serverCommAddr, serverCommPort, &descSockClientData);
            if (ecode== -1) {perror("Pb connexion client \n");exit(6);}
            ecrireClient(descSockCOM,"200 PORT command successful.\n");
          }
          //Notre proxy fonctionne aussi avec PASV bien que cela ne soit pas demandé dans le sujet (extension)
          else if (strncmp(buffer,"PASV", 4) == 0) {
            //Ouverture de la socket de data
            // Initialisation de la socket de RDV IPv4/TCP
            descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
            if (descSockRDV == -1) {
                 perror("Erreur création socket RDV\n");
                 exit(2);
            }
            // Publication de la socket au niveau du système
            // Assignation d'une adresse IP et un numéro de port
            // Mise à zéro de hints
            memset(&hints, 0, sizeof(hints));
            // Initialisation de hints
            hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
            hints.ai_socktype = SOCK_STREAM;  // TCP
            hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par
                                      // la fonction getaddrinfo

             // Récupération des informations du serveur
             ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
             if (ecode) {
                 fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
                 exit(1);
             }
             // Publication de la socket
             ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
             if (ecode == -1) {
                 perror("Erreur liaison de la socket de RDV");
                 exit(3);
             }
             // Nous n'avons plus besoin de cette liste chainée addrinfo
             freeaddrinfo(res);

             // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
             len=sizeof(struct sockaddr_storage);
             ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
             if (ecode == -1)
             {
                 perror("SERVEUR: getsockname");
                 exit(4);
             }
             ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN,
                                 serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
             if (ecode != 0) {
                     fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
                     exit(4);
             }

             sscanf(serverAddr,"%u.%u.%u.%u",&h1,&h2,&h3,&h4);
             sscanf(serverPort,"%u",&port);
             p1 = port >>8;
             p2 = (port<<8)>>8;

             memset(buffer,'\0',MAXBUFFERLEN);
             sprintf(buffer,"227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).\n",h1,h2,h3,h4,p1,p2);

             ecrireClient(descSockCOM, buffer);

             // Definition de la taille du tampon contenant les demandes de connexion
             ecode = listen(descSockRDV, LISTENLEN);
             if (ecode == -1) {
                 perror("Erreur initialisation buffer d'écoute");
                 exit(5);
             }

            len = sizeof(struct sockaddr_storage);


            // Attente connexion du client
            // Lorsque demande de connexion, creation d'une socket de communication avec le client
            descSockClientData = accept(descSockRDV, (struct sockaddr *) &from, &len);
            if (descSockClientData == -1){
             perror("Erreur accept\n");
             exit(6);
            }


          }
          //Si commande du client n'est ni PORT ni PASV, donc qu'elle ne nécessite pas de port d'échange de données
          else {
            ecrireServer(descSockFTPCTRL,buffer);
            lireServer(&ecode,descSockFTPCTRL,buffer);
            ecrireClient(descSockCOM,buffer);
            //On retourne au début de la boucle car ce qui suit n'est que l'échange des données
            continue;
          }
          //Connexion au serveur pour le transfert de données
          ecrireServer(descSockFTPCTRL, "PASV\n");

          //Lecture de la réponse
          lireServer(&ecode,descSockFTPCTRL,buffer);

          memset(&serverCommAddr,'\0',sizeof(serverCommAddr));
          memset(&serverCommPort,'\0',sizeof(serverCommPort));

          sscanf(buffer,"227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).",&h1,&h2,&h3,&h4,&p1,&p2);
          sprintf(serverCommAddr,"%u.%u.%u.%u",h1,h2,h3,h4);
          p2 += p1<<8;
          sprintf(serverCommPort,"%u",p2);

          //Connexion au serveur ftp pour communication
          ecode = connect2Server(serverCommAddr, serverCommPort, &descSockFTPCOM);
          if (ecode== -1) {perror("Pb connexion serveur ftp\n");exit(6);}

          lireClient(&ecode,descSockCOM, buffer);
          ecrireServer(descSockFTPCTRL, buffer);
          lireServer(&ecode,descSockFTPCTRL,buffer);
          ecrireClient(descSockCOM,buffer);
          //Lecture de la réponse
          do {
            lireServer(&ecode, descSockFTPCOM, buffer);
            ecrireClient(descSockClientData, buffer);
          } while (ecode == MAXBUFFERLEN-1 && buffer[ecode-1] != '\n');

          lireServer(&ecode,descSockFTPCTRL,buffer);
          ecrireClient(descSockCOM,buffer);
          close(descSockClientData);
          close(descSockFTPCOM);
      }
    }
  }
  //Fermeture de la connexion
  close(descSockFTPCTRL);
  close(descSockCOM);
  }
  close(descSockRDV);
}

void lireClient(int* ecode, int descSockCOM, char* buffer ) {
  *ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
  if (*ecode == -1) {perror("Problème de lecture\n"); exit(3);}
  buffer[*ecode] = '\0';
  printf("<--C %s",buffer);
}
void lireServer(int* ecode, int descSockCOM, char* buffer ) {
  *ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
  if (*ecode == -1) {perror("Problème de lecture\n"); exit(3);}
  buffer[*ecode] = '\0';
  printf("<--S %s",buffer);
}

void ecrireClient(int descSockCOM, char* buffer ) {
  write(descSockCOM, buffer, strlen(buffer));
  printf("-->C %s",buffer);
}
void ecrireServer( int descSockCOM, char* buffer ) {
  write(descSockCOM, buffer, strlen(buffer));
  printf("-->S %s",buffer);
}

void printMemory(const void *ptr, size_t size) {
    const unsigned char *byte = ptr;

    for (size_t i = 0; i < size; i++) {
        printf("%02X ", byte[i]);
    }

    printf("\n");
}
