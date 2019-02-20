#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include <math.h>
#include <fcntl.h> 
#include <unistd.h> 

pthread_mutex_t mlock;  

int num_of_players = 0;
Player players[10];
Player npcs[10];

Armor armors[5];
Weapon weapons[5]; 


void init_NPC(Player *npcs);
void init_game(Armor *, Weapon *);
//void show_game(int , int);
void fight(int, int, int);
int get_rand(int num_of_side);
void * processPlayer(void *);
int find_player(Player pa);
int send_response(int command_no, int arg, int sfd);
int send_player(Player *, int sfd);
int send_fight_round(Fight_round *,int sfd);
void update_player(Player *temp);



int main(int argc, char const *argv[])
{
	
	if (argc != 2)
	{
		printf("Expected input: %s <PORT NUMBER>\n", argv[0]);
		return 1;
	}

	init_game(armors, weapons);
	init_NPC(npcs);
	srand((unsigned int)time(NULL));
     
	pthread_mutex_init(&(mlock), NULL); 
	int sfd, connfd;
	struct sockaddr_in addr;
	//if<1 return an error
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sfd < 0){
		printf("an error\n");
		return -1;
	}
	int port = atoi(argv[1]);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; //automatically find IP

	//bind(sfd, (struct sockaddr *)&addr, sizeof(addr));

	if((bind(sfd, (struct sockaddr *)&addr, sizeof(addr))) < 0){
       printf("Bind failed\n");
        return -1;
	}



	if((listen(sfd,10)) < 0){
        printf("Listen failed\n");
        return -1;
	}

	while(1){

		connfd = accept(sfd, NULL,NULL);
		if((connfd)< 0){
	        printf("Accept failed\n");
	        return -1;
		}

		pthread_t tid;
		int mypara = connfd;
        int error = pthread_create(&tid, NULL, processPlayer, &mypara); 
        if(error != 0){
            perror("pthread_create failed");
            return 1;
        }
        
	}
	close(sfd);
	return 0;
}




void init_NPC(Player *npcs){
    char *npc_names[10] = {"Sauron", "Orc 1", "Orc 2", "Orc 3", "Orc 4", "Orc 5", "Orc 6", "Orc 7", "Orc 8", "Gollum"};
    int npcs_hps[10] = {115, 20, 20, 20, 20, 20, 20, 20, 20, 10 };
    //int npcs_armors[10] = {4,0,1,0,0,2,3,0,3,0};
    //int npcs_weapons[10] ={4,3,2,1,4,2,3,3,3,0};
    int npcs_armors[10] = {4,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,0};
    int npcs_weapons[10] ={4,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,get_rand(5)-1,0};
    int npcs_levels[10] = {20,1,1,1,1,1,1,1,1,1};
    int npcs_xps[10] = {1048576000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000};
    for (int i = 0; i < 10; ++i)
    {
        strcpy(npcs[i].name, npc_names[i]);
        npcs[i].hp = npcs_hps[i];
        //if(1<=i<=8){
          //  npcs[i].armor_index = get_rand(5) - 1;//npcs_armors[i];
            //npcs[i].weapon_index = get_rand(5) - 1;//npcs_weapons[i];
        //}else{
            npcs[i].armor_index = npcs_armors[i];
            npcs[i].weapon_index = npcs_weapons[i];
        //}
        npcs[i].level = npcs_levels[i];
        npcs[i].xp = npcs_xps[i];
    }

}

void init_game(Armor *armors, Weapon *weapons){
    char *anames[5] = {"cloth", "studded leather", "ring mail", "chain mail", "plate"};
    int acs[5] ={10,12,14,16,18};
    char *wnames[5] = {"dagger", "short sword", "long sword", "great sword", "great axe"};
    int wdices[5] = {1,1,1,2,1};
    int wdamage[5] = {4, 6,8,6,12};
    for (int i = 0; i < 5; ++i)
    {
        strcpy(armors[i].name, anames[i]);
        armors[i].AC = acs[i];
        strcpy(weapons[i].name, wnames[i]);
        weapons[i].dice = wdices[i];
        weapons[i].damage_per_dice = wdamage[i]; 
    }
}




void fight(int player_index, int npc_index, int fd){
	Player * pa = &players[player_index];
    Player * npc = &npcs[npc_index];
    int ri = npc->armor_index;
    int wi = npc->weapon_index;

    char input[50];

    Fight_round * fight_rounds = NULL;
    int round = 0;
    
    while(pa->hp > 0 && npc->hp > 0 ){
        // player fight first 
        round ++;
        fight_rounds = (Fight_round *)realloc(fight_rounds, sizeof(Fight_round)*round);
        int rolla = get_rand(20);
        fight_rounds[round - 1].roll_result_a = rolla; 
        if (rolla >=  armors[ri].AC)
        {
            int damage = 0;
            for (int i = 0; i < weapons[pa->weapon_index].dice; ++i)
            {
                damage +=  get_rand(weapons[pa->weapon_index].damage_per_dice);//rand()%( weapons[pa->weapon_index].damage_per_dice) + 1;
            }
            //printf("%s hits %s for %d damage (attack roll %d)\n", pa->name,  npc->name, damage,rolla);
            npc->hp -= damage;
            fight_rounds[round - 1].attack_result_a = HIT;
            fight_rounds[round - 1].damage_a = damage;   
            
            
        }
        else
        {
            //printf("%s misses %s (attack roll %d)\n", pa->name, npc->name, rolla);
            fight_rounds[round - 1].attack_result_a = MISS;
            fight_rounds[round - 1].damage_a = 0;   
           
        }
        
        rolla = get_rand(20);
        fight_rounds[round - 1].roll_result_b = rolla; 
        if (rolla >=  armors[pa->armor_index].AC)
        {
            int damage = 0;
            for (int i = 0; i < weapons[wi].dice; ++i)
            {
                damage +=  get_rand(weapons[wi].damage_per_dice);//rand()%( weapons[wi].damage_per_dice) + 1;
            }
            //printf("%s hits %s for %d damage (attack roll %d)\n", npc->name,  pa->name,damage, rolla);
            pa->hp -= damage;  

            fight_rounds[round - 1].attack_result_b = HIT;
            fight_rounds[round - 1].damage_b = damage;   
            
        }
        else{
            //printf("%s misses %s (attack roll %d)\n", npc->name, pa->name, rolla);
            fight_rounds[round - 1].attack_result_b = MISS;
            fight_rounds[round - 1].damage_b = 0; 
        }

        fight_rounds[round - 1].pa = *pa;
    	fight_rounds[round - 1].pb = *npc;
       
    }

    send_response(FIGHTROUND, round, fd);
    for (int i = 0; i < round; ++i)
    {
    	send_fight_round(&fight_rounds[i], fd);
    }
    
    free(fight_rounds);

	if (npc->hp <= 0 && pa->hp > 0) 
    {
        
        Request re;
    	recv(fd, &re, sizeof(Request),0);
    	if (re.command_no == PICKUP)
    	{
    		if (re.arg/2) // exchange weapon
    		{
    			int temp = npc->weapon_index;
                pa -> weapon_index = temp;                
    		}

    		if (re.arg%2){
    			int temp = npc->armor_index;
                pa -> armor_index = temp;        
    		}
    	}

        pa->xp += 2000*(npc->level);
        while (pa->xp >= 1000*pow(2, pa->level + 1))
        {
            pa->level ++;
            //printf("%s levels up to level %d!\n", pa->name, pa->level );      
        }
        pa->hp = 20 + (pa->level-1) * 5;

        send_player(pa, fd);

        ////printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);

        // update npc
        if(npc_index >= 1 && npc_index <= 8) // it's a orc
        {
            int new_level = get_rand(pa->level);//rand()%(pa->level) +1;
            npc->level = new_level;
            npc->xp = 1000*pow(2, new_level);
            npc->hp =  20 + (new_level-1) * 5;
            npc->weapon_index = get_rand(5) - 1;
            npc->armor_index = get_rand(5) - 1;

        }
        if (npc_index == 9)
        {
            npc->hp = 10;
        }

        if (npc_index == 0)
        {
            npc->hp = 115;
        }

        //printf("Respawning %s ...\n", npc->name);
        send_player(npc, fd);
    }
    else if (npc->hp > 0 && pa->hp <= 0)
    {
        //printf("Respawning %s ...\n", pa->name);
        pa->hp = 20 + (pa->level-1) * 5;
        pa->xp = 1000 * pow(2, pa->level); //lost all the xp 
        send_player(pa, fd);
        //printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);

    }
    else{
        // both die
        //printf("Respawning %s ...\n", pa->name);
        pa->hp = 20 + (pa->level-1) * 5;
        pa->xp = 1000 * pow(2, pa->level); //lost all the xp
        //printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);
        send_player(pa, fd);
        // update npc

        if(npc_index >= 1 && npc_index <= 8) // it's a orc
        {
            int new_level = get_rand(pa->level);//rand()%(pa->level) +1;
            npc->level = new_level;
            npc->xp = 1000*pow(2, new_level);
            npc->hp =  20 + (new_level-1) * 5;
            npc->weapon_index = get_rand(5) -1;
            npc->armor_index = get_rand(5) -1;

        }
        if (npc_index == 9)
        {
            npc->hp = 10;
        }

        if (npc_index == 0)
        {
            npc->hp = 115;
        }

        //printf("Respawning %s ...\n", npc->name);
        send_player(npc, fd);
        //printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n", 
    }

   

}

int get_rand(int num_of_side){
    return rand()%(num_of_side) + 1;
}

void * processPlayer( void * para){
	int fd = *((int *) para);
	char buffer[1024];
	Request re;
	//printf("connection from file handle %d\n", fd);
	int player_index = -1;

	while(1){
		recv(fd, &re, sizeof(Request), 0);
		if (re.command_no == SEARCH)
		{
			Player temp;
			recv(fd, &temp, sizeof(Player), 0);
			int index = find_player(temp);
			if (index >= 0)
			{
				send_response(FOUND, 0, fd);
				send_player(&players[index], fd);
				player_index = index;

			}
			else{
				send_response(NOTFOUND, 0, fd);
			}

		}
		if (re.command_no == QUIT)
		{
			break;
		}
		if (re.command_no == REGIST) // regitst the player
		{
			Player temp;
			recv(fd, &temp, sizeof(Player), 0);
			
			update_player(&temp);
			pthread_mutex_lock(&mlock); 
			player_index = num_of_players;
			players[num_of_players++] = temp;
			pthread_mutex_unlock(&mlock); 
			send_player(&temp, fd);
			
		}

		if (re.command_no == LOOK)
		{
			send_response(PLAYERS, 10, fd);
			for (int i = 0; i < 10; ++i)
			{
				send_player(&npcs[i], fd);
			}
			send_response(PLAYERS, num_of_players, fd);
			for (int i = 0; i < num_of_players; ++i)
			{
				send_player(&players[i], fd);
			}
		}

		if (re.command_no == STATS)
		{
			send_player(&players[player_index], fd);
		}

		if (re.command_no == FIGHT)
		{
			pthread_mutex_lock(&mlock); 
			fight(player_index, re.arg, fd);
			pthread_mutex_unlock(&mlock); 
		}

		

	}

	close(fd);
}

int find_player(Player p){
	if (num_of_players <= 0){
		return -1;
	}

	for (int i = 0; i < num_of_players; ++i)
	{
		if (strcmp(p.name, players[i].name) == 0)
		{
			return i;
		}
	}
	return -1;
}

int send_response(int command_no, int arg, int sfd){
	Response re = {command_no, arg};
	int ret = send(sfd, &re, sizeof(Response), 0);
	if (ret < 0){
		perror("Send Response");
		exit(ret);
	} 
	return ret;
}

int send_player(Player *p, int sfd){
	int ret = send(sfd, p, sizeof(Player), 0);
	if (ret < 0){
		perror("Send player");
		exit(ret);
	} 
	return ret;
}

void update_player(Player * temp){
	int new_level = temp->level;
    temp->xp = 1000*pow(2, new_level);
    temp->hp =  20 + (new_level-1) * 5;

}

int send_fight_round(Fight_round *fr,int sfd){
	int ret = send(sfd, fr, sizeof(Fight_round), 0);
	if (ret < 0){
		perror("Send fight_round");
		exit(ret);
	} 
	return ret;
}