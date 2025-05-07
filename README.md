# myFinger

`myFinger` è un programma scritto in linguaggio **C** che emula il comportamento del comando `finger` dei sistemi Unix.  
Permette di ottenere informazioni dettagliate sugli utenti attualmente loggati nel sistema.

---

## 🧩 Funzionalità principali

- Mostra utenti attualmente connessi
- Visualizza:
  - Nome completo
  - Terminale (`TTY`)
  - Tempo di inattività (`idle`)
  - Ultimo accesso
  - Shell, directory, telefono e posizione (se presenti)
  - Stato della posta (`mail`) e del file `.plan`
- Supporta opzioni da linea di comando:
  - `-s` → modalità semplice
  - `-l` → modalità dettagliata
  - `-m` → esclude informazioni sulla posta
  - `-p` → esclude informazioni sul file `.plan`

---

## ⚙️ Compilazione

Per compilare il programma (Linux/WSL/macOS):

```bash
make
