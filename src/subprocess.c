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
#include <msg.h>
#include <errno.h>
#include <msg_todo.h>

extern char **environ;

// returns return code
int subprocess_call(Str_view_vec cmd) {
    Arena a_temp = {0};

    int pid = fork();
    if (pid == -1) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "fork failed: %s\n", strerror(errno));
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        exit(EXIT_CODE_FAIL);
    }

    if (pid == 0) {
        // child process
        Cstr_vec cstrs = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            const char* curr = str_view_to_cstr(&a_temp, vec_at(&cmd, idx));
            vec_append(&a_temp, &cstrs, curr);
        }
        vec_append(&a_temp, &cstrs, NULL);
        execvpe(str_view_to_cstr(&a_temp, vec_at(&cmd, 0)), cstr_vec_to_c_cstr_vec(&a_temp, cstrs), environ);

        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "execvpe failed: %s\n", strerror(errno));
        // TODO: make a separate function to generate cmd_str
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        exit(EXIT_CODE_FAIL);
    }

    // parent process
    int wstatus = 0;
    wait(&wstatus);

    if (WIFEXITED(wstatus)) {
        arena_free(&a_temp);
        return WEXITSTATUS(wstatus);
    } else if (WIFSIGNALED(wstatus)) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process was killed by signal %d\n", WTERMSIG(wstatus));
        // TODO: make a separate function to generate cmd_str
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        exit(EXIT_CODE_FAIL);
    } else if (WIFSTOPPED(wstatus)) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process was stopped by signal %d\n", WSTOPSIG(wstatus));
        // TODO: make a separate function to generate cmd_str
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        msg_todo("handling process stopped by signal", POS_BUILTIN);
        exit(EXIT_CODE_FAIL);
    } else if (WIFCONTINUED(wstatus)) {
        msg_todo("handling process continued", POS_BUILTIN);
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        exit(EXIT_CODE_FAIL);
    }
    unreachable("");
}
