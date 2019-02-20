#ifndef COMMON_H
#define COMMON_H

typedef struct weapon
{
	char name[50];
	int dice;
	int damage_per_dice;
} Weapon;

typedef struct armor
{
	char name[50];
	int AC;
} Armor;


typedef struct A{
    char name[50];
    int armor_index;
    int weapon_index;
    int hp;
    int level;
    int  xp;
} Player;


// used for request of client to server
enum Command
{
	QUIT = 0,
	SEARCH,
	REGIST,
	LOOK,
	FIGHT,
	PICKUP,
	STATS
};


// for response of client's request 
enum 
{
	FOUND = 0,
	NOTFOUND,
	PLAYERS,
	FIGHTROUND
	
};

// for fight round, HIT OR MISS
enum 
{
	HIT = 0,
	MISS
};

// the package from client to server
typedef struct client_request
{
	int command_no;
	int arg;
	
} Request;

typedef struct server_response
{
	int response_no;
	int arg; 
	
} Response;

typedef struct fight_round
{
	Player pa;
	int attack_result_a;
	int damage_a;
	int roll_result_a;	
	Player pb;
	int attack_result_b;
	int damage_b;
	int roll_result_b;
} Fight_round;

#endif