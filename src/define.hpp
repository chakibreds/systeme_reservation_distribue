

union semun {
    int val;               /* cmd = SETVAL */
    struct semidds *buf;   /* cmd = IPCSTAT ou IPCSET */
    unsigned short *array; /* cmd = GETALL ou SETALL */
    struct seminfo *__buf; /* cmd = IPCINFO (sous Linux) */
};
