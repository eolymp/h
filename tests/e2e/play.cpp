#include <cstdio>
#include <cstdlib>
#include <string>

#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

std::string ending(int status) {
    if (WIFEXITED(status)) return std::to_string(WEXITSTATUS(status));
    if (WIFSIGNALED(status)) return "signal " + std::to_string(WTERMSIG(status));
    return "-1";
}

int main(int argc, char** argv) {
    bool const waiting = argc > 1 && std::string(argv[1]) == "--wait";
    if (waiting) {
        argv++;
        argc--;
    }
    if (argc < 3) {
        std::fprintf(stderr, "play [--wait] <interactor+args...> -- <solution>\n");
        return 2;
    }
    int split = 0;
    for (int at = 1; at < argc; at++)
        if (std::string(argv[at]) == "--") split = at;
    if (split == 0) return 2;

    argv[split] = nullptr;

    int to_interactor[2] = {-1, -1};
    int to_solution[2] = {-1, -1};
    if (::pipe(to_interactor) != 0 || ::pipe(to_solution) != 0) return 2;

    pid_t const jury = ::fork();
    if (jury == 0) {
        ::dup2(to_interactor[0], 0);
        ::dup2(to_solution[1], 1);
        for (int const one : {to_interactor[0], to_interactor[1], to_solution[0], to_solution[1]})
            if (one > 2) ::close(one);
        ::execv(argv[1], argv + 1);
        ::_exit(127);
    }

    pid_t const player = ::fork();
    if (player == 0) {
        ::signal(SIGPIPE, SIG_DFL);
        ::dup2(to_solution[0], 0);
        ::dup2(to_interactor[1], 1);
        for (int const one : {to_interactor[0], to_interactor[1], to_solution[0], to_solution[1]})
            if (one > 2) ::close(one);
        ::execv(argv[split + 1], argv + split + 1);
        ::_exit(127);
    }

    ::close(to_interactor[0]);
    ::close(to_interactor[1]);
    ::close(to_solution[0]);
    ::close(to_solution[1]);

    int jury_status = 0;
    int player_status = 0;
    ::waitpid(jury, &jury_status, 0);
    bool ended = false;
    for (int tries = 0; waiting && !ended && tries < 200; tries++) {
        ended = ::waitpid(player, &player_status, WNOHANG) == player;
        if (!ended) ::usleep(10000);
    }
    if (!ended) {
        ::kill(player, SIGKILL);
        ::waitpid(player, &player_status, 0);
    }
    if (waiting) {
        std::printf("interactor %s solution %s\n", ending(jury_status).c_str(), ending(player_status).c_str());
        return 0;
    }
    std::printf("interactor %d solution %d\n", WIFEXITED(jury_status) ? WEXITSTATUS(jury_status) : -1,
                WIFEXITED(player_status) ? WEXITSTATUS(player_status) : -1);
    return 0;
}
