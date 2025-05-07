#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <utmpx.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "lib.h"

/**
 * Prints user information based on the provided mode.
 * @param userInfo The UserInfo structure containing user information.
 * @param last_login The last login time as a formatted string.
 * @param mode The mode to determine the level of detail to print.
 */
void printUserInfo(UserInfo userInfo, char *last_login, char mode) {
    // Modalità compatta ('p'): stampa nome, login, terminale, idle time e orario di login
    if (mode == 'p') {
        printf("%-15s %-15s %-5s %-8s %-10s %-10s\n",
               userInfo.login, userInfo.name, userInfo.tty, userInfo.idle,
               userInfo.weekDay, userInfo.hoursMinutes);
        return;
    }
    // Modalità semplificata ('s'): stampa nome, login, terminale e idle time
    if (mode == 's') {
        printf("%-15s %-15s %-5s %-8s\n",
               userInfo.login, userInfo.name, userInfo.tty, userInfo.idle);
        return;
    }
    // Modalità estesa ('l')
    if (mode == 'l') {
        printf("%-15s %-15s %-5s %-8s %-20s %-20s %-10s %-10s %-10s %-12s\n",
               userInfo.login, userInfo.name, userInfo.tty, userInfo.idle,
               userInfo.directory, userInfo.shell,
               userInfo.weekDay, userInfo.hoursMinutes,
               userInfo.officeLocation, userInfo.officePhone);
        return;
    }
    // Stampa dettagliata se non è 'p' o 's'
    printf("Login: %-33sName: %s\n", userInfo.login, userInfo.name);
    printf("Directory: %-29sShell: %s\n", userInfo.directory, userInfo.shell);
    printf("Office: %s, %s\n", userInfo.officeLocation, userInfo.officePhone);
    printf("On since %s on %s,       %s (messages off)\n",
           last_login, userInfo.tty, userInfo.idle);

    if (mode == 'm') return;  // -m: no mail and plan
    verifyUserMail(userInfo.login);
    verifyUserPlan(userInfo.directory);
}

/**
 * Handles the user information retrieval and printing based on the username and mode.
 *
 * @param username The username to handle.
 * @param mode The mode to determine the level of detail to print.
 */
void handleUser(char *username, char mode) {
    struct utmpx *ut; //puntatore per leggere la tabella degli utenti connessi
    struct passwd *pwd = getpwnam(username); //Recupera informazioni sull'utente del file /etc/passwd
    //Se l'utente non è trovato, stampa un errore e termina
    if (pwd == NULL) {
        fprintf(stderr, "\nUser %s not found!\n\n", username);
        return;
    }

    time_t lastLoginTime = 0; //Variabile per memorizzare il timestamp dell'ultimo login
    char last_login[64]; //Stringa per formattare l'orario dell'ultimo login
    int user_info_printed = 0; //Flag per evitare di stampare più volte le informazioni dell'utente

    // Resetta la posizione del file utmpx per scorrere gli utenti attivi
    setutxent();
    //Scansiona gli utenti attualemente connessi per trovare l'ultimo login dell'utente
    while ((ut = getutxent()) != NULL) {
        if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, username, __UT_NAMESIZE) == 0) {
            //Se l'utente ha un timestamp di login più recente, aggiorniamo lastLoginTime
            if (ut->ut_tv.tv_sec > lastLoginTime) {
                lastLoginTime = ut->ut_tv.tv_sec;
            }
        }
    }

    //Converte il timestamp dell'ultimo login in una stringa leggibile
    struct tm *tm_info = localtime(&lastLoginTime);
    strftime(last_login, sizeof(last_login), "%a %b %d %H:%M (%Z)", tm_info);

    //Scansiona nuovamente la tabella degli utenti connessi
    setutxent();
    while ((ut = getutxent()) != NULL) {
        if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, username, __UT_NAMESIZE) == 0) {
            //Crea e popola la struttura UserInfo con le informazioni dell'utente
            UserInfo userInfo = getUserInfo(ut, pwd);
            //Stampa le informazioni dell'utente se non sono state già stampate
            if (!user_info_printed) {
                printUserInfo(userInfo, last_login, mode);
            }
        }
    }
}

/**
 * Lists all logged-in users with varying levels of detail based on the mode.
 *
 * @param mode The mode to determine the level of detail to print.
 */
void listLoggedUsers(char mode) {
    struct utmpx *ut; //puntatore per leggere la tabella degli utenti connessi
    setutxent(); //Resetta la posizione del file utmpx per scorrere gli utenti attivi

    //Stampa l'intestazione della tabella a seconda della modalità
    if (mode == 's') {
        printf("%-15s %-10s %-6s %-8s\n", "Login", "Name", "TTY", "Idle");
    } else if (mode == 'p') {
        printf("%-15s %-10s %-6s %-8s %-10s %-10s\n",
               "Login", "Name", "TTY", "Idle", "Login", "Time");
    } else if (mode == 'l') {
        printf("%-15s %-10s %-6s %-8s %-20s %-20s %-10s %-10s %-10s %-12s\n",
               "Login", "Name", "TTY", "Idle", "Directory", "Shell", "Login", "Time", "Office", "Phone");
    }else {
        printf("%-15s %-10s %-6s %-8s %-10s %-10s %-10s %-12s\n",
               "Login", "Name", "TTY", "Idle", "Login", "Time", "Office", "Phone");
    }

    //Scansiona tutti gli utenti connessi al sistema
    while ((ut = getutxent()) != NULL) {
        if (ut->ut_type == USER_PROCESS) { //Considera solo gli utenti loggati
            struct passwd *pwd = getpwnam(ut->ut_user); //Ottiene informazioni sull'utente dal file /etc/passwd
            if (pwd != NULL) { //Se l'utente esiste nel sistema
                UserInfo userInfo = getUserInfo(ut,
                                                pwd); //Crea e popola la struttura UserInfo con le informazioni dell'utente
                if (strcmp(ut->ut_line, "console") == 0) {
                    strncpy(userInfo.tty, "*console", sizeof(userInfo.tty));
                } else if (strncmp(ut->ut_line, "pts", 3) == 0) {
                    snprintf(userInfo.tty, sizeof(userInfo.tty), "*%.30s", ut->ut_line);
                } else {
                    strncpy(userInfo.tty, ut->ut_line, sizeof(userInfo.tty));
                }


                //Stampa le informazioni dell'utente in basse alla modalità
                if (mode == 's') {
                    printf("%-15s %-10s %-5s %-8s\n",
                           userInfo.login, userInfo.name, userInfo.tty, userInfo.idle);
                } else if (mode == 'p') {
                    printf("%-15s %-10s %-5s %-8s %-10s %-10s\n",
                           userInfo.login, userInfo.name, userInfo.tty, userInfo.idle,
                           userInfo.weekDay, userInfo.hoursMinutes);
                } else if (mode == 'l') {
                    printf("%-15s %-10s %-5s %-8s %-20s %-20s %-10s %-10s %-10s %-12s\n",
                           userInfo.login, userInfo.name, userInfo.tty, userInfo.idle,
                           userInfo.directory, userInfo.shell,
                           userInfo.weekDay, userInfo.hoursMinutes,
                           userInfo.officeLocation, userInfo.officePhone);
                }else {
                    printf("%-15s %-10s %-5s %-8s %-10s %-10s %-10s %-12s\n",
                           userInfo.login, userInfo.name, userInfo.tty, userInfo.idle,
                           userInfo.weekDay, userInfo.hoursMinutes,
                           userInfo.officeLocation, userInfo.officePhone);
                }
            }
        }
    }
    endutxent(); //Chiude il file utmpx
}

/**
 * Main function to handle command-line arguments and execute the appropriate functions.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return 0 on success, 1 on error.
 */

int main(int argc, char *argv[]) {

    if (argc == 1) { //Caso base
        listLoggedUsers(0);
        return 0;
    }
    //Se il primo argomento inizia con "-", significa che è stata passata un'opzione
    if (argv[1][0] == '-') {
        char mode = argv[1][1]; // Estrae la modalità dell'argomento passato

        if (strcmp(argv[1], "-ls") == 0) {
            struct utmpx *ut;
            setutxent();

            char printedUsers[100][__UT_NAMESIZE]; // Array per tenere traccia degli utenti già stampati
            int printedCount = 0; // Contatore degli utenti stampati

            while ((ut = getutxent()) != NULL) {
                if (ut->ut_type == USER_PROCESS) {
                    int alreadyPrinted = 0;

                    // Controlla se l'utente è già stato stampato
                    for (int i = 0; i < printedCount; i++) {
                        if (strncmp(printedUsers[i], ut->ut_user, __UT_NAMESIZE) == 0) {
                            alreadyPrinted = 1;
                            break;
                        }
                    }

                    // Se l'utente non è stato stampato, lo stampa e lo aggiunge alla lista
                    if (!alreadyPrinted) {
                        struct passwd *pwd = getpwnam(ut->ut_user); // Assicura che pwd sia aggiornato per ogni utente
                        if (pwd != NULL) {
                            UserInfo userInfo = getUserInfo(ut, pwd);  // Ottieni info dell'utente corretto
                            time_t login_time = ut->ut_tv.tv_sec;
                            struct tm *tm_info = localtime(&login_time);
                            char last_login[64];
                            strftime(last_login, sizeof(last_login), "%a %b %d %H:%M (%Z)", tm_info);

                            printUserInfo(userInfo, last_login, 0);  // Stampa info dettagliate per ogni utente

                            strncpy(printedUsers[printedCount], ut->ut_user, __UT_NAMESIZE);
                            printedCount++;
                            printf("\n");  // Riga vuota per separare gli utenti
                        }
                    }
                }
            }

            endutxent();
            return 0;
        }



        //Verifica se l'opzione è valida (controlla che non ci siano caratteri e che sia una modalità accettata
        if (argv[1][2] != '\0' || (mode != 'l' && mode != 's' && mode != 'm' && mode != 'p')) {
            printf("Usage: finger [-lmps] [user ...]\n"); //Messaggio di errore in caso di argomento non valido
            return 1; //Esce coon codice di errore
        }
        // Se c'è solo l'opzione (es. ./myFinger -l), elenca gli utenti connessi nel formato richiesto
        if (argc == 2) {
            listLoggedUsers(mode);
        } else {
            // Se ci sono anche nomi utenti (es. ./myFinger -l user1 user2), li gestisce singolarmente
            for (int i = 2; i < argc; i++) {
                handleUser(argv[i], mode);
            }
        }
    } else {
        //Se non è stata specificata un'opzione, considera gli argomenti con nomi utenti
        for (int i = 1; i < argc; i++) {
            handleUser(argv[i], 0); //Chiamata a handleUser() per ogni utente passato
        }
    }

    return 0;
}

