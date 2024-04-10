Proiectul contine fisierele: -server.c pentru implementarea serverului
                        -subscriber.c pentru implementarea clientilor tcp
                        -helpers.c pentru functiile ajutatoare de lucrat pe 
                        liste si de generat mesaje de eroare si pentru structuri
In helpers.c, pe langa functiile de prelucrare pe liste precum insert_first, insert_last,
delete, find_in_list, free_list, inspirate de pe site-ul
https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
am creat structurile folosite pentru rezolvare:
Client: structura pentru memorarea clientilor conectati, ce contine id-ul acestuia,
    socket-ul de pe care este activ, un camp pentru verificarea activitatii: 0 
    pentru offline, 1 pt online, o lista de topicuri la care este abonat si 
    o lista de mesaje trimise si salvate in timp ce era inactiv. Pentru 
    memorarea clientilor am folosit un vector pentru a facilita accesarea 
    acestora prin intermediul indicilor.
Topic: nod in lista de topicuri a fiecarui client, ce contine numele
    topicului, indicatorul de sf si link la nodul urmator din lista. Pentru
    retinerea topicurilor am ales sa folosesc liste deoarece operatiile de 
    inserare si stergere din lista sunt mai eficiente decat intr-un vector.
Message_udp: structura folosita pentru interpretarea mesajelor primite de la
    un client udp, ce contine campurile de nume topic, tip si continut.
Saved-message: nod in lista de mesaje salvate din cadrul clientului, care 
    contine un camp ce reprezinta mesajul propriu-zis, adresa, portul de 
    la care a fost trimis si link la nodul urmator.


In server.c, am pornit prin crearea socket-ului pentru conexiunea tcp, respectiv 
udp si realizarea bind-urilor pentru acestea. Am creat vectorul de clienti, 
iar pentru a nu limita numarul acestora, am facut realocare de memorie ulterior. 
Am dezactivat algoritmul lui Nagle, preluat de pe site-ul mentionat si in cod,
https://stackoverflow.com/questions/17842406/how-would-one-disable-nagles-algorithm-in-linux.
In while, am abordat 4 cazuri: 
    1) pentru o comanda in server, daca aceasta este exit, am deconectat pe rand
fiecare client din vectorul de clienti, dupa care am inchis socketul. 
    2) pentru o cerere de conexiune pe socket-ul tcp, dupa accept, primesc id-ul 
clientului, verific daca acesta se regaseste in vectorul de clienti. Daca este unic, 
adaug clientul in vector, facand afisarea ceruta si initializandu-i campurile 
specifice. In aceasta situatie tratez cazul de limitare al numarului clientilor, 
respectiv realoc memorie pentru vector. In caz contrar, verific daca clientul 
este online si ii afisez mesajul de "already connected" si inchid noul socket.
Daca este deconectat, il reconectez, adaug socketul in multimea descriptorilor si 
ii trimit mesajele trimise in timp ce el era inactiv, pentru sf-uri de topicuri 
1, care au fost pastrate intr-o lista de tip saved_message, in care, pe langa 
mesaj, am retinut si adresa si portul de la care a fost trimis. Dupa ce trimit
toate mesajele, eliberez lista.
    3) pentru un mesaj de la un client udp, retin atat mesajul, cat si adresa 
si portul prin structura sockaddr_in. Pentru fiecare client din vector, daca 
este abonat la topicul primit in mesaj, sunt 2 cazuri: daca clientul este activ,
ii redirectionez mesajul, cat si adresa si portul de unde a fost primit, iar daca
este inactiv, retin aceste informatii in campul de mesaje din cadrul structurii
clientului, mesaje care urmeaza a fi trimise cand acesta se reconecteaza.
    4) s-a primit un mesaj pe un client tcp: daca acesta are 0 bytes, inseamna 
ca acesta s-a deconectat, caz in care afisez mesajul cerut si inchid socket-ul.
Daca mesajul este subscribe, adaug topicul in lista de topicuri a clientului,
iar daca este unsubscribe il sterg. In acelasi timp, verific comanda sa fie valida.
    In final, inchid socket-ul udp si tcp.

In subscriber.c, deschid conexiunea cu server-ul, trimit id-ul clientului, apoi,
citind de la tastatura, verific daca primesc vreuna din comenzile exit, subscribe
sau unsubscribe si redirectionez catre server. Apoi, verific mesajele primite de 
la server, redirectionate la randul lor de la clientul udp. Apelez de trei ori 
functia de recv, o data pentru mesaj, o data pentru adresa si ultima data pentru 
port. Mesajul primit este prelucrat de functia message_show. Am ales sa fac aceasta
prelucare in client si nu in server deoarece reduce numarul de task-uri pe care 
trebuie sa le faca serverul si il face mai rapid. In functia message_show,
abordez fiecare caz separat: INT, SHORT REAL, FLOAT, STRING, respectand exact 
cerintele din enunt, si aplicand cast-uri specifice fiecarui tip.
