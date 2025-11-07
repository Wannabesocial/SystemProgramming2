/*
    Log File management. Useful function so we can do actions in logfiles
*/

#ifndef LOG_H
#define LOG_H

/* If file do not exist Create it. If exist truncate. */
void log_init_logfile(const char *logfile_path);

/* Write message in the Manager LogFile */
void log_write_logfile(const char *logfile_path, const char *message);

#endif
