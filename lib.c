#include <stdbool.h>
#include <stdio.h> //Operazioni di input/output
#include <stdlib.h> //Per funzioni di utilità come allocazione di memoria,conversioni
#include <pwd.h> //Per ottenere informazioni sull'utente dal file "etc/passwd"
#include <unistd.h> //Per accesso a costanti POSIX e chiamate di sistema
#include <sys/types.h> //Per gestire tipi di dato speciali e ottenere informazioni sul file
#include <time.h> //Per la gestione del tempo
#include <utmpx.h> //Per accedere alla tabella degli utenti connessi
#include <sys/stat.h> //Per ottenere informazioni sul file
#include <string.h> //Per operazioni sulle stringhe
#include <fcntl.h> //Per l'accesso a funzionalità sui file descriptor
#include "lib.h"

/**
 * Generates a formatted string of idle time.
 *
 * @param seconds The number of seconds of idle time.
 * @param detailed If true, provides a detailed format.
 * @return A formatted string representing the idle time.
 */
char* getIdleTimeFormatted(double seconds, bool detailed) {
    static char buff[50]; // Buffer statico per la stringa formattata

    if (seconds < 60) {
        sprintf(buff, detailed ? "idle %ds" : "%ds", (int)seconds);
    } else if (seconds < 3600) {
        sprintf(buff, detailed ? "idle 0:%02d" : "%d", (int)(seconds / 60));
    } else if (seconds < 86400) {
        int hours = seconds / 3600;
        int minutes = (seconds - (hours * 3600)) / 60;
        sprintf(buff, detailed ? "idle %d:%02d" : "%d:%02d", hours, minutes);
    } else {
        int days = seconds / 86400;
        sprintf(buff, detailed ? "idle %d days" : "%dd", days);
    }
    return buff;
}


/**
 * Calculates the idle time from the given seconds.
 *
 * @param seconds The number of seconds since the last activity.
 * @return A formatted string representing the idle time.
 */
char* calculateIdleTime(double seconds) {
    return getIdleTimeFormatted(difftime(time(NULL), seconds), false); //Calcola la differenza attuale tra il tempo attuale e "seconds" e formatta il risultato
}

/**
 * Returns the day of the week or the formatted date.
 *
 * @param aTime The time to format.
 * @return A formatted string representing the day of the week or date.
 */
char* getWeekDayString(time_t aTime) {
    static char time_buf[32];
    struct tm *tm_info = localtime(&aTime);
    // Converte il timestamp in una struttura tm
    strftime(time_buf, sizeof(time_buf), "%b %d", tm_info);
    return time_buf;
}

/**
 * Returns the formatted hours and minutes.
 *
 * @param aTime The time to format.
 * @return A formatted string representing the hours and minutes.
 */
char* getTimeHoursMinutes(time_t aTime) {  //Numero di secondi dal 1^gen 1970
    static char time_buf[32]; //Buffer statico per il tempo
    struct tm *tm_info = localtime(&aTime); //Converte il timestamp in una struttura tm
    strftime(time_buf, sizeof(time_buf), "%H:%M", tm_info); // Formatta il tempo nel formato "HH:MM" (es. "14:30")
    return time_buf; //Restituisce il puntatore della stringa formattata
}

/**
 * Extracts user information from the GECOS field.
 *
 * @param gecos The GECOS field string.
 * @param userInfo The UserInfo structure to populate.
 */
void parseUserGecos(const char *gecos, UserInfo *userInfo) {
    char buffer[256]; //Buffer per la stringa

    //Copia la stringa gecos nel buffer in modo sicuro evitando overflow
    strncpy(buffer, gecos, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; //assicura la stringa sia terminata con "\0"

    //Estrazione del primo termine (nome utente)
    char *token = strtok(buffer, ","); //Divide la stringa in token separati da ",
    strncpy(userInfo->name, token ? token : "", sizeof(userInfo->name));

    //Estrazione del secondo termine (Posizione ufficio)
    token = strtok(NULL, ",");
    strncpy(userInfo->officeLocation, token ? token : "", sizeof(userInfo->officeLocation));

    token = strtok(NULL, ",");
    // Se esiste il numero e ha almeno 10 cifre, lo formatta
    if (token && strlen(token) == 10) {
    snprintf(userInfo->officePhone, sizeof(userInfo->officePhone),
             "%.3s-%.3s-%.4s", token, token + 3, token + 6);
    } else {
    strncpy(userInfo->officePhone, token ? token : "", sizeof(userInfo->officePhone));
}

}

/**
 * Populates the UserInfo structure with user information.
 *
 * @param ut The utmpx structure containing user information.
 * @param pwd The passwd structure containing user information.
 * @return A populated UserInfo structure.
 */
UserInfo getUserInfo(struct utmpx *ut, struct passwd *pwd) {
    UserInfo userInfo;

    // Copia i valori nei campi della struttura UserInfo
    strncpy(userInfo.login, ut->ut_user, sizeof(userInfo.login));
    strncpy(userInfo.directory, pwd->pw_dir, sizeof(userInfo.directory));
    strncpy(userInfo.shell, pwd->pw_shell, sizeof(userInfo.shell));
    strncpy(userInfo.tty, ut->ut_line, sizeof(userInfo.tty));

    // Estrae le informazioni dal campo GECOS (Nome, Ufficio, Telefono)
    parseUserGecos(pwd->pw_gecos, &userInfo);

    // Formatta e memorizza il giorno della settimana del login
    strncpy(userInfo.weekDay, getWeekDayString(ut->ut_tv.tv_sec), sizeof(userInfo.weekDay));

    // Formatta e memorizza l'orario esatto del login in formato "HH:MM"
    strncpy(userInfo.hoursMinutes, getTimeHoursMinutes(ut->ut_tv.tv_sec), sizeof(userInfo.hoursMinutes));

    // Variabile per contenere il tempo di inattività
    char *idleTime;
    struct stat f_info;
    char path[50];

    // Controllo se l'utente è connesso alla console principale
    if (strcmp(userInfo.tty, "console") == 0) {
        // Calcola il tempo di inattività basato sul timestamp di login
        idleTime = calculateIdleTime(ut->ut_tv.tv_sec);
    } else {
        // Costruisce il percorso del terminale virtuale (es. `/dev/pts/1`)
        snprintf(path, sizeof(path), "/dev/%s", userInfo.tty);

        // Se il terminale esiste, calcola il tempo di inattività basato su `st_atime`
        if (stat(path, &f_info) == 0) {
            idleTime = calculateIdleTime(f_info.st_atime);  // Usa `st_atime` invece di `st_mtime`
        } else {
            idleTime = "";
        }
    }

    // Copia il tempo di inattività calcolato nella struttura UserInfo
    strncpy(userInfo.idle, idleTime, sizeof(userInfo.idle));

    return userInfo; // Restituisce la struttura popolata con i dati dell'utente
}


/**
 * Checks if a user is already present in the array.
 *
 * @param users The array of user names.
 * @param user_count The number of users in the array.
 * @param username The user name to check.
 * @return 1 if the user is present, 0 otherwise.
 */
int extractUserInfo(char *users[], int user_count, char *username)
{
    //Scansiona tutti gli utenti nell'array
    for (int i = 0; i < user_count; i++) {
        //Se l'utente è già presente, restituisce 1
        if (strcmp(users[i], username) == 0) return 1;
    }
    return 0; //Se l'utente non è presente, restituisce 0
}

/**
 * Checks if a file exists.
 *
 * @param path The path to the file.
 * @return 1 if the file exists, 0 otherwise.
 */
int verifyIfFileExists(const char *path) {
    struct stat buffer; //Struttura per immagazzinare le informazioni del file
    return stat(path, &buffer) == 0; //Restituisce 1 se il file esiste, 0 altrimenti
}

/**
 * Checks if the user has new emails.
 * @param username The user name to check for emails.
 */
// Funzione per verificare se l'utente ha delle nuove email
void verifyUserMail(const char *username) {
    // Dichiarazione di una variabile per contenere il percorso del file della posta dell'utente
    char mail_path[256];

    // Creazione del percorso del file della posta dell'utente
    // Il percorso del file di posta si trova in "/var/mail/<username>"
    snprintf(mail_path, sizeof(mail_path), "/var/mail/%s", username);

    // Struttura per contenere le informazioni sul file
    struct stat mail_stat;

    // Controlla se il file di posta esiste
    // Se il file di posta non esiste, stampiamo un messaggio "No Mail." e usciamo dalla funzione
    if (stat(mail_path, &mail_stat) != 0) {
        printf("No Mail.\n");
        return; // Termina la funzione se non ci sono e-mail
    }

    // Otteniamo l'ora dell'ultimo accesso al file di posta (quando è stato letto o modificato)
    time_t mail_mod_time = mail_stat.st_atime;

    // Otteniamo l'orario corrente
    time_t now = time(NULL);

    // Converte il tempo dell'ultimo accesso in una struttura tm per poterlo formattare
    struct tm *tm_info = localtime(&mail_mod_time);

    // Stringa per memorizzare l'ora in cui l'email è stata letta
    char last_read_time[64];

    // Formattta la data dell'ultimo accesso (modifica del file) in un formato leggibile
    // "%a %b %d %H:%M %Y (%Z)" sarà il formato tipo "Mon Apr 05 15:30 2025 (CET)"
    strftime(last_read_time, sizeof(last_read_time), "%a %b %d %H:%M %Y (%Z)", tm_info);

    // Variabile per memorizzare il tempo dell'ultimo accesso al file di posta
    time_t last_access_time = mail_stat.st_atime;

    // Confrontiamo il tempo di ultimo accesso con quello di modifica
    // Se il tempo di accesso è maggiore o uguale al tempo di modifica, significa che la posta è stata letta
    if (last_access_time >= mail_mod_time) {
        printf("Mail last read %s\n", last_read_time); // Stampa l'ora dell'ultimo accesso
    } else {
        // Altrimenti, se il tempo di modifica (nuove e-mail ricevute) è maggiore, significa che ci sono nuove e-mail
        printf("New mail received %s\n", last_read_time); // Stampa che ci sono nuove e-mail
    }
}



/**
 * Checks if a `.plan` file exists in the user's home directory.
 *
 * @param home_directory The home directory of the user.
 */
void verifyUserPlan(const char *home_directory)
{
    char plan_path[256]; //Buffer per costruire il percorso del file .plan
    snprintf(plan_path, sizeof(plan_path), "%s/.plan", home_directory);

    //Costruisce il percorso del file .plan nella home dell'utente
    if (!verifyIfFileExists(plan_path)) {
        printf("No Plan.\n");
        return;
    }

    //Controlla se il file .plan esiste
    FILE *file = fopen(plan_path, "r");
    if (!file) {
        perror("Errore apertura file .plan");
        return;
    }


    printf("Plan:\n");
    char line[256]; //Buffer per memorizzare ogni riga del file
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file); //Chiude il file per liberare le risorse
}