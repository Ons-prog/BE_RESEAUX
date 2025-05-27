#include <mictcp.h>
#include <api/mictcp_core.h>

#define SOCK_FD 5
#define MICTCP_PORT 9000
//Variable globale 
mic_tcp_sock s;
//variable globale pour mecannisme de reprise de perte
int current_seq_num = 0; 

//Nous allons créer un buffer circulaire de taille N (% tolerable 20% = 1 image sur 5)
//Le numéro du paquet envoyé au sein du buffer 
static int paquet=0;
//nombre de perte 
int perte=0; 
//Le N
const int buffer=5;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1; 

   if(s.state!=CLOSED)
    return -1;

   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(20);

   //creer une structure mic_tcp_sock
   s.fd=SOCK_FD;
   s.state=IDLE;
   result=s.fd;
   
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
   if(socket!=SOCK_FD){
    return -1; //On manipule qu'un socket donc le fd doit etre  SOCK_FD
   }

   s.local_addr=addr;
   return 0;

}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) //addresse remote=addr
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    s.remote_addr=addr; 
    /*
    if(socket!=SOCK_FD){
        return -1; //On manipule qu'un socket donc le fd doit etre  SOCK_FD
    }

    //On stocke la addr_remote en l'associant au socket identifié par int socket.
   

    //Construire et envoyer le PDU SYN (initiateur de connexion)
    mic_tcp_pdu syn_pdu;
    syn_pdu.header.syn=1;
    syn_pdu.header.ack=0;
    syn_pdu.header.fin=0;
    syn_pdu.header.source_port=s.addr.port;
    syn_pdu.header.dest_port=s.remote_addr.port;
    int syn_sent;
    if((syn_sent=IP_send(syn_pdu,addr))==-1)
        return -1;

    //Attendre la réponse SYN-ACK du serveur
    mic_tcp_pdu syn_ack_pdu;
    int syn_ack_recv;
    if ((syn_ack_recv = IP_recv(&syn_ack_pdu,&s.local_addr,&s.remote_addr,TIMEOUT))==-1)
        return -1;

    //Envoyer le PDU ACK final
    mic_tcp_pdu ack_pdu;
    ack_pdu.header.ack=1;
    syn_pdu.header.source_port=s.local_addr.port;
    ack_pdu.header.dest_port=s.remote_addr.port;
    int ack_sent=IP_send(ack_pdu,addr);
    if(ack_sent==-1)
        return -1;

    s.state=CONNECTED;
    */
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    //Construire le pdu
    mic_tcp_pdu pdu_message;
    pdu_message.payload.data = mesg;
    pdu_message.payload.data = malloc(mesg_size); //buffer interne
    if (!pdu_message.payload.data) return -1;
    memcpy(pdu_message.payload.data, mesg, mesg_size);
    pdu_message.payload.size = mesg_size;
    pdu_message.header.source_port =s.local_addr.port;
    pdu_message.header.dest_port=s.remote_addr.port;
        //ajouter numero de sequence du paquet
    pdu_message.header.seq_num =current_seq_num;
    //autre champ
    pdu_message.header.syn=0;
    pdu_message.header.ack=0;
    pdu_message.header.fin=0;
    s.local_addr.ip_addr.addr_size=0;

    //On envoie le pdu et on attend ACK
    int sent_data;
    mic_tcp_pdu ack_pdu;
    ack_pdu.payload.size=0;
    int ack_recv;
    int timeout=3;
    int max_loss = (buffer * 20) / 100;
 

    //On incrémente le numéro du paquet au sein du buffer circulaire pour le mécanise de reprise de perte à fiabilité partielle
    paquet=(paquet+1)%(buffer+1);
    //Si on revient au début du buffer circulaire on remet a le nombre de perte
    if(paquet==0){
        perte=0;
    }

    do
    {
        
        //1) envoyer pdu destinataire
        if((sent_data=IP_send(pdu_message,s.remote_addr.ip_addr))==-1){ 
            free(pdu_message.payload.data);
            return -1;
        }
        
        //2) attendre ack
        if ((ack_recv = IP_recv(&ack_pdu,&s.local_addr.ip_addr,&s.remote_addr.ip_addr,timeout))==-1) {
            // timeout ou erreur : on renvoie si le nombre de pertes est tolérable
            
            if ((perte+1) <= max_loss) { 
            
                perte++;
                current_seq_num=1-current_seq_num;
                //si le nb perte / nb paquet < 20% dans le buffer circulaire alors on ne renvoie pas
                break;
            }

        }
       
    } while ((ack_pdu.header.ack!=1 || ack_pdu.header.ack_num!=current_seq_num) );
    
    //Alterner le numero de séquence
    current_seq_num=1-current_seq_num;

    free(pdu_message.payload.data);
    return mesg_size;

}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload payload;
    payload.data=mesg;
    payload.size=max_mesg_size;
    int effectivement_ecrites = app_buffer_get(payload);
    if (effectivement_ecrites<0)
        return -1;
    return effectivement_ecrites;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    
    if(socket<0)
        return -1;

    s.state=CLOSED;
    return 0;

}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    int ack_send;
    
    
    /*verifier que le port destination dans le header du PDU est bien le port d’un socket existant
localement*/
    if (pdu.header.dest_port != s.local_addr.port) {
        perror("[MICTCP] Erreur DEST_PORT\n"); 
    }
    
   

    // on verifie si le PDU recu est un PDU de données
    if (pdu.header.ack==0 && pdu.header.syn==0 && pdu.header.fin==0 && pdu.payload.size!=0){
        
       
        if (pdu.header.seq_num==current_seq_num){  
            
            app_buffer_put(pdu.payload);
            current_seq_num=1-current_seq_num;
            
        }
    }


    //renvoyer un Ack
    mic_tcp_pdu ack;
    ack.header.syn=0;
    ack.header.ack=1;
    ack.header.fin=0;
    ack.payload.size=0; //Le ack ne contient pas de données dans payload
    ack.header.ack_num=pdu.header.seq_num;
    ack.header.source_port=s.local_addr.port;
    ack.header.dest_port=pdu.header.source_port;

    //Envoyer Le ack au client
    if((ack_send=IP_send(ack,remote_addr))==-1){ 
            perror("[MICTCP] Erreur IP_send(ACK)\n"); 
    }

}
