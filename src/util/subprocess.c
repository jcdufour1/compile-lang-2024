#define _GNU_SOURCE

#include <strv.h>
#include <cstr_vec.h>
#include <subprocess.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <msg.h>
#include <errno.h>
#include <msg_todo.h>
#include <newstring.h>
#include <file.h>

Strv cmd_to_strv(Arena* arena, Strv_vec cmd) {
    String cmd_str = {0};
    for (size_t idx = 0; idx < cmd.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(arena, &cmd_str, " ");
        }
        // TODO: consider arguments that contain spaces, etc.
        string_extend_strv(arena, &cmd_str, vec_at(cmd, idx));
    }
    return string_to_strv(cmd_str);
}

// returns return code
int subprocess_call(Strv_vec cmd) {
    Arena a_temp = {0};

    int pid = fork();
    if (pid == -1) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "fork failed: %s\n", strerror(errno));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    }

    if (pid == 0) {
        // child process
        Cstr_vec cstrs = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            const char* curr = strv_to_cstr(&a_temp, vec_at(cmd, idx));
            vec_append(&a_temp, &cstrs, curr);
        }
        vec_append(&a_temp, &cstrs, NULL);
        execvpe(strv_to_cstr(&a_temp, vec_at(cmd, 0)), cstr_vec_to_c_cstr_vec(&a_temp, cstrs), environ);

        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "execvpe failed: %s\n", strerror(errno));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    }

    // parent process
    int wstatus = 0;
    wait(&wstatus);

    if (WIFEXITED(wstatus)) {
        arena_free(&a_temp);
        return WEXITSTATUS(wstatus);
    } else if (WIFSIGNALED(wstatus)) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process was killed by signal %d\n", WTERMSIG(wstatus));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    } else if (WIFSTOPPED(wstatus)) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process was stopped by signal %d\n", WSTOPSIG(wstatus));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        msg_todo("handling process stopped by signal", POS_BUILTIN);
        local_exit(EXIT_CODE_FAIL);
    } else if (WIFCONTINUED(wstatus)) {
        msg_todo("handling process continued", POS_BUILTIN);
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    }
    unreachable("");
}
