### Madalina Boboc

# Imonitor

### Organizare

Aceasta este o solutie de monitorizare a conexiunilor remote realizate de useri. Implementarea se foloseste de fisierele interne ale sistemului de operare (WTMP) pentru a culege date. Aceste date sunt ulterior formatate si stocate int-un fisier tip logfile. Program-ul implementeaza o logica interna tip server web astfel ca datele logurilor pot fi accesate usor de catre clienti.

Solutia pe care am gandit-o initial suna in felul urmator:
- un program care sa logeze date despre conexiunile userilor intr-un fisier / sau mai multe fisiere (in functie de data, user, etc)
- clientii care se conecteaza la serverul web pot sa "ceara" un fisier de log (tot asa in functie de data, user)
M-am gandit ca nu exista neaparat o nevoie pentru o conexiune continua la server de tipul live-update pentru fisiere de tip logging, astfel ca un client doar "cere" de la server fisierul de log si il analizeaza ulterior. Daca vrea un update va trebui sa faca din nou o cerere pentru logfile.

Programul are 2 parti:
* partea de colectare de date
* server web asincron

Pentru partea de colectare de date, este monitorizat fisierul `/var/log/wtmp` a sistemului de operare. Acesta contine informatii despre utilizatorii logati si delogati. Fisierul este monitorizat cu ajutorul unei instante de timp inotify si notificata in program de o instanta tip epoll.

Motivatia pentru utilizarea unui server web asincron este in principal faptul ca incepusem implementarea in cadrul altui proiect si mi s-a parut interesata ideea de a integra un astfel de server intr-o solutie de logging. Ce este interesant la acest server e ca foloseste epoll pentru interogarea evenimentelor, API care merge foarte bine cu `inotify` rezultand intr-o solutie scalabila.

Serverul:
* este o masina de stari
* fiecare client care se conecteaza trece prin aceleasi stari si anume:

`STATE_NEW_CONN - s-a conectat un client la server`
`STATE_DATA_RECEIVED - clinetul trimite un request pentru un fisier. Cererea este parsata si se extrage path-ul pentru fisierul cerut, se determina daca exista sau nu`
`STATE_SENDING - in functie de existenta fisierului se trimite antetul de HTTP (200 - OK, 404 - NOT FOUND)`
`STATE_READING - se citesc datele din fisier`
`STATE_SENDING - se trimit datele (aici urmeaza un loop intre reading si sending pana sunt trimise toate datele)`
`STATE_READING - in momentul in care s-au citit file-size date, s-a facut transferul complet si se inchide conexiunea`
`STATE_CONNECTION_CLOSED - am inchis conexiunea si am eliberat resursele`


Fiecare conexiune are o structura de tipul `struct connection` si implicit un descriptor tip eventfd.
```
    struct connection {
    int sockfd; /* socket */
    char recv_buffer[BUFSIZ];
    size_t recv_len;
    char send_buffer[BUFSIZ];
    size_t send_len; /* cat am citit pana acum */
    enum connection_state state;
    char request_path[BUFSIZ];
    int eventfd;
    size_t file_dimension; /* dimensiune fisier */
    io_context_t ctx;
    size_t header_size;
    size_t data_size;
    size_t is_header;
    size_t read_data; /* cat am citit ultima data */
    int fd; /* file descriptor fisier */
};
```

 Operatiile I/O read si send se fac asincron si anume: de fiecare data cand vreau sa citesc din fisier sau sa trimit date pe socket, fac o cerere de operatie io (Linux AIO). Cand operatia e gata, voi fi notificata pe file descriptorul eventfd. Eventfd va veni ca o notificare tip EPOLLIN. Atata timp cat astept un EPOLLIN de pe un eventfd, dezactivez EPOLLOUT pe socket. In rest, sunt detalii de implementare care sunt comentate in cod.


### Implementare
Cu implementarea am ajuns intr-un punct in care programul functioneaza, dar sunt multe lucruri de imbunatatit. Unul dintre ele ar fi formatare logurilor pentru a fi mai "evident" cine s-a logat/delogat. Deocamdata iti dai seama doar dupa PID. Eu il gandisem initial cumva sa fie usor customizabil si sa pot alege pentru ce user vreau log-ul in momentul in care fac cererea de GET, de asta am si evitat sa folosesc zero-copy la transfer ca sa pot procesa datele. Dar asta e de imbunatatit. Sunt destul de multumita cu ce a iesit, ca dovada ca am multe idei de cum as putea sa il customizez in viitor.

### Cum se compilează și cum se rulează?

`./install-script.sh`
`cd imonitor`
`./imonitor`

* scriptul `./install-script.sh` nu merge cu argumentul -C imonitor

Testare server-client:
Din alt terminat se ruleaza comanda:

`wget http://localhost:8888/auth.log`

Bibliografie
* The Linux and UNIX System Programming
* Autotools, 2nd Edition: A Practitioner's Guide to GNU Autoconf, Automake and Libtool
* Resurse din cadrul cursului de SO (ACS)
* https://kkourt.io/blog/2017/10-14-linux-aio.html
* https://medium.com/@copyconstruct/the-method-to-epolls-madness-d9d2d6378642