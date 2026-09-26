#include <cstdio>
#include <cstdlib>
#include <string>

#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "play <interactor+args...> -- <solution>\n");
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
        ::close(to_interactor[1]);
        ::close(to_solution[0]);
        ::execv(argv[1], argv + 1);
        ::_exit(127);
    }

    pid_t const player = ::fork();
    if (player == 0) {
        ::dup2(to_solution[0], 0);
        ::dup2(to_interactor[1], 1);
        ::close(to_solution[1]);
        ::close(to_interactor[0]);
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
    ::kill(player, SIGKILL);
    ::waitpid(player, &player_status, 0);
    std::printf("interactor %d solution %d\n", WIFEXITED(jury_status) ? WEXITSTATUS(jury_status) : -1,
                WIFEXITED(player_status) ? WEXITSTATUS(player_status) : -1);
    return 0;
}
