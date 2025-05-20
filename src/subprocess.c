#define _GNU_SOURCE

#include <str_view.h>
#include <cstr_vec.h>
#include <subprocess.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

// returns return code
int subprocess_call(Str_view_vec args) {
    Arena a_temp = {0};

    int pid = fork();
    if (pid == -1) {
        // error
        // TODO
        todo();
    }

    if (pid == 0) {
        // child process
        Cstr_vec cstrs = {0};
        for (size_t idx = 0; idx < args.info.count; idx++) {
            const char* curr = str_view_to_cstr(&a_temp, vec_at(&args, idx));
            vec_append(&a_temp, &cstrs, curr);
        }
        for (size_t idx = 0; idx < cstrs.info.count; idx++) {
            //log(LOG_DEBUG, "%s\n", vec_at(&cstrs, idx));
        }
        vec_append(&a_temp, &cstrs, NULL);
        execvpe(str_view_to_cstr(&a_temp, vec_at(&args, 0)), cstr_vec_to_c_cstr_vec(&a_temp, cstrs), environ);

        // execve failed
        todo();
    }

    // parent process
    int wstatus = 0;
    wait(&wstatus);
    if (!WIFEXITED(wstatus)) {
        // child exited abnormally (eg. terminated by signal)
        // TODO
        todo();
    }

    arena_free(&a_temp);
    return WEXITSTATUS(wstatus);
}
