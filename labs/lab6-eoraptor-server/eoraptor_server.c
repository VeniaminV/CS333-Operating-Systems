// Veniamin Velikoretskikh veniamin@pdx.edu
// CS 333 Lab 6 - Spring 2026
// eoraptor_server.c - UDP game server for the Eoraptor lunar lander game
// maintains all moon state, accepts client commands, broadcasts updates

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>


#include "eoraptor_ui.h"


#define OPTIONS          "P:vh"

// number of file descriptors we hand to poll()
#define N_FDS            3
#define STDIN_FD_POS     0
#define SOCKET_FD_POS    1
#define TIMER_FD_POS     2

// update rate: 60 Hz -> 16 666 667 ns per tick
#define TICK_NS          16666667L

// starting Y position for all landers
#define LANDER_START_Y   2.0f

// how far above centre the lander spawns horizontally
#define SPAWN_SPACING    (GRID_W / (MAX_PLAYERS + 1))

#define SRV_KEY_UP    259
#define SRV_KEY_DOWN  258
#define SRV_KEY_LEFT  260
#define SRV_KEY_RIGHT 261

// pilot letters map directly to player slot index
static const char PILOT_CHARS[MAX_PLAYERS] = { 'A', 'B', 'C', 'D' };


// set to false by SIGINT handler or console 'q' command
static volatile sig_atomic_t keep_running = true;

// the one and only moon; only the server owns this
static moon_state_t moon;

// one address slot per player; filled when they first connect
static struct sockaddr_in client_addrs[MAX_PLAYERS];
static bool client_connected[MAX_PLAYERS];


static void handle_sigint(int sig);
static void print_help(const char *prog);
static void initialize_terrain(void);
static void initialize_player(int slot);
static int  find_or_add_client(struct sockaddr_in *addr);
static void remove_client(int slot);
static void broadcast_moon(int sockfd);
static void process_command(int slot, int cmd, int sockfd);
static void tick_physics(void);
static void check_landing(int slot);
static void show_server_keys(void);
static void list_clients(void);
static void launch_all(void);
static void reset_all(void);


// signal handler
static void
handle_sigint(int sig)
{
        (void)sig;
        keep_running = false;
}


int
main(int argc, char *argv[])
{
        int opt;
        int port       = DEFAULT_PORT;
        int is_verbose = 0;

        int sockfd  = -1;
        int timerfd = -1;

        // poll array: stdin | socket | timerfd
        struct pollfd fds[N_FDS];
        struct itimerspec timer_spec;
        struct timespec   now;
        // socket addresses
        struct sockaddr_in srv_addr;
        struct sockaddr_in cli_addr;
        socklen_t          addrlen = sizeof(struct sockaddr_in);

        // signal action
        struct sigaction sa;

        // console input line buffer
        char console_buf[128];


        while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
                switch (opt) {
                case 'P':
                        sscanf(optarg, "%d", &port);
                        break;
                case 'v':
                        is_verbose++;
                        break;
                case 'h':
                        print_help(basename(argv[0]));
                        exit(EXIT_SUCCESS);
                        break;
                default:
                        fprintf(stderr, "Unknown option. Use -h for help.\n");
                        exit(1);
                }
        }

        // ------------------------------------------------------------------
        // signal handler - sigaction, not deprecated signal()
        memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);
        sa.sa_flags   = 0;
        sa.sa_handler = handle_sigint;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction SIGINT");
                exit(1);
        }

        if (is_verbose) {
                fprintf(stderr, "signal handler established\n");
        }

        // -----------------------------------------------------------------------
        // seed the RNG once, before initializing terrain
        srandom((unsigned int)(time(NULL) ^ getpid()));

       // ------------------------------------------------------------------
        // initialize moon state
        memset(&moon, 0, sizeof(moon_state_t));
        memset(client_addrs,     0, sizeof(client_addrs));
        memset(client_connected, 0, sizeof(client_connected));

        initialize_terrain();

        for (int i = 0; i < MAX_PLAYERS; i++) {
                initialize_player(i);
        }

        snprintf(moon.global_msg, GLOBAL_MSG_LEN, "Waiting for pilots...");

        if (is_verbose) {
                fprintf(stderr, "moon initialized\n");
        }

        // ---------------------------------------------------------------------
        // UDP socket

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd < 0) {
                perror("socket");
                exit(1);
        }

        memset(&srv_addr, 0, sizeof(srv_addr));
        srv_addr.sin_family      = AF_INET;
        srv_addr.sin_port        = htons(port);
        srv_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
                perror("bind failure");
                fprintf(stderr, "Port %d must already be in use.\n", port);
                fprintf(stderr, "Try another with the -P # command line option.\n");
                fprintf(stderr, "Remember to start your client with the same port.\n");
                close(sockfd);
                exit(1);
        }

        if (is_verbose) {
                fprintf(stderr, "socket bound on port %d\n", port);
        }

        // -----------------------------------------------------------------------
        // timerfd - 60 Hz game tick; replaces the poll() timeout parameter

        timerfd = timerfd_create(CLOCK_REALTIME, 0);

        if (timerfd < 0) {
                perror("timerfd_create");
                close(sockfd);
                exit(1);
        }

        clock_gettime(CLOCK_MONOTONIC, &now);

        timer_spec.it_value.tv_sec     = now.tv_sec;
        timer_spec.it_value.tv_nsec    = now.tv_nsec + TICK_NS;
        timer_spec.it_interval.tv_sec  = 0;
        timer_spec.it_interval.tv_nsec = TICK_NS;

        if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &timer_spec, NULL) < 0) {
                perror("timerfd_settime");
                close(sockfd);
                close(timerfd);
                exit(1);
        }

        if (is_verbose) {
                fprintf(stderr, "timerfd configured at 60 Hz\n");
        }

        // ---------------------------------------------------------------------
        // poll setup
        fds[STDIN_FD_POS].fd     = STDIN_FILENO;
        fds[STDIN_FD_POS].events = POLLIN;

        fds[SOCKET_FD_POS].fd     = sockfd;
        fds[SOCKET_FD_POS].events = POLLIN;

        fds[TIMER_FD_POS].fd     = timerfd;
        fds[TIMER_FD_POS].events = POLLIN;

        fprintf(stdout, "Eoraptor server running on port %d\n", port);
        fprintf(stdout, "Type 'h' for server console commands.\n");
        fflush(stdout);

        // -------------------------------------------------------------------
        // main loop
        while (keep_running) {
                int ret = poll(fds, N_FDS, -1); // -1: no timeout, timerfd drives ticks

                if (ret < 0) {
                        if (errno == EINTR) {
                                // a signal interrupted poll(), loop back to check keep_running
                                continue;
                        }
                        perror("poll");
                        break;
                }

                // -------------------------------------------------------------------
                // stdin: server console commands
                if (fds[STDIN_FD_POS].revents & POLLIN) {
                        if (fgets(console_buf, sizeof(console_buf), stdin) != NULL) {
                                char cmd = console_buf[0];

                                switch (cmd) {
                                case 'q':
                                case 'Q':
                                        fprintf(stdout, "Server shutting down.\n");
                                        keep_running = false;
                                        break;

                                case 'h':
                                        fprintf(stdout, "Server console commands:\n");
                                        fprintf(stdout, "  q/Q  Quit the server\n");
                                        fprintf(stdout, "  h    This help\n");
                                        fprintf(stdout, "  L    Launch all landers\n");
                                        fprintf(stdout, "  R    Reset all landers\n");
                                        fprintf(stdout, "  k    Show client key bindings\n");
                                        fprintf(stdout, "  l    List connected clients\n");
                                        fflush(stdout);
                                        break;

                                case 'L':
                                        launch_all();
                                        broadcast_moon(sockfd);
                                        break;

                                case 'R':
                                        reset_all();
                                        broadcast_moon(sockfd);
                                        break;

                                case 'k':
                                        show_server_keys();
                                        break;

                                case 'l':
                                        list_clients();
                                        break;

                                default:
                                        fprintf(stdout,
                                                "Unknown command '%c'. Type 'h' for help.\n",
                                                cmd);
                                        fflush(stdout);
                                        break;
                                }
                        }
                }

                // -------------------------------------------------------------------
                // socket: incoming client datagram
                if (fds[SOCKET_FD_POS].revents & POLLIN) {
                        int     cmd = 0;
                        ssize_t br;

                        br = recvfrom(sockfd, &cmd, sizeof(cmd), 0,
                                      (struct sockaddr *) &cli_addr, &addrlen);

                        if (br == (ssize_t)sizeof(cmd)) {
                                int slot = find_or_add_client(&cli_addr);

                                if (slot >= 0) {
                                        process_command(slot, cmd, sockfd);
                                        broadcast_moon(sockfd);
                                }
                                else {
                                        if (is_verbose) {
                                                fprintf(stderr,
                                                        "server full; rejected client\n");
                                        }
                                }
                        }
                }

                // -------------------------------------------------------------------
                // timerfd: game tick at 60 Hz

                if (fds[TIMER_FD_POS].revents & POLLIN) {
                        // must read the timerfd or it stays readable forever
                        uint64_t expirations = 0;
                        ssize_t  rb = read(timerfd, &expirations, sizeof(expirations));
                        (void)rb;

                        tick_physics();
                        broadcast_moon(sockfd);
                }
        }

        // ------------------------------------------------------------------
        // cleanup
        if (sockfd >= 0) {
                close(sockfd);
        }

        if (timerfd >= 0) {
                close(timerfd);
        }

        fprintf(stdout, "Eoraptor server exited cleanly.\n");

        return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// print_help
static void
print_help(const char *prog)
{
        printf("%s [%s]\n", prog, OPTIONS);
        printf("\t-P #  port on which to listen (default %d)\n", DEFAULT_PORT);
        printf("\t-v    verbose diagnostics to stderr\n");
        printf("\t-h    show this help and exit\n");
}

// ---------------------------------------------------------------------------
// initialize_terrain
// generates plausible lunar terrain with flat landing pads and mountains
static void
initialize_terrain(void)
{
        int x              = 0;
        int current_height = GRID_H - 4;

        while (x < GRID_W) {
                int feature_width = (int)(random() % 7) + 4;
                int is_flat  = (int)(random() % 10) < 6;
                int i;

                for (i = 0; i < feature_width && x < GRID_W; i++) {
                        if (!is_flat) {
                                current_height += (int)(random() % 3) - 1;

                                if (current_height < 5) {
                                        current_height = 5;
                                }

                                if (current_height > (GRID_H - 1)) {
                                        current_height = GRID_H - 1;
                                }
                        }

                        moon.terrain[x] = current_height;
                        x++;
                }
        }
}

// ---------------------------------------------------------------------------
// initialize_player
// sets a player slot to its default pre-flight state

static void
initialize_player(int slot)
{
        player_t *p = &moon.players[slot];

        memset(p, 0, sizeof(player_t));

        p->active       = false;
        p->id           = slot;
        p->x            = (float)(SPAWN_SPACING * (slot + 1));
        p->y            = LANDER_START_Y;
        p->velo_y       = 0.0f;
        p->fuel         = INITIAL_FUEL_MED;
        p->thrust_level = 0.0f;
        p->status       = PREFLIGHT;

        memset(p->private_msg, 0, PRIVATE_MSG_LEN);
}

// ---------------------------------------------------------------------------
// find_or_add_client
// returns the slot index for a known client or claims a free slot for a new one
// returns -1 if the server is full
static int
find_or_add_client(struct sockaddr_in *addr)
{
        int free_slot = -1;
        int i;

        for (i = 0; i < MAX_PLAYERS; i++) {
                if (client_connected[i]) {
                        if (client_addrs[i].sin_addr.s_addr == addr->sin_addr.s_addr &&
                            client_addrs[i].sin_port        == addr->sin_port) {
                                return i;
                        }
                }
                else if (free_slot == -1) {
                        free_slot = i;
                }
        }

        if (free_slot >= 0) {
                client_addrs[free_slot]        = *addr;
                client_connected[free_slot]    = true;
                moon.players[free_slot].active = true;
                moon.color                     = free_slot;
                moon.pilot                     = PILOT_CHARS[free_slot];

                snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                         "Pilot %c has entered the moon.", PILOT_CHARS[free_slot]);

                fprintf(stdout, "Client connected: slot %d  %s:%d\n",
                        free_slot,
                        inet_ntoa(addr->sin_addr),
                        ntohs(addr->sin_port));
                fflush(stdout);
        }

        return free_slot;
}

// -----------------------------------------------------------------------------
// remove_client
// marks a slot as free and resets the player within it
static void
remove_client(int slot)
{
        fprintf(stdout, "Client disconnected: slot %d  %s:%d\n",
                slot,
                inet_ntoa(client_addrs[slot].sin_addr),
                ntohs(client_addrs[slot].sin_port));
        fflush(stdout);

        client_connected[slot] = false;
        memset(&client_addrs[slot], 0, sizeof(struct sockaddr_in));

        initialize_player(slot);

        snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                 "Pilot %c has left the moon.", PILOT_CHARS[slot]);
}

// -------------------------------------------------------------------------
// broadcast_moon
// sends the current moon state to every connected client

static void
broadcast_moon(int sockfd)
{
        int i;

        for (i = 0; i < MAX_PLAYERS; i++) {
                if (!client_connected[i]) {
                        continue;
                }

                // set per-client fields so the dashboard highlights the right pilot
                moon.color = moon.players[i].id;
                moon.pilot = PILOT_CHARS[i];

                sendto(sockfd, &moon, sizeof(moon_state_t), 0,
                       (struct sockaddr *) &client_addrs[i],
                       sizeof(struct sockaddr_in));
        }
}

// ------------------------------------------------------------------------
// process_command
// handles a single keystroke from a client
static void
process_command(int slot, int cmd, int sockfd)
{
        player_t *p = &moon.players[slot];

        switch (cmd) {
        //movement: left
        case 'a':
        case 'h':
        case SRV_KEY_LEFT:
                p->x--;
                if (p->x <= 0) {
                        p->x = (float)(GRID_W - 2);
                }
                break;

        //movement: right
        case 'd':
        case 'l':
        case SRV_KEY_RIGHT:
                p->x++;
                if (p->x >= (float)(GRID_W - 1)) {
                        p->x = 1.0f;
                }
                break;

        //thrust: increase
        case 'w':
        case 'k':
        case SRV_KEY_UP:
                p->thrust_level += THROTTLE_INCREMENT_MED;
                if (p->thrust_level > MAX_THRUST_LEVEL) {
                        p->thrust_level = MAX_THRUST_LEVEL;
                }
                break;

        // thrust: decrease
        case 's':
        case 'j':
        case SRV_KEY_DOWN:
                p->thrust_level -= THROTTLE_INCREMENT_MED;
                if (p->thrust_level < 0.0f) {
                        p->thrust_level = 0.0f;
                }
                break;

        // cut thrust
        case ' ':
                p->thrust_level = 0.0f;
                break;

        //launch
        case 'L':
                if (p->status == PREFLIGHT) {
                        p->status = FLYING;
                        snprintf(p->private_msg, PRIVATE_MSG_LEN, "Launched!");
                        snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                                 "Pilot %c has launched!", PILOT_CHARS[slot]);
                }
                break;

        //reset
        case 'R':
                initialize_player(slot);
                moon.players[slot].active = true;
                snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                         "Pilot %c reset.", PILOT_CHARS[slot]);
                break;

        //quit
        case 'Q':
                remove_client(slot);
                broadcast_moon(sockfd);
                break;

        //introduction / no-op
        case '\r':
        case '\n':
                break;

        default:
                break;
        }
}

// ------------------------------------------------------------------------
// tick_physics
// called every 60 Hz tick; advances every flying lander by one simulation step
static void
tick_physics(void)
{
        int i;

        for (i = 0; i < MAX_PLAYERS; i++) {
                player_t *p = &moon.players[i];
                float actual_thrust;

                if (!p->active || p->status != FLYING) {
                        continue;
                }

                // run out of fuel: cut the engine
                if (p->fuel <= 0.0f) {
                        p->fuel         = 0.0f;
                        p->thrust_level = 0.0f;
                        snprintf(p->private_msg, PRIVATE_MSG_LEN, "OUT OF FUEL");
                }

                // compute actual thrust from throttle fraction
                actual_thrust = (p->thrust_level / MAX_THRUST_LEVEL) * MAX_THRUST;


                // apply gravity then subtract thrust
                p->velo_y += GRAVITY;
                p->velo_y -= actual_thrust;

                // advance position
                p->y += p->velo_y;

                // burn fuel proportional to throttle setting
                if (p->thrust_level > 0.0f) {
                        p->fuel -= p->thrust_level * FUEL_BURN_RATE;

                        if (p->fuel < 0.0f) {
                                p->fuel = 0.0f;
                        }
                }

                // warn when fuel is low
                if (p->fuel > 0.0f && p->fuel <= FUEL_LOW_WARNING) {
                        snprintf(p->private_msg, PRIVATE_MSG_LEN, "1202 LOW FUEL");
                }


                // keep the lander inside the top of the grid
                if (p->y < 1.0f) {
                        p->y      = 1.0f;
                        p->velo_y = 0.0f;
                }

                // check for terrain contact
                check_landing(i);
        }
}

// -----------------------------------------------------------------------------
// check_landing
// determines whether a lander has touched down; sets LANDED or CRASHED
static void
check_landing(int slot)
{
        player_t *p          = &moon.players[slot];
        int       ground_col = (int)p->x;
        float     ground_y;
        float     impact_velo;

        if (ground_col < 0) {
                ground_col = 0;
        }

        if (ground_col >= GRID_W) {
                ground_col = GRID_W - 1;
        }

        ground_y = (float)moon.terrain[ground_col];

        if (p->y >= ground_y) {
                // save velocity before zeroing it out
                impact_velo     = p->velo_y;
                p->y            = ground_y;
                p->velo_y       = 0.0f;
                p->thrust_level = 0.0f;

                if (impact_velo > VELO_Y_CRASH_THRESHOLD_MED) {
                        p->status = CRASHED;
                        snprintf(p->private_msg, PRIVATE_MSG_LEN, "CRASHED!");
                        snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                                 "Pilot %c CRASHED!", PILOT_CHARS[slot]);
                }
                else {
                        p->status = LANDED;
                        snprintf(p->private_msg, PRIVATE_MSG_LEN, "Landed safely!");
                        snprintf(moon.global_msg, GLOBAL_MSG_LEN,
                                 "Pilot %c landed safely!", PILOT_CHARS[slot]);
                }
        }
}

// ----------------------------------------------------------------------
// show_server_keys
// prints the client key bindings to the server console
static void
show_server_keys(void)
{
        printf("Client movement keys:\n");
        printf("  w / k / UP     Increase thrust\n");
        printf("  s / j / DOWN   Decrease thrust\n");
        printf("  a / h / LEFT   Move left\n");
        printf("  d / l / RIGHT  Move right\n");
        printf("  SPACE          Cut thrust\n");
        printf("Client other keys:\n");
        printf("  L    Launch lander\n");
        printf("  R    Reset lander\n");
        printf("  Q    Quit\n");
        fflush(stdout);
}

// -----------------------------------------------------------------------------
// list_clients 
// prints each connected client's IPv4 address and port
static void
list_clients(void)
{
        int count = 0;
        int i;

        printf("Connected clients:\n");

        for (i = 0; i < MAX_PLAYERS; i++) {
                if (client_connected[i]) {
                        printf("  slot %d  pilot %c  %s:%d\n",
                               i,
                               PILOT_CHARS[i],
                               inet_ntoa(client_addrs[i].sin_addr),
                               ntohs(client_addrs[i].sin_port));
                        count++;
                }
        }

        if (count == 0) {
                printf("  (none)\n");
        }

        fflush(stdout);
}

// ----------------------------------------------------------------------
// launch_all
// server console 'L': set all active preflight landers to FLYING
static void
launch_all(void)
{
        int i;

        for (i = 0; i < MAX_PLAYERS; i++) {
                if (moon.players[i].active &&
                    moon.players[i].status == PREFLIGHT) {
                        moon.players[i].status = FLYING;
                        snprintf(moon.players[i].private_msg, PRIVATE_MSG_LEN,
                                 "Launched!");
                }
        }

        snprintf(moon.global_msg, GLOBAL_MSG_LEN, "All pilots launched!");
        fprintf(stdout, "All landers launched.\n");
        fflush(stdout);
}

// ---------------------------------------------------------------------------
// reset_all
// server console 'R': return every active lander to its initial position
static void
reset_all(void)
{
        int i;

        for (i = 0; i < MAX_PLAYERS; i++) {
                if (moon.players[i].active) {
                        bool was_active = moon.players[i].active;

                        initialize_player(i);
                        moon.players[i].active = was_active;
                }
        }

        snprintf(moon.global_msg, GLOBAL_MSG_LEN, "All landers reset.");
        fprintf(stdout, "All landers reset.\n");
        fflush(stdout);
}
