// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午1:06
// Description : ...
//
// **************************************************


#include <cstdio>
#include <unistd.h>

int main(int argc, char **argv) {
    int fd[2];
    int ret = pipe(fd);
    if (ret == -1) {
        perror("pipe create failed");
        return -1;
    }
    int childRead = fd[0];
    int childWrite = fd[1];
    pid_t pid = fork();
    if (pid > 0) {
        // parent
        dup2(childWrite, STDOUT_FILENO);
        close(childRead);
        execlp("ps", "ps", "aux", NULL);
    } else if (pid == 0) {
        // child
        dup2(childRead, STDIN_FILENO);
        close(childWrite);
        execlp("grep", "grep", "bash", "--color=auto", NULL);
    }
    return 0;
}