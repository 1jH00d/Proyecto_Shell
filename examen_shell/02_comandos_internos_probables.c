// =============================================================================
// PREGUNTAS PROBABLES DEL EXAMEN - COMANDOS INTERNOS
// =============================================================================
// Basadas en los ejercicios de entrenamiento y patrones típicos del examen.
// Cada sección es un comando interno completo listo para copiar y adaptar.
// =============================================================================


// =============================================================================
// PREGUNTA 1: killall - matar todos los jobs de la lista
// =============================================================================
// Enunciado típico: "Implementar un comando interno killall que mande SIGKILL
// a todos los procesos de la lista de jobs y los elimine de la lista."

/*
if (strcmp(argv[0], "killall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    while (job_list->count > 0) {
        job *j = get_job_bypos(job_list, 1);
        killpg(j->pgid, SIGKILL);
        del_job(job_list, j);
        free_job(j);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 2: stopall - suspender todos los jobs en background
// =============================================================================
// Enunciado típico: "Implementar stopall que suspenda todos los jobs
// que estén ejecutándose en segundo plano."

/*
if (strcmp(argv[0], "stopall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) {
            killpg(j->pgid, SIGSTOP);
            j->state = STOPPED;
            printf("[%d] (%s) Stopped\n", j->pgid, j->command);
        }
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 3: contjobs - contar jobs por estado
// =============================================================================
// Enunciado típico: "Implementar contjobs que imprima cuántos jobs hay
// en background y cuántos suspendidos."

/*
if (strcmp(argv[0], "contjobs") == 0) {
    int bg = 0, stopped = 0;
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) bg++;
        if (j->state == STOPPED)    stopped++;
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    printf("Background: %d, Stopped: %d\n", bg, stopped);
    continue;
}
*/


// =============================================================================
// PREGUNTA 4: infojob - info detallada de un job por posición
// =============================================================================
// Enunciado típico: "Implementar infojob N que muestre PID, comando y
// estado del job en la posición N de la lista."

/*
if (strcmp(argv[0], "infojob") == 0) {
    if (argv[1] == NULL) {
        printf("infojob: falta el número de job\n");
        continue;
    }
    mask_signal(SIGCHLD, SIG_BLOCK);
    job *j = get_job_bypos(job_list, atoi(argv[1]));
    if (j == NULL) {
        printf("infojob: no existe el job %s\n", argv[1]);
    } else {
        printf("PID=%d command=%s state=%s\n",
               j->pgid, j->command, state_strings[j->state]);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 5: bgall - reanudar todos los jobs suspendidos en background
// =============================================================================
// Enunciado típico: "Implementar bgall que reanude en background todos
// los jobs suspendidos."

/*
if (strcmp(argv[0], "bgall") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == STOPPED) {
            j->state = BACKGROUND;
            killpg(j->pgid, SIGCONT);
            printf("[%d] (%s) Running in BACKGROUND\n", j->pgid, j->command);
        }
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 6: sendjob - mandar una señal a un job por posición
// =============================================================================
// Enunciado típico: "Implementar sendjob N S que mande la señal S
// al job en la posición N de la lista."
// Ejemplo: sendjob 1 9  →  manda SIGKILL al job 1

/*
if (strcmp(argv[0], "sendjob") == 0) {
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("sendjob: uso: sendjob <posicion> <señal>\n");
        continue;
    }
    int pos = atoi(argv[1]);
    int sig = atoi(argv[2]);
    mask_signal(SIGCHLD, SIG_BLOCK);
    job *j = get_job_bypos(job_list, pos);
    if (j == NULL) {
        printf("sendjob: no existe el job %d\n", pos);
    } else {
        killpg(j->pgid, sig);
        printf("[%d] (%s) Signal %d sent\n", j->pgid, j->command, sig);
    }
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 7: myjobs - versión personalizada de jobs con formato diferente
// =============================================================================
// Enunciado típico: "Implementar myjobs que liste solo los jobs en background
// (no los suspendidos), mostrando PID y nombre."

/*
if (strcmp(argv[0], "myjobs") == 0) {
    mask_signal(SIGCHLD, SIG_BLOCK);
    int found = 0;
    for (int i = 1; i <= job_list->count; i++) {
        job *j = get_job_bypos(job_list, i);
        if (j->state == BACKGROUND) {
            printf("[%d] %s\n", j->pgid, j->command);
            found = 1;
        }
    }
    if (!found) printf("No hay jobs en background\n");
    mask_signal(SIGCHLD, SIG_UNBLOCK);
    continue;
}
*/


// =============================================================================
// PREGUNTA 8: exec - ejecutar un script interno del shell
// =============================================================================
// Enunciado típico: "Implementar un comando interno exec que ejecute
// un fichero de comandos línea a línea como si los escribiera el usuario."
// NOTA: esto es complejo, lo más probable es que pidan algo más simple.

/*
if (strcmp(argv[0], "exec") == 0) {
    if (argv[1] == NULL) {
        printf("exec: falta el fichero\n");
        continue;
    }
    // Simplemente ejecutarlo como comando externo con sh
    char *script_argv[3];
    script_argv[0] = "sh";
    script_argv[1] = argv[1];
    script_argv[2] = NULL;
    pid_fork = fork();
    if (pid_fork == 0) {
        setpgid(0, 0);
        terminal_signals(SIG_DFL);
        execvp(script_argv[0], script_argv);
        perror(script_argv[0]);
        exit(EXIT_FAILURE);
    } else {
        setpgid(pid_fork, pid_fork);
        tcsetpgrp(STDIN_FILENO, pid_fork);
        waitpid(pid_fork, &wstatus, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        if (WIFEXITED(wstatus))
            printf("[%d] (exec) Terminated with status: %d\n",
                   pid_fork, WEXITSTATUS(wstatus));
    }
    continue;
}
*/


// =============================================================================
// PREGUNTA 9: nohup - lanzar comando inmune a SIGHUP
// =============================================================================
// Enunciado típico: "Implementar nohup <cmd> que lance el comando ignorando
// la señal SIGHUP (para que no muera al cerrar el terminal)."

/*
if (strcmp(argv[0], "nohup") == 0) {
    if (argv[1] == NULL) {
        printf("nohup: falta el comando\n");
        continue;
    }
    char **args = &argv[1];
    pid_fork = fork();
    if (pid_fork == 0) {
        setpgid(0, 0);
        terminal_signals(SIG_DFL);
        signal(SIGHUP, SIG_IGN);   // ignorar SIGHUP
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        setpgid(pid_fork, pid_fork);
        if (background == 0) {
            tcsetpgrp(STDIN_FILENO, pid_fork);
            waitpid(pid_fork, &wstatus, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpid());
            if (WIFEXITED(wstatus))
                printf("[%d] (%s) Terminated with status: %d\n",
                       pid_fork, args[0], WEXITSTATUS(wstatus));
            else if (WIFSIGNALED(wstatus))
                printf("[%d] (%s) Signaled by signal: %d\n",
                       pid_fork, args[0], WTERMSIG(wstatus));
            else if (WIFSTOPPED(wstatus)) {
                mask_signal(SIGCHLD, SIG_BLOCK);
                add_job(job_list, new_job(pid_fork, args[0], STOPPED));
                mask_signal(SIGCHLD, SIG_UNBLOCK);
                printf("[%d] (%s) Stopped by signal: %d\n",
                       pid_fork, args[0], WSTOPSIG(wstatus));
            }
        } else {
            mask_signal(SIGCHLD, SIG_BLOCK);
            add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) Running in Background\n", pid_fork, args[0]);
        }
    }
    continue;
}
*/


// =============================================================================
// PREGUNTA 10: repeat N cmd - lanzar N veces el mismo comando en background
// =============================================================================
// Enunciado típico: "Implementar repeat N <cmd> que lance el comando N veces
// en background." (similar a bgteam)

/*
if (strcmp(argv[0], "repeat") == 0) {
    if (argc < 3 || atoi(argv[1]) <= 0) {
        printf("repeat: uso: repeat <N> <comando> [args...]\n");
        continue;
    }
    int N = atoi(argv[1]);
    char **args = &argv[2];
    for (int k = 0; k < N; k++) {
        pid_fork = fork();
        if (pid_fork == 0) {
            setpgid(0, 0);
            terminal_signals(SIG_DFL);
            execvp(args[0], args);
            perror(args[0]);
            exit(EXIT_FAILURE);
        } else {
            setpgid(pid_fork, pid_fork);
            mask_signal(SIGCHLD, SIG_BLOCK);
            add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) Running in Background\n", pid_fork, args[0]);
        }
    }
    continue;
}
*/
