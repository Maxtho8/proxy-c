#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include  <unistd.h>

#define SERVADDR "localhost"        // Definition de l'adresse IP d'ecoute
#define SERVPORT "5000"             // Definition du port d'ecoute, si 0 port choisi dynamiquement
#define LISTENLEN 5                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 50
#define MAXPORTLEN 6

int main(){
    bool isConnected;                // booleen indiquant que l'on est connecte ou non
	int ecode;                       // Code retour des fonctions
	char serverAddr[MAXHOSTLEN];     // Adresse du serveur
	char serverPort[MAXPORTLEN];     // Port du server
	int descSockRDV;                 // Descripteur de socket de rendez-vous
	int descSockComClient;           // Descripteur de socket de communication
    int descSockComServeur;          // Descripteur de socket de communication
    int dataClient;                  // Descripteur de socket de donnees client
    int dataServeur;                 // Descripteur de socket de donnees serveur

    struct addrinfo *res;                                 // Resultat de la fonction getaddrinfo client
	struct addrinfo hintsServer;                          // Contrôle la fonction getaddrinfo serveur
    struct addrinfo hintsClient;                          // Contrôle la fonction getaddrinfo client
    struct addrinfo hintsDataClient;                      // Socket de transfert de donnees client
    struct addrinfo hintsDataServeur;                     // Socket de transfert de donnees serveur
    struct addrinfo *resDataClientPtr, *resDataClient;    // Resultat de la fonction getaddrinfo data client
    struct addrinfo *resDataServeurPtr, *resDataServeur;  // Resultat de la fonction getaddrinfo data serveur
	struct addrinfo *resServ,*resServPtr;                 // Resultat de la fonction getaddrinfo serveur
	struct sockaddr_storage myinfo;                       // Informations sur la connexion de RDV
	struct sockaddr_storage from;                         // Informations sur le client connecte
	socklen_t len;                                        // Variable utilisee pour stocker les longueurs des structures de socket
	char buffer[MAXBUFFERLEN];                            // Tampon de communication entre le client et le serveur
    

	// Publication de la socket au niveau du système
	// Assignation d'une adresse IP et un numero de port
	// Mise a zero de hintsServer
	memset(&hintsServer, 0, sizeof(hintsServer));
	// Initailisation de hintsServer
	hintsServer.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
	hintsServer.ai_socktype = SOCK_STREAM;  // TCP
	hintsServer.ai_family = AF_UNSPEC;      // les adresses IPv4 et IPv6 seront presentees par 
				                         // la fonction getaddrinfo

	// Initailisation de hintsClient
	memset(&hintsClient, 0, sizeof(hintsClient));
	hintsClient.ai_socktype = SOCK_STREAM;
	hintsClient.ai_family = AF_INET;


	// Recuperation des informations du serveur
	ecode = getaddrinfo(SERVADDR, SERVPORT, &hintsServer, &res);
	if (ecode) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	//Creation de la socket IPv4/TCP
	descSockRDV = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (descSockRDV == -1) {
		perror("Erreur creation socket");
		exit(4);
	}

	// Publication de la socket
	ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
	if (ecode == -1) {
		perror("Erreur liaison de la socket de RDV");
		exit(3);
	}
	// Nous n'avons plus besoin de cette liste chainee addrinfo
	freeaddrinfo(res);
	// Recupperation du nom de la machine et du numero de port pour affichage a l'ecran

	len=sizeof(struct sockaddr_storage);
	ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
	if (ecode == -1) { perror("SERVEUR: getsockname"); exit(4);}

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
	if (ecode == -1) { perror("Erreur initialisation buffer d'ecoute"); exit(5);}

	len = sizeof(struct sockaddr_storage);
	// Attente connexion du client
	// Lorsque demande de connexion, creation d'une socket de communication avec le client
	descSockComClient = accept(descSockRDV, (struct sockaddr *) &from, &len);
	if (descSockComClient == -1){ perror("Erreur accept\n"); exit(6);}

	// Echange de donnees avec le client connecte
	strcpy(buffer, "220 Proxy OK\n");

	write(descSockComClient, buffer, strlen(buffer));

	ecode = read(descSockComClient, buffer, MAXBUFFERLEN-1);
	if (ecode==-1){	perror("Erreur de lecture"); exit(8);}

	buffer[ecode] = '\0';
	printf("USER (recu du client) : %s\n", buffer);

	//Decoupage de la chaine stockee dans buffer 
    char login[45];
    char serverName[45];
    sscanf(buffer, "%[^@]@%s", login, serverName);
    strcat(login, "\n");
    printf("Login : %s\n", login);
    printf("Server Name : %s\n", serverName);
    // Set buffer to 0
    memset(buffer, 0, MAXBUFFERLEN-1);

	//ouvrir connexion avec serveur ftp
    //Recuperation des informations sur le serveur
	ecode = getaddrinfo(serverName,serverPort,&hintsServer,&resServ);
	if (ecode){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resServPtr = resServ;

    isConnected = false;
	while(!isConnected && resServPtr!=NULL){
		//Creation de la socket IPv4/TCP
		descSockComServeur = socket(resServPtr->ai_family, resServPtr->ai_socktype, resServPtr->ai_protocol);
		if (descSockComServeur == -1) {
			perror("Erreur creation socket");
			exit(2);
		}
  
  		//Connexion au serveur
		ecode = connect(descSockComServeur, resServPtr->ai_addr, resServPtr->ai_addrlen);
		if (ecode == -1) {
			resServPtr = resServPtr->ai_next;    		
			close(descSockComServeur);	
		}
		// On a pu se connecte
		else isConnected = true;
	}
	freeaddrinfo(resServ);
	if (!isConnected){
		perror("Connexion 1 impossible");
		exit(2);
	}


	//Echange de donnees avec le serveur
	ecode = read(descSockComServeur, buffer, MAXBUFFERLEN);
	if (ecode == -1) { perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);
    memset(buffer, 0, MAXBUFFERLEN-1);

	//envoyer au serveur (sur socket descSockCtrlServ) le login
	ecode = write(descSockComServeur, login, strlen(login));
	if(ecode == -1){perror("Probleme d'ecriture cote serveur\n");exit(3);	}

	//lire reponse du serveur (descSockCtrlServ) 
	ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);
    
	// Evoyer reponse du serveur au client (descSockComClient)
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
	
    // Lire PASS XXX depuis le client
    ecode = read(descSockComClient, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){ perror("Problème de lecture cote client"); exit(3);}
    buffer[ecode] = '\0';
    printf("MESSAGE RECU DU CLIENT: %s\n", buffer); 

    // Envoyer PASS XXX au serveur
    ecode = write(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote serveur\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
	
    // Lire reponse du serveur
	ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);

    // Envoyer au client reponse du serveur
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

	// Lire commande SYST depuis client
    ecode = read(descSockComClient, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){ perror("Problème de lecture cote client"); exit(3);}
    buffer[ecode] = '\0';
    printf("MESSAGE RECU DU CLIENT: %s\n", buffer);

	// Envoyer au serveur la commande du client
	ecode = write(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote serveur\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
    
    // Lire reponse serveur
    ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);

	// Envoyer reponse du serveur au client
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

	// Lire commande depuis client
    ecode = read(descSockComClient, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){ perror("Problème de lecture cote client"); exit(3);}
    buffer[ecode] = '\0';
    printf("MESSAGE RECU DU CLIENT: %s\n", buffer);

	// Passage en mode actif avec le client, passif avec le serveur
	// Lire depuis client commande PORT n1,n2...n6
    int n1, n2, n3, n4, n5, n6;
    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
    memset(buffer, 0, MAXBUFFERLEN-1);

    // Extraire et construire adresse IP socket data client, par concatenation de n1, n2, n3, n4
	char adrDataClient[16];
    char portDataClient[10];
    sprintf(adrDataClient,"%d.%d.%d.%d",n1,n2,n3,n4);
    
    // Calculer num port (n5*256)+n6
    sprintf(portDataClient,"%d",n5*256+n6);

	// Preparation socket data client 
    memset(&hintsDataClient ,0, sizeof(hintsDataClient));
    hintsDataClient.ai_socktype = SOCK_STREAM;
	hintsDataClient.ai_family = AF_INET;

	//Recuperation des informations sur le serveur
	ecode = getaddrinfo(adrDataClient, portDataClient, &hintsDataClient, &resDataClientPtr);
	if (ecode){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resDataClientPtr = resDataClient;

	while(!isConnected && resDataClientPtr!=NULL){

		//Creation de la socket IPv4/TCP
		dataClient = socket(resDataClientPtr->ai_family, resDataClientPtr->ai_socktype, resDataClientPtr->ai_protocol);
		if (dataClient == -1) {	perror("Erreur creation socket"); exit(2);}
  
  		//Connexion au serveur
		ecode = connect(dataClient, resDataClientPtr->ai_addr, resDataClientPtr->ai_addrlen);
		if (ecode == -1) {
			resDataClientPtr = resDataClientPtr->ai_next;    		
			close(dataClient);	
		}
		// On a pu se connecte
		else isConnected = true;
	}
	freeaddrinfo(resDataClient);
	if (!isConnected){ perror("Connexion 2 impossible"); exit(2);}

	// Passer en mode passif avec le serveur
	// Envoyer sur socket controle commande PASV\n
    char passiv[5] = "PASV\n";
    strncat(buffer, passiv, strlen(passiv));
    
	// Envoyer au serveur la commande du client
	ecode = write(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote serveur\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
    
    // Lire depuis le serveur "227 Entering Passive Mode" (n1,n2,n3,n4,n5,n6)
    ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);


    // Extraire et construire adresse IP socket data serveur, par concatenation de n1, n2, n3, n4	
    // Set les n a zero
    n1,n2,n3,n4,n5,n6=0;
    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
    memset(buffer, 0, MAXBUFFERLEN-1);

    // Extraire et construire adresse IP socket data client, par concatenation de n1, n2, n3, n4
	char adrDataServeur[16];
    char portDataServeur[10];
    sprintf(adrDataServeur,"%d.%d.%d.%d",n1,n2,n3,n4);
    
    // Calculer num port (n5*256)+n6
    sprintf(portDataServeur,"%d",n5*256+n6);

	// Preparation socket data client 
    memset(&hintsDataServeur, 0, sizeof(hintsDataServeur));
    hintsDataServeur.ai_socktype = SOCK_STREAM;
	hintsDataServeur.ai_family = AF_INET;

	//Recuperation des informations sur le serveur
	ecode = getaddrinfo(adrDataServeur, portDataServeur, &hintsDataServeur, &resDataServeurPtr);
	if (ecode){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resDataServeurPtr = resDataServeur;

    isConnected = false;
	while(!isConnected && resDataServeurPtr!=NULL){

		//Creation de la socket IPv4/TCP
		dataServeur = socket(resDataServeurPtr->ai_family, resDataServeurPtr->ai_socktype, resDataServeurPtr->ai_protocol);
		if (dataServeur == -1) { perror("Erreur creation socket"); exit(2);}
  
  		//Connexion au serveur
		ecode = connect(dataClient, resDataServeurPtr->ai_addr, resDataServeurPtr->ai_addrlen);
		if (ecode == -1) {
			resDataServeurPtr = resDataServeurPtr->ai_next;    		
			close(dataServeur);	
		}
		// On a pu se connecte
		else isConnected = true;
	}
	freeaddrinfo(resDataServeur);
	if (!isConnected){ perror("Connexion 3 impossible"); exit(2);}

	// Envoyer au vrai client le message (ctrl) 200 Port Succesfull
    char msg200[25] = "200 PORT Successfull\n";
    strncat(buffer,msg200,strlen(msg200));

    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

	// Lire depuis le client (ctrl) LIST
    ecode = read(descSockComClient, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){ perror("Problème de lecture cote client"); exit(3);}
    buffer[ecode] = '\0';

	// Envoyer au vrai serveur (ctrl)
	ecode = write(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote serveur\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

    // Lire reponse serveur
    ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: %s\n",buffer);

	// Envoyer reponse du serveur au client
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

	// Lire sur la data du serveur (ecode = read(dataServeur))
    ecode = read(dataServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';

    // Boucle afin de recevoir les donnees du serveur
    while(ecode!=0){
		ecode=write(dataClient, buffer, strlen(buffer));
		memset(buffer, 0, MAXBUFFERLEN-1);
		ecode=read(dataServeur,buffer,MAXBUFFERLEN-1);
		if(ecode==-1){
			perror("probleme de lecture");
			exit(3);
		}
        memset(buffer, 0, MAXBUFFERLEN-1);
	}

    // Fermer socket de data (client et serveur)
	close(dataClient);
    close(dataServeur);

    // Lire sur ctrl serveur message 226 (Fin de transmission)
    ecode = read(descSockComServeur, buffer, MAXBUFFERLEN-1);
	if(ecode == -1){ perror("Probleme de lecture cote serveur\n"); exit(3);}
	buffer[ecode] = '\0';

	// Envoyer au client message du serveur
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);

    // Envoyer au serveur message quit
    char quit[5] = "QUIT\n";
    strncat(buffer, quit, strlen(quit));
    ecode = write(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
	 
	// Lire la reponse serveur (au quit)
    ecode = read(descSockComServeur, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    
    // Envoyer reponse du serveur au client
    ecode = write(descSockComClient, buffer, strlen(buffer));
    if(ecode == -1){ perror("Probleme d'ecriture cote client\n"); exit(3);}
    memset(buffer, 0, MAXBUFFERLEN-1);
	
	// Fermeture de la connexion
	close(descSockComClient);
	close(descSockComServeur);
    close(descSockRDV);
}