#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <limits.h>

#include <cutils/properties.h>

#define RECOVERY_MODE_FILE "/data/.recovery_mode"

#define UPDATE_BINARY BOARD_HIJACK_RECOVERY_PATH"/update-binary"
#define UPDATE_PACKAGE BOARD_HIJACK_RECOVERY_PATH"/recovery.zip"

int
exec_and_wait(char** argp)
{
    pid_t pid;
    sig_t intsave, quitsave;
    sigset_t mask, omask;
    int pstat;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);
    switch (pid = vfork()) {
    case -1:            /* error */
        sigprocmask(SIG_SETMASK, &omask, NULL);
        return(-1);
    case 0:                /* child */
        sigprocmask(SIG_SETMASK, &omask, NULL);
        execve(argp[0], argp, environ);
    _exit(127);
  }

    intsave = (sig_t)  bsd_signal(SIGINT, SIG_IGN);
    quitsave = (sig_t) bsd_signal(SIGQUIT, SIG_IGN);
    pid = waitpid(pid, (int *)&pstat, 0);
    sigprocmask(SIG_SETMASK, &omask, NULL);
    (void)bsd_signal(SIGINT, intsave);
    (void)bsd_signal(SIGQUIT, quitsave);
    return (pid == -1 ? -1 : pstat);
}

void remount_root() {
    char* remount_root_args[] = { "/system/bin/hijack", "mount", "-orw,remount", "/", NULL };
    exec_and_wait(remount_root_args);
}

typedef char* string;

void mark_file(char* filename) {
    FILE *f = fopen(filename, "wb");
    fwrite("1", 1, 1, f);
    if (f != NULL)
        fclose(f);
}

int main(int argc, char** argv) {
    char* hijacked_executable = argv[0];
    struct stat info;

    if (NULL != strstr(hijacked_executable, "hijack")) {
        // no op
        if (argc >= 2) {
            if (strcmp("sh", argv[1]) == 0) {
                return ash_main(argc - 1, argv + 1);
            }
            if (strcmp("mount", argv[1]) == 0) {
                printf("mount!\n");
                return mount_main(argc - 1, argv + 1);
            }
            if (strcmp("umount", argv[1]) == 0) {
                printf("umount!\n");
                return umount_main(argc - 1, argv + 1);
            }
        }
        printf("Hijack!\n");
        return 0;
    }

    // check to see if hijack was already run, and if so, just continue on.
    if (argc >= 3 && 0 == strcmp(argv[2], "cache")) {
        if (0 == stat(RECOVERY_MODE_FILE, &info)) {
            // don't boot into recovery again
            remove(RECOVERY_MODE_FILE);
            remount_root();

            mkdir("/preinstall", S_IRWXU);
            mkdir("/tmp", S_IRWXU);
            mkdir("/res", S_IRWXU);
            mkdir("/res/images", S_IRWXU);
            remove("/etc");
            mkdir("/etc", S_IRWXU);
            rename("/sbin/adbd", "/sbin/adbd.old");
            property_set("ctl.stop", "runtime");
            property_set("ctl.stop", "zygote");
            property_set("persist.service.adb.enable", "1");

            char* mount_preinstall_args[] = { "/system/bin/hijack", "mount", "/dev/block/preinstall", "/preinstall", NULL };
            exec_and_wait(mount_preinstall_args);

            // this will prevent hijack from being called again
            char* umount_args[] = { "/system/bin/hijack", "umount", "-l", "/system", NULL };
            exec_and_wait(umount_args);

            char* updater_args[] = { UPDATE_BINARY, "2", "0", UPDATE_PACKAGE, NULL };
            return exec_and_wait(updater_args);
        }

        // mark it in case we don't boot
        mark_file(RECOVERY_MODE_FILE);
    }

    char real_executable[PATH_MAX];
    sprintf(real_executable, "%s.bin", hijacked_executable);
    string* argp = (string*)malloc(sizeof(string) * (argc + 1));
    int i;
    for (i = 0; i < argc; i++) {
        argp[i]=argv[i];
    }
    argp[argc] = NULL;

    argp[0] = real_executable;

    // should clean up memory leaks, but it really doesn't matter since the process immediately exits.
    return exec_and_wait(argp);
}
