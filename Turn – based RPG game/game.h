#ifndef GAME_H
#define GAME_H

#define ANSI_CLEAR                          "\e[1;1H\e[2J"
#define ANSI_RESET                          "\e[0m"
#define ANSI_YELLOW_BG                      "\e[43m"
#define ANSI_RED_BG                         "\e[41m"
#define ANSI_BLUE_BG                        "\e[44m"

#define SINGLE_PLAYER                       1
#define MULTI_PLAYER                        2

#define MOVE_UP_KEY                         'W'
#define MOVE_DOWN_KEY                       'S'
#define MOVE_LEFT_KEY                       'A'
#define MOVE_RIGHT_KEY                      'D'
#define ESC_KEY                             27

#define ABILITY_COUNT                       4
#define ABILITY_MIN_POINTS                  1
#define ABILITY_MAX_POINTS                  100
#define HEALTH                              0
#define ARMOR                               1
#define ATTACK                              2
#define ACCURACY                            3

#define MAZE_COUNT                          10
#define MAZE_WIDTH                          16
#define MAZE_HEIGHT                         16
#define MAZE_TREASURE_COUNT                 10
#define MAZE_MONSTER_COUNT                  10
#define MAZE_EMPTY_POINT                    ' '
#define MAZE_HORIZONTAL_WALL                '_'
#define MAZE_VERTICAL_WALL                  '|'
#define MAZE_TREASURE                       'T'
#define MAZE_MONSTER                        'm'
#define MAZE_MONSTER_BOSS                   'M'

#define NEW_CHARACTER_POINTS                50
#define MULTIPLAYER_PLAYERS                 3

#define RESPONSE_SERVER_ERROR               "A server error occured"
#define RESPONSE_UNKNOWN_USERNAME           "Unknown username"
#define RESPONSE_INCORRECT_PASSWORD         "Incorrect password"
#define RESPONSE_LOGIN_SUCCESSFULL          "Login successful"
#define RESPONSE_USERNAME_EXISTS            "Username already exists"
#define RESPONSE_REGISTRATION_SUCCESSFUL    "Registration successful"
#define RESPONSE_NO_CREATED_CHARACTER       "You haven't created a character for this mode"
#define RESPONSE_CHARACTER_CREATED          "Your character has been created!"
#define RESPONSE_SP_GAME_COMPLETE           "Good job! It seems that you found a way out of the mazes!"
#define RESPONSE_MP_WAITING_FOR_PLAYERS     "Please wait for other players to connect..."
#define RESPONSE_MP_GAME_IN_PROGRESS        "A multi-player game is already in progress. Please try again later."

const char *ANSI_COLORS[] = {
    ANSI_YELLOW_BG,
    ANSI_RED_BG,
    ANSI_BLUE_BG
};

typedef enum player_color_t {
    YELLOW,
    RED,
    BLUE
} player_color_t;

typedef struct position_t {
    int x;
    int y;
} position_t;

typedef struct abilities_t {
    int health;
    int armor;
    int attack;
    int accuracy;
} abilities_t;

typedef struct treasure_t {
    position_t position;
    int ability;
    int points;
} treasure_t;

typedef struct monster_t {
    position_t position;
    abilities_t abilities;
} monster_t;

typedef struct maze_t {
    char map[MAZE_HEIGHT + 1][MAZE_WIDTH * 2 + 1];
    treasure_t treasures[MAZE_TREASURE_COUNT];
    monster_t monsters[MAZE_MONSTER_COUNT];
    monster_t monster_boss;
} maze_t;

typedef struct character_t {
    char name[33];
    int wins;
    int defeats;
    int level;
    position_t position;
    abilities_t initial_abilities;
    abilities_t abilities;
} character_t;

typedef struct player_t {
    char username[33];
    char password[33];
    struct {
        character_t character;
        maze_t maze;
    } single;
} player_t;

typedef struct multiplayer_t {
    int connected_players;
    int level;
    maze_t maze;
    character_t characters[MULTIPLAYER_PLAYERS];
} multiplayer_t;

typedef struct request_t {
    enum {
        LOGIN,
        REGISTER,
        GET_SINGLE_PLAYER_PROGRESS,
        GET_MULTI_PLAYER_PROGRESS,
        CREATE_SINGLE_PLAYER_CHARACTER,
        CREATE_MULTI_PLAYER_CHARACTER,
        UPDATE_SINGLE_PLAYER_PROGRESS,
        UPDATE_MULTI_PLAYER_PROGRESS,
        QUIT
    } code;
    char username[33];

    union {
        struct {
            char password[33];
        } authentication;
        struct {
            char movement_key;
        } play;
        struct {
            abilities_t abilities;
        } new_character;
    } data;
} request_t;

typedef struct response_t {
    enum {
        SERVER_ERROR,
        UNKNOWN_USERNAME,
        INCORRECT_PASSWORD,
        LOGIN_SUCCESSFUL,
        USERNAME_EXISTS,
        REGISTRATION_SUCCESSFUL,
        NO_CREATED_CHARACTER,
        CHARACTER_CREATED,
        SP_READY_TO_PLAY,
        SP_GAME_COMPLETE,
        MP_READY_TO_PLAY,
        MP_GAME_COMPLETE,
        MP_WAITING_FOR_PLAYERS,
        MP_GAME_IN_PROGRESS
    } code;

    maze_t maze;
    character_t characters[MULTIPLAYER_PLAYERS];
} response_t;

#endif