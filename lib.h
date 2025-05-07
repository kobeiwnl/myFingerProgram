// lib.h
#ifndef LIB_H
#define LIB_H

#include <time.h> //Per la gestione del tempo

/**
 * Structure to hold user information.
 */
typedef struct {
    char login[32];          /**< User login name */
    char name[256];          /**< User full name */
    char directory[50];      /**< User home directory */
    char shell[32];          /**< User shell */
    char tty[32];            /**< User terminal */
    char idle[50];           /**< User idle time */
    char weekDay[32];        /**< Day of the week */
    char hoursMinutes[32];   /**< Hours and minutes */
    char officePhone[32];    /**< Office phone number */
    char officeLocation[32]; /**< Office location */
} UserInfo;

/**
 * Calculates the idle time from the given seconds.
 *
 * @param seconds The number of seconds since the last activity.
 * @return A formatted string representing the idle time.
 */
char* calculateIdleTime(double seconds);

/**
 * Gets the complete idle time from the given seconds.
 *
 * @param seconds The number of seconds since the last activity.
 * @return A formatted string representing the complete idle time.
 */
char* getCompleteIdle(double seconds);

/**
 * Returns the day of the week or the formatted date.
 *
 * @param aTime The time to format.
 * @return A formatted string representing the day of the week or date.
 */
char* getWeekDayString(time_t aTime);

/**
 * Returns the formatted hours and minutes.
 *
 * @param aTime The time to format.
 * @return A formatted string representing the hours and minutes.
 */
char* getTimeHoursMinutes(time_t aTime);

/**
 * Extracts user information from the GECOS field.
 *
 * @param gecos The GECOS field string.
 * @param userInfo The UserInfo structure to populate.
 */
void parseUserGecos(const char *gecos, UserInfo *userInfo);

/**
 * Populates the UserInfo structure with user information.
 *
 * @param ut The utmpx structure containing user information.
 * @param pwd The passwd structure containing user information.
 * @return A populated UserInfo structure.
 */
UserInfo getUserInfo(struct utmpx *ut, struct passwd *pwd);

/**
 * Checks if a user is already present in the array.
 *
 * @param users The array of user names.
 * @param user_count The number of users in the array.
 * @param username The user name to check.
 * @return 1 if the user is present, 0 otherwise.
 */
int extractUserInfo(char *users[], int user_count, char *username);

/**
 * Checks if a file exists.
 *
 * @param path The path to the file.
 * @return 1 if the file exists, 0 otherwise.
 */
int verifyIfFileExists(const char *path);

/**
 * Checks if the user has new emails.
 *
 * @param username The user name to check for emails.
 */
void verifyUserMail(const char *username);

/**
 * Checks if a `.plan` file exists in the user's home directory.
 *
 * @param home_directory The home directory of the user.
 */
void verifyUserPlan(const char *home_directory);

#endif