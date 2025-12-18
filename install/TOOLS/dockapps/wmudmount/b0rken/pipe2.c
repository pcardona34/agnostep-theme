#include <unistd.h>
#include <fcntl.h>

int pipe2(int pipefd[2], int flags){
    long arg;
    if(pipe(pipefd)) return -1;
    flags &= O_NONBLOCK|O_CLOEXEC;
    if(flags){
        arg = fcntl(pipefd[0], F_GETFL);
        if(arg == -1) goto err;
        if(fcntl(sig_fd[0], F_SETFL, arg|flags)) goto err;
        arg = fcntl(pipefd[1], F_GETFL);
        if(arg == -1) goto err;
        if(fcntl(sig_fd[1], F_SETFL, arg|flags)) goto err;
    }
    return 0;

err:
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;
}
