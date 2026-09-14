# myshell — a Unix shell in C

A small interactive shell written in C for the Extended Systems Programming Lab (ESPL) at Ben-Gurion University. It runs external programs, pipes them together, manages background processes with signals, and keeps a command history.

## Features

- **Command execution** — `fork` + `execvp`, foreground by default, background with `&`
- **I/O redirection** — `<` and `>` via `dup`
- **Pipelines** — `ls | wc -l` style two-process pipes; both children get their stdin/stdout rewired and the parent closes both pipe ends so EOF propagates correctly
- **Process manager** — child processes are tracked in a linked list:
  - `procs` — list processes with their status (running / suspended / terminated)
  - `zzzz <pid>` — suspend (`SIGSTOP`)
  - `kuku <pid>` — resume (`SIGCONT`)
  - `blast <pid>` — kill (`SIGINT`)
- **History** — last 10 commands in a circular buffer: `history`, `!!` (repeat last), `!n` (repeat n-th)
- **Built-ins** — `cd`, `quit`
- **Debug mode** — `./myshell -d` prints PIDs and commands to stderr
- No memory leaks (`make valgrind_shell`)

`mypipeline.c` is a standalone exercise: hard-wired `ls -lsa | tail -n 3` using `pipe`, `fork` and `dup`.

## Building

```bash
make
./myshell
```

`LineParser.c` / `LineParser.h` (the command-line parser) were provided by the course and are not included here.

## Tech
C · POSIX (`fork`, `execvp`, `pipe`, `dup`, `waitpid`, `kill`) · Linux · Make · Valgrind
