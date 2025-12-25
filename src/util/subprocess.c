#define _GNU_SOURCE

#include <strv.h>
#include <cstr_darr.h>
#include <subprocess.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <msg.h>
#include <errno.h>
#include <local_string.h>
#include <file.h>
#include <ast_msg.h>

#ifdef _WIN32
#   include <winapi_wrappers.h>
#else
#   include <sys/wait.h>
#endif // _WIN32

Strv cmd_to_strv(Arena* arena, Strv_darr cmd) {
    String cmd_str = {0};
    for (size_t idx = 0; idx < cmd.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(arena, &cmd_str, " ");
        }
        // TODO: consider arguments that contain spaces, etc.
        string_extend_strv(arena, &cmd_str, darr_at(cmd, idx));
    }
    return string_to_strv(cmd_str);
}

#ifndef _WIN32
static int subprocess_call_posix(Strv_darr cmd) {
    int pid = fork();
    if (pid == -1) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "fork failed: %s\n", strerror(errno));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    }

    if (pid == 0) {
        // child process
        Cstr_darr cstrs = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            const char* curr = strv_dup(&a_temp, darr_at(cmd, idx));
            darr_append(&a_temp, &cstrs, curr);
        }
        darr_append(&a_temp, &cstrs, NULL);
        execvpe(strv_dup(&a_temp, darr_at(cmd, 0)), cstr_darr_to_c_cstr_darr(&a_temp, cstrs), environ);

        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "execvpe failed: %s\n", strerror(errno));
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_temp, cmd)));
        local_exit(EXIT_CODE_FAIL);
    }

    // parent process
    int wstatus = 0;
    wait(&wstatus);

    if (WIFEXITED(wstatus)) {
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
#endif // _WIN32

typedef void* Winapi_handle;

typedef struct {
    Winapi_handle hProcess;
    Winapi_handle hThread;
    unsigned long dwProcessId;
    unsigned long dwThreadId;
} Process_information;

// below function and struct adapted from part of subprocess_create_ex and from struct subprocess_startup_info_s in:
//   https://github.com/sheredom/subprocess.h
//   original license: unlicence
//

#ifdef _WIN32
typedef struct {
  unsigned long cb;
  char *lpReserved;
  char *lpDesktop;
  char *lpTitle;
  unsigned long dwX;
  unsigned long dwY;
  unsigned long dwXSize;
  unsigned long dwYSize;
  unsigned long dwXCountChars;
  unsigned long dwYCountChars;
  unsigned long dwFillAttribute;
  unsigned long dwFlags;
  unsigned short wShowWindow;
  unsigned short cbReserved2;
  unsigned char *lpReserved2;
  void *hStdInput;
  void *hStdOutput;
  void *hStdError;
} Subprocess_startup_info_s;

static bool excape_thing(char* commandLine[]) {
    // Combine commandLine together into a single string
    size_t len = 0;
    int i = 0;
    int j = 0;
    int need_quoting = 0;
    char *commandLineCombined = NULL;
    for (i = 0; commandLine[i]; i++) {
        // for the trailing \0
        len++;

        // Quote the argument if it has a space in it
        if (strpbrk(commandLine[i], "\t\v ") != NULL || commandLine[i][0] == '\0') {
            len += 2;
        }

        for (j = 0; '\0' != commandLine[i][j]; j++) {
              switch (commandLine[i][j]) {
                  case '\\':
                    if (commandLine[i][j + 1] == '"') {
                      len++;
                    }

                    break;
                  case '"':
                    len++;
                    break;
                  default:
                    break;
              }
              len++;
        }
    }

    commandLineCombined = arena_alloc(&a_temp, len);

    if (!commandLineCombined) {
      return false;
    }

    // Gonna re-use len to store the write index into commandLineCombined
    len = 0;

    for (i = 0; commandLine[i]; i++) {
        if (0 != i) {
            commandLineCombined[len++] = ' ';
        }

        need_quoting = strpbrk(commandLine[i], "\t\v ") != NULL || commandLine[i][0] == '\0';
        if (need_quoting) {
            commandLineCombined[len++] = '"';
        }

        for (j = 0; '\0' != commandLine[i][j]; j++) {
            switch (commandLine[i][j]) {
                case '\\':
                    if (commandLine[i][j + 1] == '"') {
                      commandLineCombined[len++] = '\\';
                    }

                    break;
                case '"':
                    commandLineCombined[len++] = '\\';
                    break;
                default:
                    break;
            }

            commandLineCombined[len++] = commandLine[i][j];
        }
        if (need_quoting) {
            commandLineCombined[len++] = '"';
        }
    }

    commandLineCombined[len] = '\0';

    Subprocess_startup_info_s startup_info = {0};
    Process_information process_info = {0};

    if (!winapi_CreateProcessA(
        NULL,
        commandLineCombined,
        NULL,
        NULL,
        false,
        0 /* TODO */,
        NULL,
        NULL,
        &startup_info,
        &process_info
    )) {
        unsigned long err = winapi_GetLastError();
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "could not create subprocess: %s\n", winapi_err_print(err));
        msg(DIAG_NOTE, POS_BUILTIN, "attempt to run subprocess with the command: \"%s\"\n", commandLineCombined);
        todo();
    }
    todo();
}
#endif // _WIN32

#ifdef _WIN32
static int subprocess_call_win32(Strv_darr cmd) {
    Cstr_darr cstrs = {0};
    for (size_t idx = 0; idx < cmd.info.count; idx++) {
        const char* curr = strv_dup(&a_temp, darr_at(cmd, idx));
        darr_append(&a_temp, &cstrs, curr);
    }
    darr_append(&a_temp, &cstrs, NULL);
    if (!excape_thing(cstr_darr_to_c_cstr_darr(&a_temp, cstrs))) {
    }
    todo();
}
#endif // _WIN32

// returns return code
int subprocess_call(Strv_darr cmd) {
#   ifdef _WIN32
        return subprocess_call_win32(cmd);
#   else
        return subprocess_call_posix(cmd);
#   endif // _WIN32
}
