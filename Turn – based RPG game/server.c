#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "game.h"

#define USERS_DIR   "./users/"
#define MAZES_DIR   "./mazes/"

// Maze attributes
enum direction { N = 1, S = 2, E = 4, W = 8 };
unsigned dirMap[] = { N, S, E, W };

// Multiplayer
multiplayer_t multiplayer;
pthread_mutex_t multiplayer_mutex;

// Server functions
int check_directories();
int generate_mazes();

// Thread functions
void *thread_function(void *socket_fd);
int authenticate_user(request_t *request, response_t *response, unsigned int authentication_type);
int get_player_progress(request_t *request, response_t *response, unsigned int game_mode);
int update_player_progress(request_t *request, response_t *response, unsigned int game_mode);
int create_player_character(request_t *request, response_t *response, unsigned int game_mode);

// Maze
void carve_maze_path(int x, int y, unsigned int maze[MAZE_WIDTH][MAZE_HEIGHT]);
void create_maze(unsigned int original_maze[MAZE_WIDTH][MAZE_HEIGHT], maze_t *maze, unsigned int maze_id);
int save_maze(maze_t *maze, unsigned int maze_id);
int load_maze(maze_t *maze, unsigned int maze_id);

// Player
int initialize_player(player_t *player, request_t *request);
int save_player(player_t *player, request_t *request, response_t *response);
int load_player(player_t *player, request_t *request, response_t *response);
int update_player(player_t *player, request_t *request, response_t *response);

// Game
int maze_battle(abilities_t *attacker_abilities, abilities_t *victim_abilities);
int increase_game_level(player_t *player, unsigned int game_mode);
int reset_player_progress(player_t *player, unsigned int game_mode);

// Multi-player
void initialize_multiplayer();
int get_player_id_by_username(const char *username);

int main(int argc, char **argv) {
    srand(time(NULL));

    // Check if server port was given
    if (argc != 2) {
        printf("\nUsage: %s <PORT>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    const unsigned int server_port = atoi(argv[1]);
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len = sizeof(server_addr), client_addr_len = sizeof(client_addr);
    pthread_t pthread;

    // Check server directories
    if (check_directories() == -1) return EXIT_FAILURE;

    // Initialize multiplayer
    initialize_multiplayer();

    if (pthread_mutex_init(&multiplayer_mutex, NULL) != 0) {
        perror("\n[ERROR] pthread_mutex_init");
        return EXIT_FAILURE;
    }

    // Server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\n[ERROR] main/socket");
        return EXIT_FAILURE;
    }

    // Server address
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    // Socket binding
    if (bind(server_fd, (struct sockaddr *) &server_addr, server_addr_len) == -1) {
        perror("\n[ERROR] main/bind");
        return EXIT_FAILURE;
    }

    // Socket listening
    if (listen(server_fd, 10) == -1) {
        perror("\n[ERROR] main/listen");
        return EXIT_FAILURE;
    }

    printf("[INFO] Server is running on port %u...\n", server_port);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
            perror("\n[ERROR] main/accept");
            continue;
        }

        if (pthread_create(&pthread, NULL, thread_function, &client_fd) != 0) {
            perror("\n[ERROR] main/pthread_create");
            continue;
        }
    }

    close(server_fd);

    // Destroy multiplayer mutex
    if (pthread_mutex_destroy(&multiplayer_mutex) != 0) {
        perror("\n[ERROR] pthread_mutex_destroy");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int check_directories() {
    struct stat st; // Used by 'stat' function call

    // Check if users directory exists - create if not
    if (stat(USERS_DIR, &st) == -1) {
        if (mkdir(USERS_DIR, 0700) == -1) {
            perror("\n[ERROR] check_directories/mkdir (users)");
            return -1;
        }
    }

    // Check if maps directory exists - create if not
    if (stat(MAZES_DIR, &st) == -1) {
        if (mkdir(MAZES_DIR, 0700) == -1) {
            perror("\n[ERROR] check_directories/mkdir (maps)");
            return -1;
        }

        if (generate_mazes() == -1) {
            fprintf(stderr, "\n[ERROR] check_directories/generate_mazes\n");
            return -1;
        }
    }

    return 0;
}

int generate_mazes() {
    int i;

    for (i = 0; i < MAZE_COUNT; i++) {
        unsigned original_maze[MAZE_WIDTH][MAZE_HEIGHT];
        maze_t maze;

        memset(original_maze, 0, sizeof(original_maze));
        carve_maze_path(0, 0, original_maze);
        create_maze(original_maze, &maze, i + 1);
        save_maze(&maze, i + 1);
    }

    return 0;
}

void *thread_function(void *socket_fd) {
    int client_fd = *((int *) socket_fd), exit = 0, i, is_player_connected, character_loaded;
    request_t request;
    response_t response;

    while (!exit) {
        recv(client_fd, &request, sizeof(request), 0);

        switch (request.code) {
            case LOGIN:
                authenticate_user(&request, &response, LOGIN);
                break;
            case REGISTER:
                authenticate_user(&request, &response, REGISTER);
                break;
            case GET_SINGLE_PLAYER_PROGRESS:
                get_player_progress(&request, &response, SINGLE_PLAYER);
                break;
            case GET_MULTI_PLAYER_PROGRESS:
                is_player_connected = 0;
                character_loaded = 0;

                pthread_mutex_lock(&multiplayer_mutex);

                for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
                    if (!strcmp(multiplayer.characters[i].name, request.username)) {
                        is_player_connected = 1;
                        break;
                    }
                }

                if (multiplayer.connected_players < MULTIPLAYER_PLAYERS) {
                    if (!is_player_connected) {
                        response.code = NO_CREATED_CHARACTER;
                    } else {
                        character_loaded = 1;
                        multiplayer.connected_players++;
                    }

                    if (character_loaded) {
                        pthread_mutex_unlock(&multiplayer_mutex);
                        response.code = MP_WAITING_FOR_PLAYERS;
                        send(client_fd, &response, sizeof(response), 0);

                        while (1) {
                            pthread_mutex_lock(&multiplayer_mutex);
                            if (multiplayer.connected_players == MULTIPLAYER_PLAYERS) break;
                            pthread_mutex_unlock(&multiplayer_mutex);
                        }

                        response.code = MP_READY_TO_PLAY;
                        response.maze = multiplayer.maze;
                        memcpy(response.characters, multiplayer.characters, MULTIPLAYER_PLAYERS * sizeof(character_t));
                    }
                } else {
                    if (!is_player_connected) {
                        response.code = MP_GAME_IN_PROGRESS;
                    } else {
                        response.code = MP_READY_TO_PLAY;
                        response.maze = multiplayer.maze;
                        memcpy(response.characters, multiplayer.characters, MULTIPLAYER_PLAYERS * sizeof(character_t));
                    }
                }

                pthread_mutex_unlock(&multiplayer_mutex);

                break;
            case UPDATE_SINGLE_PLAYER_PROGRESS:
                update_player_progress(&request, &response, SINGLE_PLAYER);
                break;
            case UPDATE_MULTI_PLAYER_PROGRESS:
                pthread_mutex_lock(&multiplayer_mutex);

                if (update_player_progress(&request, &response, MULTI_PLAYER) == 0) {
                    response.code = MP_READY_TO_PLAY;
                    response.maze = multiplayer.maze;
                    memcpy(response.characters, multiplayer.characters, MULTIPLAYER_PLAYERS * sizeof(character_t));
                }

                pthread_mutex_unlock(&multiplayer_mutex);

                break;
            case CREATE_SINGLE_PLAYER_CHARACTER:
                create_player_character(&request, &response, SINGLE_PLAYER);
                break;
            case CREATE_MULTI_PLAYER_CHARACTER:
                pthread_mutex_lock(&multiplayer_mutex);

                if (multiplayer.connected_players < MULTIPLAYER_PLAYERS) {
                    strcpy(multiplayer.characters[multiplayer.connected_players].name, request.username);
                    create_player_character(&request, &response, MULTI_PLAYER);
                } else {
                    response.code = MP_GAME_IN_PROGRESS;
                }

                pthread_mutex_unlock(&multiplayer_mutex);
                
                break;
            case QUIT:
                exit = 1;
                break;
            default:
                printf("\n[ERROR] Socket: %d - Unknown request code (%d)\n", client_fd, request.code);
        }

        send(client_fd, &response, sizeof(response), 0);
    }

    close(client_fd);
    pthread_exit(NULL);
}

int authenticate_user(request_t *request, response_t *response, unsigned int authentication_type) {
    player_t player;

    if (authentication_type == LOGIN) {
        if (load_player(&player, request, response) == -1) return -1;

        if (strcmp(player.password, request->data.authentication.password)) {
            response->code = INCORRECT_PASSWORD;
            return 0;
        }

        response->code = LOGIN_SUCCESSFUL;
    } else {
        initialize_player(&player, request);
        if (save_player(&player, request, response) == -1) return -1;

        response->code = REGISTRATION_SUCCESSFUL;
    }

    return 0;
}

int get_player_progress(request_t *request, response_t *response, unsigned int game_mode) {
    player_t player;
    character_t *character;
    int player_id;

    if (game_mode != SINGLE_PLAYER) return -1;
    
    if (load_player(&player, request, response) == -1) return -1;
    character = &player.single.character;

    if (character->level == 0) {
        response->code = NO_CREATED_CHARACTER;
        return -1;
    }

    response->code = SP_READY_TO_PLAY;
    response->maze = player.single.maze;
    response->characters[0] = player.single.character;

    return 0;
}

int update_player_progress(request_t *request, response_t *response, unsigned int game_mode) {
    player_t player;
    character_t *character;
    maze_t *maze;
    int player_id;
    int i, old_x, old_y, new_x, new_y, mp_character_id;

    if (game_mode == SINGLE_PLAYER) {
        if (load_player(&player, request, response) == -1) return -1;
        character = &player.single.character;

        if (character->level == 0) {
            response->code = NO_CREATED_CHARACTER;
            return -1;
        } else if (character->level > MAZE_COUNT) {
            response->code = SP_GAME_COMPLETE;
            return -1;
        }

        maze = &player.single.maze;
    } else {
        if ((player_id = get_player_id_by_username(request->username)) == -1) return -1;
        character = &multiplayer.characters[player_id];

        if (character->level == 0) {
            response->code = NO_CREATED_CHARACTER;
            return -1;
        }

        maze = &multiplayer.maze;
    }

    old_x = new_x = character->position.x;
    old_y = new_y = character->position.y;

    // Check if player is allowed to move or if there is any barrier on his way
    switch (request->data.play.movement_key) {
        case MOVE_UP_KEY:
            new_x--;
            if (new_x == 0) return 0;
            if (maze->map[new_x][new_y] == MAZE_HORIZONTAL_WALL) return 0;
            if (maze->map[new_x][new_y] == MAZE_VERTICAL_WALL) return 0;
            break;
        case MOVE_DOWN_KEY:
            new_x++;
            if (maze->map[old_x][old_y] == MAZE_HORIZONTAL_WALL) return 0;
            if (maze->map[new_x][new_y] == MAZE_VERTICAL_WALL) return 0;
            break;
        case MOVE_LEFT_KEY:
            new_y--;
            if (maze->map[new_x][new_y] == MAZE_VERTICAL_WALL) return 0;
            break;
        case MOVE_RIGHT_KEY:
            new_y++;
            if (maze->map[new_x][new_y] == MAZE_VERTICAL_WALL) return 0;
    }

    // Check if player moved on any treasure
    if (maze->map[new_x][new_y] == MAZE_TREASURE) {
        // Find this specific treasure based on its position on the maze
        for (i = 0; i < MAZE_TREASURE_COUNT; i++) {
            if (maze->treasures[i].position.y == new_x &&
                maze->treasures[i].position.x == new_y) {
                    break;
            }
        }

        // Award player with treasure's points
        switch (maze->treasures[i].ability) {
            case HEALTH:
                character->abilities.health += maze->treasures[i].points;
                if (character->abilities.health > ABILITY_MAX_POINTS) {
                    character->abilities.health = ABILITY_MAX_POINTS;
                }

                break;
            case ARMOR:
                character->abilities.armor += maze->treasures[i].points;
                if (character->abilities.armor > ABILITY_MAX_POINTS) {
                    character->abilities.armor = ABILITY_MAX_POINTS;
                }

                break;
            case ATTACK:
                character->abilities.attack += maze->treasures[i].points;
                if (character->abilities.attack > ABILITY_MAX_POINTS) {
                    character->abilities.attack = ABILITY_MAX_POINTS;
                }

                break;
            case ACCURACY:
                character->abilities.accuracy += maze->treasures[i].points;
                if (character->abilities.accuracy > ABILITY_MAX_POINTS) {
                    character->abilities.accuracy = ABILITY_MAX_POINTS;
                }
        }

        // Remove the treasure from the maze and move player
        maze->map[new_x][new_y] = MAZE_EMPTY_POINT;
        character->position.x = new_x;
        character->position.y = new_y;
    } else if (maze->map[new_x][new_y] == MAZE_MONSTER) {
        // Find this specific monster based on its position on the maze
        for (i = 0; i < MAZE_MONSTER_COUNT; i++) {
            if (maze->monsters[i].position.y == new_x &&
                maze->monsters[i].position.x == new_y) {
                    break;
            }
        }

        // Check if the monster died after player's attack
        if (maze_battle(&character->abilities, &maze->monsters[i].abilities) == 1) {
            maze->map[new_x][new_y] = MAZE_EMPTY_POINT;
            character->position.x = new_x;
            character->position.y = new_y;
        } else {
            // Check if the player died after monster's attack
            if (maze_battle(&maze->monsters[i].abilities, &character->abilities) == 1) {
                character->defeats++;
                reset_player_progress(&player, game_mode);
            }
        }
    } else if (maze->map[new_x][new_y] == MAZE_MONSTER_BOSS) {
        // Check if the monster boss died after player's attack
        if (maze_battle(&character->abilities, &maze->monster_boss.abilities) == 1) {
            character->wins++;

            if (game_mode == SINGLE_PLAYER) {
                if (character->level <= MAZE_COUNT) {
                    increase_game_level(&player, game_mode);
                } else {
                    response->code = SP_GAME_COMPLETE;
                }
            } else {
                if (multiplayer.level <= MAZE_COUNT) {
                    increase_game_level(&player, game_mode);
                } else {
                    initialize_multiplayer();
                    response->code = MP_GAME_COMPLETE;
                }
            }
        } else {
            // Check if the player died after monster boss's attack
            if (maze_battle(&maze->monster_boss.abilities, &character->abilities) == 1) {
                character->defeats++;
                reset_player_progress(&player, game_mode);
            }
        }
    } else {
        // Just update player's position
        character->position.x = new_x;
        character->position.y = new_y;
    }

    if (game_mode == SINGLE_PLAYER) {
        response->code = SP_READY_TO_PLAY;
        response->characters[0] = *character;
    } else {
        for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
            if (!strcmp(multiplayer.characters[i].name, request->username)) {
                mp_character_id = i;
                break;
            }
        }

        multiplayer.characters[mp_character_id] = *character;
    }

    // Finally, update player's data
    if (update_player(&player, request, response) == -1) return -1;

    return 0;
}

int create_player_character(request_t *request, response_t *response, unsigned int game_mode) {
    player_t player;
    character_t *character;
    int player_id;

    if (game_mode == SINGLE_PLAYER) {
        if (load_player(&player, request, response) == -1) return -1;
        character = &player.single.character;
    } else {
        if ((player_id = get_player_id_by_username(request->username)) == -1) return -1;
        character = &multiplayer.characters[player_id];
    }

    // Set character's name as player's username
    strcpy(character->name, player.username);

    // Initialize character's stats
    character->wins = 0;
    character->defeats = 0;
    character->level = 1;

    // Initialize character's position
    character->position.x = 1;
    character->position.y = 1;

    // Initialize character's abilities
    character->initial_abilities.health = character->abilities.health = ABILITY_MAX_POINTS;
    character->initial_abilities.armor = character->abilities.armor = request->data.new_character.abilities.armor;
    character->initial_abilities.attack = character->abilities.attack = request->data.new_character.abilities.attack;
    character->initial_abilities.accuracy = character->abilities.accuracy = request->data.new_character.abilities.accuracy;

    // Load first game level (maze) in single-player mode
    if (game_mode == SINGLE_PLAYER) {
        if (load_maze(&player.single.maze, character->level) == -1) return -1;
        if (update_player(&player, request, response) == -1) return -1;
    }

    response->code = CHARACTER_CREATED;

    return 0;
}

void carve_maze_path(int x, int y, unsigned int maze[MAZE_WIDTH][MAZE_HEIGHT]) {
    int nx, ny, i, j = rand() % 4;
    
    for(i = 0; i < 4; i++, j = (j + 1) % 4) {
        nx = x, ny = y;
        switch(dirMap[j]) {
            case N: ny--; break;
            case S: ny++; break;
            case E: nx++; break;
            case W: nx--; break;
        }

        if (nx < MAZE_HEIGHT && nx >= 0 && ny < MAZE_HEIGHT && ny >= 0 && !maze[nx][ny]) {
            maze[x][y] |= dirMap[j];
            maze[nx][ny] |= dirMap[j + (j & 1 ? -1 : 1)];
            carve_maze_path(nx, ny, maze);
        }
    }
}

void create_maze(unsigned int original_maze[MAZE_WIDTH][MAZE_HEIGHT], maze_t *maze, unsigned int maze_id) {
    int x, y, i;
    int rand_y, rand_x;
    
    // Maze's upper wall
    maze->map[0][0] = MAZE_EMPTY_POINT;
    for(x = 0; x < (MAZE_WIDTH * 2) - 1; x++) maze->map[0][x + 1] = MAZE_HORIZONTAL_WALL;

    // The rest maze
    for(y = 0; y < MAZE_HEIGHT; y++) {
        maze->map[y + 1][0] = MAZE_VERTICAL_WALL;

        for(x = 0; x < MAZE_WIDTH; x++) {
            if (original_maze[x][y] & S) maze->map[y + 1][x * 2 + 1] = MAZE_EMPTY_POINT;
            else maze->map[y + 1][x * 2 + 1] = MAZE_HORIZONTAL_WALL;

            if(original_maze[x][y] & E){
                if ((original_maze[x][y] | original_maze[x + 1][y]) & S) maze->map[y + 1][x * 2 + 2] = MAZE_EMPTY_POINT;
                else maze->map[y + 1][x * 2 + 2] = MAZE_HORIZONTAL_WALL;
            } else {
                maze->map[y + 1][x * 2 + 2] = MAZE_VERTICAL_WALL;
            }
        }
    }

    // Show maze entrance
    maze->map[0][1] = MAZE_EMPTY_POINT;
    maze->map[0][MAZE_WIDTH * 2] = MAZE_EMPTY_POINT;

    // Add monster boss
    maze->map[MAZE_HEIGHT][MAZE_WIDTH * 2 - 1] = MAZE_MONSTER_BOSS;
    maze->map[MAZE_HEIGHT][MAZE_WIDTH * 2] = MAZE_VERTICAL_WALL;
    maze->monster_boss.position.x = MAZE_WIDTH * 2;
    maze->monster_boss.position.y = MAZE_HEIGHT;
    maze->monster_boss.abilities.health = ABILITY_MAX_POINTS;
    maze->monster_boss.abilities.armor = ABILITY_MAX_POINTS;
    maze->monster_boss.abilities.attack = maze_id * 10;
    maze->monster_boss.abilities.accuracy = maze_id * 10;

    // Add treasures
    for (i = 0; i < MAZE_TREASURE_COUNT; i++) {
        do {
            rand_y = rand() % (MAZE_HEIGHT - 1) + 1;
            rand_x = rand() % (MAZE_WIDTH * 2) + 1;
        } while (maze->map[rand_y][rand_x] != MAZE_EMPTY_POINT);   // TODO

        maze->map[rand_y][rand_x] = MAZE_TREASURE;
        maze->treasures[i].position.x = rand_x;
        maze->treasures[i].position.y = rand_y;
        maze->treasures[i].ability = rand() % ABILITY_COUNT;
        maze->treasures[i].points = rand() % ABILITY_MAX_POINTS + 1;
    }

    // Add monsters
    for (i = 0; i < MAZE_MONSTER_COUNT; i++) {
        do {
            rand_y = rand() % (MAZE_HEIGHT - 1) + 1;
            rand_x = rand() % (MAZE_WIDTH * 2) + 1;
        } while (maze->map[rand_y][rand_x] != MAZE_EMPTY_POINT);   // TODO

        maze->map[rand_y][rand_x] = MAZE_MONSTER;
        maze->monsters[i].position.x = rand_x;
        maze->monsters[i].position.y = rand_y;
        maze->monsters[i].abilities.health = maze_id * 10;
        maze->monsters[i].abilities.armor = maze_id * 10;
        maze->monsters[i].abilities.attack = maze_id * 10;
        maze->monsters[i].abilities.accuracy = maze_id * 10;
    }
}

int save_maze(maze_t *maze, unsigned int maze_id) {
    char file_path[64];
    FILE* fp;

    sprintf(file_path, "%smaze%d.txt", MAZES_DIR, maze_id);

    if (!(fp = fopen(file_path, "w"))) {
        perror("\n[ERROR] save_maze/fopen");
        return -1;
    }

    if (fwrite(maze, sizeof(maze_t), 1, fp) == 0) {
        perror("\n[ERROR] save_maze/fwrite");
        return -1;
    }

    fclose(fp);
    return 0;
}

int load_maze(maze_t *maze, unsigned int maze_id) {
    char file_path[64];
    FILE* fp;

    sprintf(file_path, "%smaze%d.txt", MAZES_DIR, maze_id);

    if (!(fp = fopen(file_path, "r"))) {
        perror("\n[ERROR] load_maze/fopen");
        return -1;
    }

    if (fread(maze, sizeof(maze_t), 1, fp) == 0) {
        perror("\n[ERROR] load_maze/fread");
        return -1;
    }

    fclose(fp);
    return 0;
}

int initialize_player(player_t *player, request_t *request) {
    strcpy(player->username, request->username);
    strcpy(player->password, request->data.authentication.password);

    player->single.character.level = 0;

    return 0;
}

int save_player(player_t *player, request_t *request, response_t *response) {
    char file_path[64];
    struct stat st;
    FILE* fp;

    sprintf(file_path, "%s%s.txt", USERS_DIR, request->username);

    if (stat(file_path, &st) == 0) {
        response->code = USERNAME_EXISTS;
        return -1;
    }

    if (!(fp = fopen(file_path, "w"))) {
        perror("\n[ERROR] save_player/fopen");
        response->code = SERVER_ERROR;
        return -1;
    }

    if (fwrite(player, sizeof(player_t), 1, fp) == 0) {
        perror("\n[ERROR] save_player/fwrite");
        response->code = SERVER_ERROR;
        return -1;
    }

    response->code = REGISTRATION_SUCCESSFUL;

    fclose(fp);
    return 0;
}

int load_player(player_t *player, request_t *request, response_t *response) {
    char file_path[64];
    struct stat st;
    FILE* fp;

    sprintf(file_path, "%s%s.txt", USERS_DIR, request->username);

    if (stat(file_path, &st) == -1) {
        response->code = UNKNOWN_USERNAME;
        return -1;
    }

    if (!(fp = fopen(file_path, "r"))) {
        perror("\n[ERROR] load_player/fopen");
        response->code = SERVER_ERROR;
        return -1;
    }

    if (fread(player, sizeof(player_t), 1, fp) == 0) {
        perror("\n[ERROR] load_player/fread");
        response->code = SERVER_ERROR;
        return -1;
    }

    fclose(fp);
    return 0;
}

int update_player(player_t *player, request_t *request, response_t *response) {
    char file_path[64];
    FILE* fp;

    sprintf(file_path, "%s%s.txt", USERS_DIR, request->username);

    if (!(fp = fopen(file_path, "w"))) {
        perror("\n[ERROR] update_player/fopen");
        response->code = SERVER_ERROR;
        return -1;
    }

    if (fwrite(player, sizeof(player_t), 1, fp) == 0) {
        perror("\n[ERROR] update_player/fwrite");
        response->code = SERVER_ERROR;
        return -1;
    }

    fclose(fp);
    return 0;
}

int maze_battle(abilities_t *attacker_abilities, abilities_t *victim_abilities) {
    int successful_shot_probability = rand() % 100 + 1;
    int remaining_damage_points = attacker_abilities->attack;
    int old_armor_points;

    // Attacker's shot didn't hit the victim
    if (successful_shot_probability > attacker_abilities->accuracy) return -1;

    // Else, attacker's shot hit the victim
    // If victim got any armor, hit it first
    old_armor_points = victim_abilities->armor;
    if (victim_abilities->armor > 0) {
        victim_abilities->armor -= remaining_damage_points;
        remaining_damage_points -= (old_armor_points - victim_abilities->armor);
    }

    // Check if victim's armor got destroyed and if there are remaining damage points
    if (victim_abilities->armor < 0) {
        remaining_damage_points += victim_abilities->armor * -1;
        victim_abilities->armor = 0;
    }

    // If there are remaining damage points, decrease health too
    if (remaining_damage_points > 0) victim_abilities->health -= remaining_damage_points;

    // Check if victim is dead
    if (victim_abilities->health <= 0) return 1;

    return 0;
}

int increase_game_level(player_t *player, unsigned int game_mode) {
    character_t *character;
    int i;

    if (game_mode == SINGLE_PLAYER) {
        character = &player->single.character;
        character->level++;
        character->position.x = character->position.y = 1;
        if (load_maze(&player->single.maze, character->level) == -1) return -1;
    } else {
        multiplayer.level++;
        for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
            multiplayer.characters[i].level = multiplayer.level;
            multiplayer.characters[i].position.x = multiplayer.characters[i].position.y = 1;
        }
        if (load_maze(&multiplayer.maze, multiplayer.level) == -1) return -1;
    }

    return 0;
}

int reset_player_progress(player_t *player, unsigned int game_mode) {
    character_t *character;
    int player_id;
    
    if (game_mode == SINGLE_PLAYER) {
        character = &player->single.character;
    } else {
        if ((player_id = get_player_id_by_username(player->username)) == -1) return -1;
        character = &multiplayer.characters[player_id];
    }

    character->position.x = 1;
    character->position.y = 1;
    character->abilities = character->initial_abilities;

    if (game_mode == SINGLE_PLAYER && load_maze(&player->single.maze, character->level) == -1) return -1;

    return 0;
}

void initialize_multiplayer() {
    int i;

    multiplayer.connected_players = 0;
    multiplayer.level = 1;
    load_maze(&multiplayer.maze, multiplayer.level);

    for (i = 0; i < MULTIPLAYER_PLAYERS; i++) memset(&multiplayer.characters[i], 0, sizeof(character_t));
}

int get_player_id_by_username(const char *username) {
    int i;

    for (i = 0; i < MULTIPLAYER_PLAYERS; i++) {
        if (!strcmp(multiplayer.characters[i].name, username)) return i;
    }

    return -1;
}