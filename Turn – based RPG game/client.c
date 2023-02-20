#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <ctype.h>
#include "game.h"

char username[33];

// Authentication
int user_authentication(int client_fd);
int get_authentication_option();
int prepare_authentication_request(request_t *request, unsigned int authentication_option);

// Gameplay
int play_game(int client_fd);
int get_game_mode_option();
int create_game_character(request_t *request);
int get_player_movement(request_t *request);

// Both game modes
void print_abilities(const char *name, abilities_t *abilities);
void print_player_stats(response_t *response, unsigned int game_mode);
void print_player_abilities(response_t *response, unsigned int game_mode);
void print_monster_abilities(response_t *response, unsigned int game_mode);
void print_game_legend();

// Single-player only
void print_sp_maze(response_t *response);

// Multi-player only
int get_player_id_by_username(response_t *response, const char *username);
void assign_player_colors(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]);
void print_mp_maze(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]);
void print_player_color_legend(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]);

// Client-server communication
void send_quit_request(int client_fd, request_t *request, response_t *response);
void print_response(response_t *response);

// Auxiliary
void clear_screen();

int main(int argc, char **argv) {
    srand(time(NULL));

    // Check if server port was given
    if (argc != 2) {
        printf("\nUsage: %s <SERVER_PORT>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    const unsigned int server_port = atoi(argv[1]);
    int client_fd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    // Server address
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\n[ERROR] main/socket");
        return EXIT_FAILURE;
    }

    if (connect(client_fd, (struct sockaddr *) &server_addr, server_addr_len) == -1) {
        perror("\n[ERROR] main/connect");
        return EXIT_FAILURE;
    }

    if (user_authentication(client_fd) == -1) {
        return EXIT_FAILURE;
    }

    if (play_game(client_fd) == -1) {
        return EXIT_FAILURE;
    }

    close(client_fd);
    return EXIT_SUCCESS;
}

int user_authentication(int client_fd) {
    unsigned int authentication_option;
    request_t request;
    response_t response;

    if ((authentication_option = get_authentication_option()) == -1 || prepare_authentication_request(&request, authentication_option) == -1) {
        send_quit_request(client_fd, &request, &response);
        return -1;
    }
    
    send(client_fd, &request, sizeof(request_t), 0);
    recv(client_fd, &response, sizeof(response_t), 0);
    print_response(&response);

    if ((request.code == LOGIN && response.code != LOGIN_SUCCESSFUL) || (request.code == REGISTER && response.code != REGISTRATION_SUCCESSFUL)) {
        send_quit_request(client_fd, &request, &response);
        return -1;
    }

    // Save username for later requests
    strcpy(username, request.username);

    return 0;
}

int get_authentication_option() {
    const char *authentication_options[] = {
        "1. LOGIN",
        "2. REGISTER",
        "0. EXIT"
    };
    char user_option[3];
    int i;

    printf("Authentication menu:\n");
    for (i = 0; i < 3; i++) printf("\n%s", authentication_options[i]);
    printf("\n\nWhat would you like to do? (0-2): ");

    if (!fgets(user_option, sizeof(user_option), stdin)) return -1;

    if (user_option[0] == '1') return LOGIN;
    else if (user_option[0] == '2') return REGISTER;
    else return -1;
}

int prepare_authentication_request(request_t *request, unsigned int authentication_option) {
    if (authentication_option == LOGIN) request->code = LOGIN;
    else if (authentication_option == REGISTER) request->code = REGISTER;

    printf("\nUsername: ");
    if (!(fgets(request->username, sizeof(request->username), stdin))) return -1;
    request->username[strlen(request->username) - 1] = '\0';

    printf("Password: ");
    if (!(fgets(request->data.authentication.password, sizeof(request->data.authentication.password), stdin))) return -1;
    request->data.authentication.password[strlen(request->data.authentication.password) - 1] = '\0';

    return 0;
}

int play_game(int client_fd) {
    unsigned int game_mode, exit = 0;
    int movement;
    request_t request;
    response_t response;

    if ((game_mode = get_game_mode_option()) == -1) {
        send_quit_request(client_fd, &request, &response);
        return -1;
    }

    while (!exit) {
        clear_screen();

        request.code = (game_mode == SINGLE_PLAYER) ? GET_SINGLE_PLAYER_PROGRESS : GET_MULTI_PLAYER_PROGRESS;
        strcpy(request.username, username);

        send(client_fd, &request, sizeof(request_t), 0);
        recv(client_fd, &response, sizeof(response_t), 0);

        print_response(&response);

        switch (response.code) {
            case NO_CREATED_CHARACTER:
                request.code = (game_mode == SINGLE_PLAYER) ? CREATE_SINGLE_PLAYER_CHARACTER : CREATE_MULTI_PLAYER_CHARACTER;
                create_game_character(&request);

                send(client_fd, &request, sizeof(request_t), 0);
                recv(client_fd, &response, sizeof(response_t), 0);

                break;
            case SP_READY_TO_PLAY:
                request.code = UPDATE_SINGLE_PLAYER_PROGRESS;
                movement = get_player_movement(&request);

                if (movement == 1) {
                    send(client_fd, &request, sizeof(request_t), 0);
                    recv(client_fd, &response, sizeof(response_t), 0);
                } else if (movement == -1) {
                    exit = 1;
                }

                break;
            case SP_GAME_COMPLETE:
                exit = 1;
                break;
            case MP_READY_TO_PLAY:
                request.code = UPDATE_MULTI_PLAYER_PROGRESS;
                movement = get_player_movement(&request);

                if (movement == 1) {
                    send(client_fd, &request, sizeof(request_t), 0);
                    recv(client_fd, &response, sizeof(response_t), 0);
                } else if (movement == -1) {
                    exit = 1;
                }

                break;
            case MP_WAITING_FOR_PLAYERS:
                // Wait for all players to connect (receive MP_READY_TO_PLAY response)
                recv(client_fd, &response, sizeof(response), 0);

                if (response.code != MP_READY_TO_PLAY) {
                    send_quit_request(client_fd, &request, &response);
                    return -1;
                }

                break;
            case MP_GAME_IN_PROGRESS:
                exit = 1;
            default:
                send_quit_request(client_fd, &request, &response);
                return -1;
        }
    }

    send_quit_request(client_fd, &request, &response);

    return 0;
}

int get_game_mode_option() {
    const char *authentication_options[] = {
        "1. Single-player",
        "2. Multi-player",
        "0. EXIT"
    };
    char user_option[3];
    int i;

    printf("\nGame mode:\n");
    for (i = 0; i < 3; i++) printf("\n%s", authentication_options[i]);
    printf("\n\nSelect game mode (0-2): ");

    if (!fgets(user_option, sizeof(user_option), stdin)) return -1;

    if (user_option[0] == '1') return SINGLE_PLAYER;
    if (user_option[0] == '2') return MULTI_PLAYER;
    return -1;
}

int create_game_character(request_t *request) {
    int remaining_points = NEW_CHARACTER_POINTS;

    request->data.new_character.abilities.health = 100;
    request->data.new_character.abilities.armor = 0;
    request->data.new_character.abilities.attack = 0;
    request->data.new_character.abilities.accuracy = 0;

    printf("\nYou got %d points to improve your character abilities!\n", NEW_CHARACTER_POINTS);

    do {
        printf("Armor points (%d remaining): ", remaining_points);
        scanf("%d", &request->data.new_character.abilities.armor);
    } while (request->data.new_character.abilities.armor < 0 || remaining_points - request->data.new_character.abilities.armor < 0);
    remaining_points -= request->data.new_character.abilities.armor;

    if (remaining_points) {
        do {
            printf("Attack points (%d remaining): ", remaining_points);
            scanf("%d", &request->data.new_character.abilities.attack);
        } while (request->data.new_character.abilities.attack < 0 || remaining_points - request->data.new_character.abilities.attack < 0);
        remaining_points -= request->data.new_character.abilities.attack;
    }

    if (remaining_points) {
        do {
            printf("Accuracy points (%d remaining): ", remaining_points);
            scanf("%d", &request->data.new_character.abilities.accuracy);
        } while (request->data.new_character.abilities.accuracy < 0 || remaining_points - request->data.new_character.abilities.accuracy < 0);
        remaining_points -= request->data.new_character.abilities.accuracy;
    }

    while (remaining_points) {
        int ability = rand() % 3;

        switch (ability) {
            case 0:
                request->data.new_character.abilities.armor++;
                break;
            case 1:
                request->data.new_character.abilities.attack++;
                break;
            case 2:
                request->data.new_character.abilities.accuracy++;
        }

        remaining_points--;
    }

    return 0;
}

int get_player_movement(request_t *request) {
    system("/bin/stty raw");

    int fd = open("/dev/tty", O_NONBLOCK);
    char key = '\0';

    sleep(1);
    if (read(fd, &key, sizeof(key))) key = toupper(key);

    system("/bin/stty cooked");

    switch (key) {
        case MOVE_UP_KEY:
        case MOVE_DOWN_KEY:
        case MOVE_LEFT_KEY:
        case MOVE_RIGHT_KEY:
            request->data.play.movement_key = key;
            return 1;
        case ESC_KEY:
            return -1;
        default:
            return 0;
    }
}

void print_abilities(const char *name, abilities_t *abilities) {
    printf("[!] Name: %s - Health: %d - Armor: %d - Attack: %d - Accuracy: %d\n", name, abilities->health, abilities->armor, abilities->attack, abilities->accuracy);
}

void print_player_stats(response_t *response, unsigned int game_mode) {
    int player_id;

    if (game_mode == SINGLE_PLAYER) {
        printf("\n[!] Level: %d - Wins: %d - Defeats: %d\n", response->characters[0].level, response->characters[0].wins, response->characters[0].defeats);
    } else {
        player_id = get_player_id_by_username(response, username);
        printf("\n[!] Level: %d - Wins: %d - Defeats: %d\n", response->characters[player_id].level, response->characters[player_id].wins, response->characters[player_id].defeats);
    }
}

void print_player_abilities(response_t *response, unsigned int game_mode) {
    int i;

    if (game_mode == SINGLE_PLAYER) {
        print_abilities(response->characters[0].name, &response->characters[0].abilities);
    } else {
        for (i = 0; i < MULTIPLAYER_PLAYERS; i++) print_abilities(response->characters[i].name, &response->characters[i].abilities);
    }
}

void print_monster_abilities(response_t *response, unsigned int game_mode) {
    int player_id;
    position_t player_position;
    position_t monster_position;
    int i;

    if (game_mode == SINGLE_PLAYER) {
        player_position = response->characters[0].position;
    } else {
        player_id = get_player_id_by_username(response, username);
        player_position = response->characters[player_id].position;
    }

    // Needed for the 'if' condition before the 'for' loop, so that 'monsters' array won't be iterated unnecessarily
    monster_position.x = monster_position.y = 0;

    if (response->maze.map[player_position.x - 1][player_position.y] == MAZE_MONSTER ||
        response->maze.map[player_position.x - 1][player_position.y] == MAZE_MONSTER_BOSS) {
        monster_position.x = player_position.x - 1;
        monster_position.y = player_position.y;
    } else if ( response->maze.map[player_position.x][player_position.y] != MAZE_HORIZONTAL_WALL &&
                response->maze.map[player_position.x + 1][player_position.y] == MAZE_MONSTER ||
                response->maze.map[player_position.x + 1][player_position.y] == MAZE_MONSTER_BOSS ) {
        monster_position.x = player_position.x + 1;
        monster_position.y = player_position.y;
    } else if ( response->maze.map[player_position.x][player_position.y - 1] == MAZE_MONSTER ||
                response->maze.map[player_position.x][player_position.y - 1] == MAZE_MONSTER_BOSS ) {
        monster_position.x = player_position.x;
        monster_position.y = player_position.y - 1;
    } else if ( response->maze.map[player_position.x][player_position.y + 1] == MAZE_MONSTER ||
                response->maze.map[player_position.x][player_position.y + 1] == MAZE_MONSTER_BOSS ) {
        monster_position.x = player_position.x;
        monster_position.y = player_position.y + 1;
    }

    if (monster_position.x && monster_position.y) {
        for (i = 0; i < MAZE_MONSTER_COUNT; i++) {
            if (response->maze.monsters[i].position.y == monster_position.x &&
                response->maze.monsters[i].position.x == monster_position.y) {
                    break;
            }
        }

        if (i < MAZE_MONSTER_COUNT) print_abilities("MONSTER", &response->maze.monsters[i].abilities);
        else print_abilities("MONSTER BOSS", &response->maze.monster_boss.abilities);
    }
}

void print_game_legend() {
    printf("\n[!] Press %c to move up, %c to move down, %c to move left, %c to move right", MOVE_UP_KEY, MOVE_DOWN_KEY, MOVE_LEFT_KEY, MOVE_RIGHT_KEY);
    printf("\n[!] Press ESC to quit\n");
}

void print_sp_maze(response_t *response) {
    int i, j;

    for (i = 0; i < MAZE_HEIGHT + 1; i++) {
        for (j = 0; j < MAZE_WIDTH * 2 + 1; j++) {
            if (i == response->characters[0].position.x && j == response->characters[0].position.y) printf("%s", ANSI_COLORS[0]);
            printf("%c", response->maze.map[i][j]);
            printf(ANSI_RESET);
        }
        printf("\n");
    }

    printf("\n");
}

int get_player_id_by_username(response_t *response, const char *username) {
    int i;

    for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
        if (!strcmp(response->characters[i].name, username)) return i;
    }

    return -1;
}

void assign_player_colors(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]) {
    int i, j, color_id = 0, same_color;

    for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
        if (i > 0) {
            // If there is any other player on the same position, assign the same color to them
            same_color = 0;

            for (j = 0; j < i; j++) {
                if (response->characters[i].position.x == response->characters[j].position.x &&
                    response->characters[i].position.y == response->characters[j].position.y) {
                        same_color = 1;
                        break;
                }
            }

            if (same_color) {
                player_colors[i] = player_colors[j];
            } else {
                color_id++;
                player_colors[i] = color_id;
            }
        } else {
            // The first player - assign the first color to him
            player_colors[i] = color_id;
        }
    }
}

void print_mp_maze(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]) {
    int i, j, k;

    for (i = 0; i < MAZE_HEIGHT + 1; i++) {
        for (j = 0; j < MAZE_WIDTH * 2 + 1; j++) {
            for (k = 0; k < MULTIPLAYER_PLAYERS; k++) {
                if (i == response->characters[k].position.x && j == response->characters[k].position.y) {
                    printf("%s", ANSI_COLORS[player_colors[k]]);
                    break;
                }
            }

            printf("%c", response->maze.map[i][j]);
            printf(ANSI_RESET);
        }
        printf("\n");
    }

    printf("\n");
}

void print_player_color_legend(response_t *response, player_color_t player_colors[MULTIPLAYER_PLAYERS]) {
    int i, j, color_exists;

    for (i = 0; i < 3; i++) {
        color_exists = 0;

        for (j = 0; j < MULTIPLAYER_PLAYERS; j++) {
            if (player_colors[j] == i) {
                color_exists = 1;
                break;
            }
        }

        if (color_exists) {
            printf("%s %s: ", ANSI_COLORS[i], ANSI_RESET);
            for (j = 0; j < MULTIPLAYER_PLAYERS; j++) {
                if (player_colors[j] == i) printf("%s ", response->characters[j].name);
            }
        }

        printf("     ");
    }

    printf("\n");
}

void send_quit_request(int client_fd, request_t *request, response_t *response) {
    request->code = QUIT;
    send(client_fd, request, sizeof(request_t), 0);
    recv(client_fd, response, sizeof(response_t), 0);
}

void print_response(response_t *response) {
    player_color_t player_colors[MULTIPLAYER_PLAYERS];

    switch (response->code) {
        case SERVER_ERROR:
            printf("\n%s\n", RESPONSE_SERVER_ERROR);
            break;
        case UNKNOWN_USERNAME:
            printf("\n%s\n", RESPONSE_UNKNOWN_USERNAME);
            break;
        case INCORRECT_PASSWORD:
            printf("\n%s\n", RESPONSE_INCORRECT_PASSWORD);
            break;
        case LOGIN_SUCCESSFUL:
            printf("\n%s\n", RESPONSE_LOGIN_SUCCESSFULL);
            break;
        case USERNAME_EXISTS:
            printf("\n%s\n", RESPONSE_USERNAME_EXISTS);
            break;
        case REGISTRATION_SUCCESSFUL:
            printf("\n%s\n", RESPONSE_REGISTRATION_SUCCESSFUL);
            break;
        case NO_CREATED_CHARACTER:
            printf("\n%s\n", RESPONSE_NO_CREATED_CHARACTER);
            break;
        case CHARACTER_CREATED:
            printf("\n%s\n", RESPONSE_CHARACTER_CREATED);
            break;
        case SP_READY_TO_PLAY:
            print_sp_maze(response);
            print_player_stats(response, SINGLE_PLAYER);
            print_player_abilities(response, SINGLE_PLAYER);
            print_monster_abilities(response, SINGLE_PLAYER);
            print_game_legend();
            break;
        case SP_GAME_COMPLETE:
            printf("\n%s\n", RESPONSE_SP_GAME_COMPLETE);
            break;
        case MP_READY_TO_PLAY:
            assign_player_colors(response, player_colors);
            print_mp_maze(response, player_colors);
            print_player_color_legend(response, player_colors);
            print_player_stats(response, MULTI_PLAYER);
            print_player_abilities(response, MULTI_PLAYER);
            print_monster_abilities(response, MULTI_PLAYER);
            print_game_legend();
            break;
        case MP_WAITING_FOR_PLAYERS:
            printf("\n%s\n", RESPONSE_MP_WAITING_FOR_PLAYERS);
            break;
        case MP_GAME_IN_PROGRESS:
            printf("\n%s\n", RESPONSE_MP_GAME_IN_PROGRESS);
            break;
        default:
            printf("\n[ERROR] Unknown response code (%d)\n", response->code);
    }
}

void clear_screen() {
    printf(ANSI_CLEAR);
}