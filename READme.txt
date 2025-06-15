Commande de compilation : make

Fonctionnalités opérationnelles :
Jusqu’à la version 3, tout fonctionne correctement, notamment la phase de transfert de données avec un mécanisme de perte à fiabilité partielle.
Cependant, à partir de la version 4.1, certains problèmes apparaissent. Le premier se manifeste par un blocage du 3-way handshake : le client répète indéfiniment l’envoi de ses SYN en timeout, tandis que le serveur ne renvoie jamais de SYN-ACK.
Le second problème survient après l’établissement logique de la connexion : la fonction mic_tcp_connect returne prématurément dès la fin du handshake, ce qui rend la phase de négociation du taux de perte complètement inaccessible. En conséquence, aucun PDU de négociation n’est émis, et le serveur reste bloqué dans sa boucle d’attente.

Choix d’implémentation :

-Stop & Wait à fiabilité partielle : Nous avons choisi d’implémenter ce mécanisme à l’aide d’un buffer circulaire. Concrètement, pour chaque groupe de n paquets envoyés, un nombre fixe de pertes est toléré. Au-delà de ce seuil, les paquets perdus sont automatiquement retransmis. Une fois que n paquets ont été traités, le compteur de paquets ainsi que celui des pertes est réinitialisé, et le cycle recommence.

- Négociation du taux de perte : Après l’établissement du handshake, le client envoie un PDU dédié (flag ACK + champ ack_num = taux souhaité) que le serveur boucle en lecture via IP_recv(..., TIMEOUT) jusqu’à ce qu’il le reçoive, puis renvoie un ACK de confirmation avant d’appeler set_loss_rate(), garantissant ainsi une négociation réseau fiable et asynchrone.

- Asynchronisme (thread réseau):  La détection du SYN et de l’ACK final se fait dans process_received_PDU() exécuté par un thread séparé, qui met à jour s.state (SYN_RECEIVED puis ESTABLISHED) ; mic_tcp_accept() et mic_tcp_connect() font ensuite un simple while(s.state!=…) sleep(1), évitant tout blocage direct sur des IP_recv pour le handshake.  


MICTCP-v4.2 apporte deux améliorations clés par rapport à TCP et MICTCP-v2 :

Négociation d’un taux de perte dès le handshake, ce qui réduit les retransmissions inutiles pour les flux tolérants (vidéo, IoT) et améliore le débit et la latence.

Thread réseau asynchrone qui sépare la réception des paquets du code applicatif, évitant les blocages sur send/recv et rendant le serveur plus réactif.
