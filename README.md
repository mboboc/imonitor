# imonitor

> Some people, when confronted with a problem think "I know, I'll use a configuration file". Now they have their clients problems.


Program that monitors remote connections done by a user.

The program needs to use:

* **inotify** for monitoring events
* **autotools** for project config
* **event queue** for info logging
* **client-server** to monitor many remote devices

* **log file** identify the user account and type of connection
* **readme**

--> types of connections: ssh, sftp, ftp

Archive name: <Nume_Prenume>.tar.gz
For compilling use: `install-script.sh`.

To see users logged in on a station:
`who -u -H`

Display list of users and their activities:
`w`

terminals can be:
* hardware terminals (tty-teletype) using the serial port/USB
* pseudo terminals (pts) using a terminal emulator

`tty`
= terminam = text input/output environment
- teletypewriter
- any serial port on UNIX/Linux
- tty is a regular terminal device
- direct connections --> keyboard, mouse or serial devices

Get a list of open terminals:
`ps aux | grep tty`

`pts`
= pseudo terminal slave
- slave part of a pty
- pty = pseudo terminal device, terminal device that is emulated by other program (ssh, xterm, screen)
- ssh or telnet connections


Idee:
- de fiecare data cand un user se conecteaza se face un fisier ceva in so, daca reusesc sa monitorizez cand se intampla asta pot sa vad cand un utilizator se conecteaza la o statie

dinamically directories --> /dev/pts
--> reflects the state of the running system

-----------------------------------------------------------------

ssh
--> de file is created in /dev/pts everytime there is a connection

sftp = ssh file transfer protocol, pretty much replaced the legacy ftp as file transfer protocol
--> 

The basic steps to build an autotools based software component are:
1. Configuration
./configure
Will look at the available build environment, verify required dependencies, generate
Makefiles and a config.h
2. Compilation
make
Actually builds the software component, using the generated Makefiles.
3. Installation
make install
Installs what has been built

/usr --> Linus distribution packages get installed

sudo apt-get install autoconf --> package binaries installed in /usr/bin

/usr/local/bin 00> hand-build Autoconf binaries

/usr/local/bin positioned in your PATH before /usr/bin => this allows your locally built and installed programs to override the ones installed by your distribution package manager

The build system from autotools assumes that you want to install your software in /usr/local. If you want to override this behaviour :

`./configure --prefix=$HOME`

--> $HOME/bin

Autotools:

* autoconf - used to generate configuration script for a project
* automake - use to simplify the process of creating consistent and funtional makefiles
* libtool which provides an abstraction for the portable creation of shared libraries


autoreconf - executes the configuration tools in the autoconf, automake and libtool packages

The GNU Coding Standards (GCS)
The Filesystem Hierarchy Standard (FHS)
GNU Make Manual

recursive build system

A good build system should incorporate proper unuit testing. The most commonly used target for testing a build is the check target, so we'll go ahead and add it in the usual manner.

User commans to build and install projects:
gzip -cd jupiter-1.0.tar.gz | tar xf -
cd jupiter-1.0
make all check
sudo make install


Useful:
https://elinux.org/images/4/43/Petazzoni.pdf
https://books.google.ro/books?id=CuZ8DwAAQBAJ&pg=PT2&lpg=PT2&dq=autotools+in+2020&source=bl&ots=B7bw2JzyHP&sig=ACfU3U09Ku9-23b-diTnmJUgCIKHXmFGfA&hl=ro&sa=X&ved=2ahUKEwio1JDvx53pAhXHpYsKHQ5jApQQ6AEwAHoECAoQAQ#v=onepage&q&f=false
