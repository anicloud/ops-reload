#include <sys/types.h>
#include <sys/wait.h>
#include <alloca.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "op/oped/noah/sailor/utility/jails.h"
#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/Cgroups.h"

extern char **environ;

typedef struct {
    char  *user;
    char  *path;
    char  *shell;
    char **args;
    bool   cgroup;
} context_t;

// it's cpp here...
bool jump_into_cgroup(const char *path)
{
    char *p = strdup(path);
    char *cinstance_id = basename(p);
    std::string instance_id(cinstance_id);

    sailor::Logger::set_attached_only(true);

    sailor::Cgroups cgroups("/cgroups");
    sailor::Cgroups::Group group = cgroups.group(instance_id);

    free(p);
    return group.jump_into();
}

static int execute(const context_t *ctx)
{
    char buffer[4096];
    uid_t uid;
    gid_t gid;

    snprintf(buffer, sizeof(buffer), "%s%s", ctx->path, "/tmp");
    if (jails_mount_target_bind("/", buffer, "/tmp")) {
        return 175;
    }

    if (jails_mount_target_bind("/", "/home/coresave", "/tmp/coresave")) {
        return 175;
    }

    snprintf(buffer, sizeof(buffer), "%s%s", ctx->path, "/home");
    if (jails_mount_target_bind("/", buffer, "/home")) {
        return 175;
    }

    if (ctx->cgroup && !jump_into_cgroup(ctx->path)) {
        return 174;
    }

    if (jails_su(ctx->user)) {
        return 173;
    }

    uid = getuid();
    gid = getgid();
    if (uid < 500 || uid > 60000 || gid < 500 || gid > 60000) { /* Hell no. */
        return 173;
    }

    execve(ctx->shell, ctx->args, environ);
    return 176;
}

static int child(void *parameter)
{
    context_t *ctx = (context_t *)parameter;
    _exit(execute(ctx));
}

static int jail(int argc, char *argv[])
{
    char stack[512 * 1024];
    context_t ctx;
    int status;
    pid_t pret;
    pid_t pid;
    int size;
    int ret;
    int i;
    int offset = 0;   // i hate this...

    ctx.cgroup = true;
    if (strncmp("-n", argv[1], 3) == 0) {
        offset = 1;
        ctx.cgroup = false;
    }

    ctx.path = argv[1 + offset];
    ctx.user = argv[2 + offset];

    size = argc - 3 - offset + 2; /* Skip some then add other some. */
    ctx.args = (char **)alloca(sizeof(char *) * (size + 1));
    if (!ctx.args) {
        return -1;
    }

    ctx.shell = getenv("SHELL");
    if (!ctx.shell || !(ctx.args[0] = rindex(ctx.shell, '/'))) {
        ctx.shell = "/bin/sh";
        ctx.args[0] = "sh";
    } else {
        ++ctx.args[0];
    }

    ctx.args[1] = "-c";
    for (i = 2; i < size; ++i) {
        ctx.args[i] = argv[i + 1 + offset];
    }
    ctx.args[size] = NULL;

    /* Look out if stack grows upwards. */
    ret = clone(child, stack + sizeof(stack), CLONE_NEWNS | SIGCHLD, &ctx);
    if (ret < 0) {
        return -1;
    }

    pid = (pid_t)ret;
    for (;;) {
        pret = waitpid(pid, &status, 0);
        if (pret == pid) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        return -2;
    }

    return status;
}

static void print_help(const char *arg)
{
    fprintf(stderr, "%s [-n] <jail> <user> <cmd> [args...]\n", arg);
    fprintf(stderr, "\t[-n] means not jump into cgroup.\n");
}

int main(int argc, char *argv[])
{
    int status;

    if (argc < 4) {
        print_help(argv[0]);
        return 171;
    }

    if (geteuid()) {
        return 170;
    }

    status = jail(argc, argv);
    if (status == -1) {
        return 170;
    } else if (status == -2) {
        return 177;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return WTERMSIG(status) + 128;
    }
}
