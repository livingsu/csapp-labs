# lab7 tiny shell笔记

主要任务是在`tsh.h`中实现一个简单功能的shell（支持job）。有16个测试，是`make test01`至`make test16`，依次测试`trace01.txt`至`trace16.txt`中的内容。

## 测试的流程

输入`make test01`，会调用sdriver.pl对tsh进行测试。

<img src="https://s2.loli.net/2022/04/15/5EuTYflewmZxIsb.png" alt="image-20220415094224415" style="zoom:50%;" />

sdriver.pl的主要功能是模拟用户输入，比如可以等待1秒钟之后输入ctrl+z（发送SIGSTP）。

sdriver读取txt的每一行，识别自己的特殊命令（如TSTP，INT）等，可以给child shell发信号。自己识别不了的才通过Writer发送给child shell的stdin，child shell的打印（stdout）会发送给sdriver的Reader，然后Reader打印

<img src="https://raw.githubusercontent.com/livingsu/ImageRepo/main/img/image-20220415095417364.png?token=ANJSFJ7LDJOQMEIPXBNGQ6LCLDKXW" alt="image-20220415095417364" style="zoom:50%;" />

<img src="https://raw.githubusercontent.com/livingsu/ImageRepo/main/img/image-20220415094446236.png?token=ANJSFJ7UK5FGTF4BQ43QLBTCLDKVI" alt="image-20220415094446236" style="zoom:50%;" />

<img src="https://raw.githubusercontent.com/livingsu/ImageRepo/main/img/image-20220415095457355.png?token=ANJSFJ3R6DKVEEJSP4OXNCLCLDKYO" alt="image-20220415095457355" style="zoom:50%;" />

## trace01

```
#
# trace01.txt - Properly terminate on EOF.
#
CLOSE
WAIT
```

```perl
 // sdriver.pl
    # Close pipe (sends EOF notification to child)
    elsif ($line =~ /CLOSE/) {
	  if ($verbose) {
	  	print "$0: Closing output end of pipe to child $pid\n";
		}
		close Writer;
    }
```

sdriver打印前三行注释，执行CLOSE发送eof信号给child shell，正确的功能是shell关闭，wait被SIGCHILD打断。在初始代码中已实现该功能：

```c
	// main()
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}
```

## trace02

是要实现quit内置指令。参考CSAPP 3e的P525~526，先把eval()和buildin_command()框架写上去：

```c
void eval(char *cmdline) 
{
    char *argv[MAXARGS];    // Argument list execve()
    int bg;                 // Should the job run in bg or fg?
    pid_t pid;              // Process id

    bg = parseline(cmdline, argv);
    if (argv[0] == NULL)
        return;  // Ignore empty lines

    if (!builtin_cmd(argv)) {
        if ((pid = Fork()) == 0) {  // Child runs user job
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }
        // Parent waits for foreground job to terminate
        if (!bg) {
            int status;
          	if (waitpid(-1, &status, 0) < 0)
            		unix_error("waitfg: waitpid error");
        } else {
            printf("%d %s", pid, cmdline);
        }
    }

    return;
}

int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit"))
        exit(0);
    if (!strcmp(argv[0], "&"))
        return 1;
    return 0; /* not a builtin command */
}
```

## trace03

要实现非内置命令执行文件。trace02已经完成。

第4行的意思是执行

```shell
/bin/echo "tsh>" "quit"
```

简单打印`tsh> quit`

而不是执行

```
/bin/echo tsh> quit
```

后者会将tsh字符串输出到quit文件中

## trace04

echo -e表示打印转义字符，\046表示&

目的是要执行后台job，tsh不用等待。trace02也已经完成，但是输出需要修改。

make test04输出：

```
#
# trace04.txt - Run a background job.
#
tsh> ./myspin 1 &
[1] (26252) ./myspin 1 &
```

则将eval()中的bg输出改为：

```c
printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
```

## trace05

实现`jobs`指令

在`buildin_command()`添加：

```c
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
```

在`eval()`中添加`add_job()`，需要对信号进行阻塞。这里的`Sigfillset()`等函数是从[csapp官网的csapp.c](https://csapp.cs.cmu.edu/3e/ics3/code/src/csapp.c)获取到的。

```c
	void eval(char *cmdline) 
{
    char *argv[MAXARGS];    // Argument list execve()
    int bg;                 // Should the job run in bg or fg?
    pid_t pid;              // Process id
    sigset_t mask_all, mask_one, prev;      // signal sets

    bg = parseline(cmdline, argv);
    if (argv[0] == NULL)
        return;  // Ignore empty lines

    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);

    if (!builtin_cmd(argv)) {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev);
        if ((pid = Fork()) == 0) {  // Child runs user job
            Sigprocmask(SIG_SETMASK, &prev, NULL);
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(jobs, pid, bg ? BG : FG, cmdline);
        Sigprocmask(SIG_SETMASK, &prev, NULL);
        // Parent waits for foreground job to terminate
        if (!bg) {
            int status;
          	if (waitpid(-1, &status, 0) < 0)
            		unix_error("waitfg: waitpid error");
        } else {
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }

    return;
}
```

## trace06,07

实现sigint_handler和sigchld_handler

shell发送sigint给前台job（这里先不考虑进程组），job结束后到sigchld_handler中回收，并打印出相关信息和删除job

```c
void sigint_handler(int sig) 
{
    int olderrno = errno;
    pid_t fg_pid = fgpid(jobs);
    if (fg_pid)
        Kill(fg_pid, sig);
    errno = olderrno;
    return;
}

void sigchld_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid;
    sigset_t mask_all, prev;
    int status;

    Sigfillset(&mask_all);
    // WNOHANG | WUNTRACED return immediately
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        Sigprocmask(SIG_BLOCK, &mask_all, &prev);
        struct job_t* job = getjobpid(jobs, pid);
        if (WIFEXITED(status)) {
            // normally returned or exited, delete job
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) {
            // terminated by signal, delete job and print message
            printf("Job [%d] (%d) terminated by signal %d\n", job->jid, pid, WTERMSIG(status));
            deletejob(jobs, pid);
        }
        Sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    errno = olderrno;
    return;
}
```

同时，eval()中的`waitpid`改为`waitfg()`

```c
				if (!bg) {
            waitfg(pid);
        }
```

```c
void waitfg(pid_t pid)
{
    while (pid == fgpid(jobs))
        Sleep(1);  // sleep one second
    return;
}
```

采用简单的轮询策略：当回收后，deletejob成功，则waitfg不继续等待。

## trace08

实现sigtstp_handler

```c
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    pid_t fg_pid = fgpid(jobs);
    if (fg_pid)
        Kill(fg_pid, sig);
    errno = olderrno;
    return;
}
```

然后到sigchld_handler中添加：

```c
				else if (WIFSTOPPED(status)) {
            // stopped by signal, change job state
            printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid, WSTOPSIG(status));
            job->state = ST;
        }
```

## trace09,10

实现`bg`，`fg`指令：将停止的job变为运行的后台/前台job

在`buildin_command()`添加：

```c
		if (!strcmp(argv[0], "fg") || !strcmp(argv[0], "bg")) {
        do_bgfg(argv);
        return 1;
    }
```

```c
void do_bgfg(char **argv) 
{
    char* cmd = argv[0];
    char* id = argv[1];
    struct job_t* job;

    if (id[0] == '%') {
        int jid = atoi(id + 1);
        job = getjobjid(jobs, jid);
    } else {
        int pid = atoi(id);
        job = getjobpid(jobs, pid);
    }
    Kill(job->pid, SIGCONT);
    if (!strcmp(cmd, "bg")) {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    } else if (!strcmp(cmd, "fg")) {
        job->state = FG;
        waitfg(job->pid);
    }
    return;
}
```

## trace 11,12,13

检查是否信号发送给前台的进程组而不是单个进程。

首先修改`sigint_handler`，`sigtstp_handler`：

```c
		if (fg_pid)
        Kill(-fg_pid, sig);  // send signal to process group
```

但是进程id并不等于进程组id，所以需要在execve之前显式设置：

```c
						Setpgid(0, 0);
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
```

还要修改`do_bgfg`：

```c
    Kill(-(job->pid), SIGCONT);
```

## trace 14

bg，fg错误信息打印。

```c
void do_bgfg(char **argv) 
{
    char* cmd = argv[0];
    char* id = argv[1];
    struct job_t* job;

    if (!id) {
        printf("%s command requires PID or %%jobid argument\n", cmd);
        return;
    }
    if (id[0] == '%') {
        int jid = atoi(id + 1);
        if (!jid) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
        job = getjobjid(jobs, jid);
        if (!job) {
            printf("%%%d: No such job\n", jid);
            return;
        }
    } else {
        int pid = atoi(id);
        if (!pid) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
        job = getjobpid(jobs, pid);
        if (!job) {
            printf("(%d): No such process\n", pid);
            return;
        }
    }
    Kill(-(job->pid), SIGCONT);
    if (!strcmp(cmd, "bg")) {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    } else if (!strcmp(cmd, "fg")) {
        job->state = FG;
        waitfg(job->pid);
    }
    return;
}
```

## trace 15,16

完成1~14后，15,16测试通过