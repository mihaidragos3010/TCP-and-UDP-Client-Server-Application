Mihai Dragos-Andrei
Grupa 322CB

			Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

	In implementarea acestei tema am avut de facut functionalitatea unui server si a unui client care comunica prin TCP. Server-ul primea , de asemenea, un set de mesaje UDP ca topicuri ce au fost convertite ca mesaje TCP catre clienti. 

Pasi de implementate:

	SERVER:
	
	- am initializat socketi principali de TCP si UDP prin care se vor conecta clienti;
	- am scos algoritmul lui Neagle de pe fiecare port TCP;
	- am construit un vector 'poll' prin care ascultam pe mai multi socketi in acelasi timp;
	- am structura 'udp_message' si 'tcp_message' pe care le folosesc pentru a comunica intre clienti si server;
	- folosesc structura 'server_messaje' pentru a trimite peste TCP comenzile de la client la server;
	- folosesc structura 'subscriber' pentru a stoca mai multe atribute ale fiecarui client;
	- fiecare subscriber contine un vector de structuri 'topic' care reprezinta topicurile la care a dat subscribe si;
	daca acestea respecta sau nu proprietatea de 'sf'. Fiecare element din acest vector de topicuri contine un
	vector de 'tcp_message' ce reprezinta stocarea concreta a topicurilor la care a dat 'sf'
	
	SUBSCRIBER:
	
	- am initializat 2 socketi pentru stdin si socket-ul TCP prin care o sa trimita mesaje la server;
	- pe un socket receptioneaz comenzile de la tastatura si le execut;
	- un alt socket receptioneaza mesaajele primite de la server din TCP si verifica daca primeste o comanda sa se
	inchida sau afiseaza tapicul pe care l-a primit;
	
1) Ma folosesc de un vector de subscribers pentru a salva fiecare client si parametri necesari. Fiecare contine un vector
de topicuri ce reprezinta topicurile la care a dat subscribe si daca a folosit proprietatea de 'sf'. In acest vector se 
salveaza doar un model a topicului, si nu mesajul concret. Fiecare structura topic contine un alt vector de 'tcp_massage'
ce salveaza concret mesajele de tip TCP ce urmeaza a fi trimise catre client la reconectare.

LIMITARI PROGRAM: Programul poate sustie un maxim de 10 clienti si un maxim de 31 de topicuri pentru fiecare model de topic
in parte la obtiunea 'sf'.

OBS: Am remarcat ca in momentul in care rulez de mai multe ori checker-ul imi pica testul de 'quick_flow'. Dupa o recompilare
cu 'make clean' urmata ce retestare ar trebui sa isi revina.


