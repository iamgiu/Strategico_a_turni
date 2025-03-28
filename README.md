Informazioni Base\n
● 1vs1(Human Player versus AI). 

Mappa di gioco
● Dimensione Mappa: Griglia di 25 celle x 25 celle 
● Cellequadrate: ○ Dimensione di ogni Cella: di poco maggiore a quella delle unità di gioco
● Visuale dall’alto 2D al centro della griglia ● Lagriglia deve essere interamente visibile dall’alto 
● Presenza di ostacoli (celle di colore diverso che rappresentano le montagne e gli alberi).
  ○ Il numero di ostacoli deve essere configurabile in percentuale sul numero totale delle celle. 
  
Descrizione Parametri delle Unità di gioco:
Ciascuna unità ha le seguenti variabili: 
● Movimento 
● Tipo di Attacco 
● Range Attacco 
● Danno 
● Punti Vita 

Movimento:
Variabile che identifica di quante caselle (MAX) può muoversi l’unità attraverso uno spostamento scalare verticale o orizzontale (NO OBLIQUO).
Non è possibile muoversi sopra le caselle dove sono presenti ostacoli e dove è già presente un’altra unità, anche se queste caselle si trovano nel tragitto (non si possono “saltare”). 

Tipo di Attacco: 
Esistono due tipi di Attacco: 
● Attacco a distanza 
● Attacco a corto raggio 
Gli attacchi a Distanza ignorano gli ostacoli. 

Range Attacco:
Variabile che identifica quante caselle tra l’unità in attacco e quella in difesa sono necessarie per effettuare l’attacco. 
Il calcolo è effettuato come quello del movimento, ma nel caso dell’attacco a distanza, non tiene conto degli ostacoli. 

Danno:
Variabile che identifica quanti Punti Vita vengono sottratti all'avversario quando il giocatore compie un attacco.Ogniqualvolta che una unità effettua un attacco, il valore Danno viene estratto randomicamente da un range di valori specifici per ogni tipo di unità di gioco. L’unità che subisce un ammontare di danno pari o superiore ai propri punti vita rimanenti viene eliminata dalla griglia. 

Punti Vita:
Variabile che identifica la Vita della singola unità. Una unità muore all’esaurimento dei Punti Vita. 

Tipologia Classi di Gioco: 
  Sniper: 
    ● Movimento: Max 3 celle 
    ● Tipodi Attacco: Attacco a distanza 
    ● Range Attacco: Max 10 
    ● Danno: val. min 4 ↔ val. max 8 
    ● Vita: 20 
  Brawler: 
    ● Movimento: Max 6 celle 
    ● Tipodi Attacco: Attacco a corto raggio 
    ● Range Attacco: 1 
    ● Danno: val. min 1↔ val. max 6 
    ● Vita: 40 
    
Fasi Partita:
1. Adentrambe le parti in gioco vengono assegnate 2 Unità di gioco, una per tipo. 
2. Viene effettuato il lancio di una moneta.
3.  Il vincitore del lancio comincia a mettere una unità a sua scelta.
4.   seguire l’avversario esegue la stessa azione.
5.    Vincitore e avversario continueranno a posizionare le unità, alternandosi l’uno con l'altro, fino all’esaurimento delle unità da posizionare.
6. Il vincitore del lancio esegue il primo turno.
7. Vincitore e avversario continueranno a giocare i propri turni, alternandosi l’uno con l'altro, fino a che non finisce la partita.

Turno di Gioco:
1. Il player in turno seleziona una sua unità cliccando sulla stessa, evidenziando la cella sottostante con un colore diverso.
2. Può effettuare una delle seguenti opzioni:
  a. Eseguire prima un movimento e poi attaccare (se il range di attacco lo consente)
  b. Attaccare senza effettuare il movimento.
  c. Eseguire un movimento senza attaccare. Nel caso che una unità non possa effettuare un movimento, può solo effettuare un attacco. In generale, non dovrebbe essere mai possibile che l’unità non possa eseguire almeno un’azione.
3. Il player in turno seleziona la sua seconda unità ed esegue la medesima azione descritta nel punto 2.
4. Il Turno termina al concludersi delle azioni disponibili di tutte le proprie unità.
5. L’avversario inizia il proprio turno come da punto 1 del Turno di Gioco.

Fine Partita:
La partita finisce quando si verifica una delle seguenti condizioni: 
  ● Tutte le unità avversarie esauriscono i Punti Vita ➜ Vittoria del Player
  ● Tutte le proprie unità esauriscono i Punti Vita ➜ Sconfitta del Player Osservazioni e ulteriori specifiche 
