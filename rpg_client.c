#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <errno.h>
#include "common.h"

Player pa;
int num_of_players = 0;
Player players[10];
Player npcs[10];

Armor armors[5];
Weapon weapons[5]; 
void init_player();
void init_NPC(Player *npcs);
void init_game(Armor *, Weapon *);

// send request to the server
int send_request(int command_no, int arg, int sfd);
void show_game(int , int);
void show_fight_round(Fight_round);
void handle_fight_result(Fight_round fr, int sfd);


int main(int argc, char const *argv[]){

	if (argc != 2)
	{
		printf("To setup: %s <PORT NUMBER>\n", argv[0]);
		return 1;
	}

	char recv_buffer[1024];
	init_game(armors, weapons);
	init_NPC(npcs);

	int sfd, connfd;
	struct sockaddr_in addr;
	char *servInetAddr = "127.0.0.1";
 	
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
	//addr.sin_addr.s_addr = INADDR_ANY; //automatically find IP
	inet_pton(AF_INET,servInetAddr,&addr.sin_addr);
	if((connect(sfd,(struct sockaddr*)&addr,sizeof(addr))) < 0 )
	{
		printf("connect error %s errno: %d\n",strerror(errno),errno);
		exit(0);
	}

	// init a player

	
	printf("What is your name?\n");
	fgets(pa.name, 20, stdin);
	if (pa.name[strlen(pa.name) -1] == '\n')
	{
		pa.name[strlen(pa.name) -1] = '\0';
	}

	send_request(SEARCH, 0, sfd);
	send(sfd, &pa, sizeof(Player), 0);
	Response response;
	int n = recv(sfd, &response, sizeof(Response), 0);
	if (n < 0)
	{
		perror("Recv error");
		return -1;
	}
	

	if (response.response_no == NOTFOUND) // not found
	{
		init_player();
		send_request(REGIST, 0, sfd);
		send(sfd, &pa, sizeof(Player), 0);
	}
	else{
		printf("\nExisting user profile found. Loading ...\n");
	}
	recv(sfd, &pa, sizeof(Player), 0);
	printf("\nPlayer Setting Complete:\n");
    printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",pa.name, pa.hp, armors[pa.armor_index].name, weapons[pa.weapon_index].name, pa.level, pa.xp);



	char line[100];
    char command[20];
    while(1){
        printf("command >> ");
        fgets(line, 99, stdin);
        if (strcmp(line, "") == 0 || strcmp(line, "\n") == 0)
        {
            continue;
        }

        sscanf(line, "%s", command);
        
        if (strcmp(command, "quit") == 0)
        {
        	send_request(QUIT, 0, sfd);
        	close(sfd);//unless quit, dont close the connection
            break;
        }
        else if (strcmp(command, "look") == 0)
        {
            send_request(LOOK, 0, sfd);
            recv(sfd, &response, sizeof(Response),0);
            for (int i = 0; i < response.arg; ++i)
            {
            	recv(sfd, &npcs[i], sizeof(Player),0);

            }

            recv(sfd, &response, sizeof(Response),0);
            num_of_players = response.arg;
            for (int i = 0; i < response.arg; ++i)
            {
            	recv(sfd, &players[i], sizeof(Player),0);

            }
            show_game(1,1);   
        }
         else if (strcmp(command, "stats") == 0)
        {
            send_request(STATS, 0, sfd);
            recv(sfd, &pa, sizeof(Player), 0);
            printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",pa.name, pa.hp, armors[pa.armor_index].name, weapons[pa.weapon_index].name, pa.level, pa.xp);
        }
        else if (strcmp(command, "fight") == 0)
        {
            int npc_index;
            sscanf(line, "%s %d", command, &npc_index);
            if (npc_index >= 10 || npc_index < 0)
            {
            	printf("Invalid fighting NPC index\n\n");
            	continue;
            }
            send_request(FIGHT, npc_index, sfd);
            recv(sfd, &response, sizeof(Response),0);
            Fight_round fr;
            if (response.response_no == FIGHTROUND)
            {
            	
            	for (int i = 0; i < response.arg; ++i)
            	{
            		recv(sfd, &fr, sizeof(Fight_round),0);
            		show_fight_round(fr);
            	}
            }
            // update result
            handle_fight_result(fr, sfd);
        }
       
       
        else{
            printf("Invalid command \"%s\"\n", command);
        }
        // flush remain inputs
        
    }
	//close(sfd);

	return 0;
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

void init_NPC(Player *npcs){
    char *npc_names[10] = {"Sauron", "Orc 1", "Orc 2", "Orc 3", "Orc 4", "Orc 5", "Orc 6", "Orc 7", "Orc 8", "Gollum"};
    int npcs_hps[10] = {115, 20, 20, 20, 20, 20, 20, 20, 20, 10 };
    int npcs_armors[10] = {4,0,1,0,0,2,3,0,3,0};
    int npcs_weapons[10] ={4,3,2,1,4,2,3,3,3,0};
    int npcs_levels[10] = {20,1,1,1,1,1,1,1,1,1};
    int npcs_xps[10] = {1048576000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000};
    for (int i = 0; i < 10; ++i)
    {
        strcpy(npcs[i].name, npc_names[i]);
        npcs[i].hp = npcs_hps[i];
        npcs[i].armor_index = npcs_armors[i];
        npcs[i].weapon_index = npcs_weapons[i];
        npcs[i].level = npcs_levels[i];
        npcs[i].xp = npcs_xps[i];
    }

}

void init_player(){
     // promt to input name
    
    pa.level = 1;
    pa.xp = 0;
    pa.hp = 0;

    
    printf("\nUser profile not found. Creating new character...\n");
    printf("\nList of available armors:\n"); //0: cloth (AC=10)\n1: studded leather (AC=12)\n2: ring mail (AC=14)\n3: chain mail (AC=16)\n4: plate (AC=18)\n");
    for (int i = 0; i < 5; ++i)
    {
        printf("%d: %s (AC=%d)\n",i, armors[i].name, armors[i].AC);
    }
    int a;
    printf("\nChoose %s's Armor (0~4): ", pa.name);
    scanf("%d",&a);
    while (a < 0 || a > 4)
    {
       
        printf("\nChoose %s's Armor (0~4): ", pa.name);
        scanf("%d",&a);
    }
    pa.armor_index = a;
    
  

    printf("\nList of available weapons:\n");//0: dagger (damage=1d4)\n1: short sword (damage=1d6)\n2: long sword (damage=1d8)\n3: great sword (damage=2d6)\n4: great axe (damage=1d12)\n");
    for (int i = 0; i < 5; ++i)
    {
        printf("%d: %s (damage=%dd%d)\n",i, weapons[i].name, weapons[i].dice, weapons[i].damage_per_dice);
    }
    printf("\nChoose %s's Weapon (0~4): ", pa.name);
    scanf("%d",&a);
    while (a < 0 || a > 4)
    {
        
        printf("\nChoose %s's Weapon (0~4): ", pa.name);
        scanf("%d",&a);
    }
    pa.weapon_index = a;
    while((fgetc(stdin) != '\n'));
    
}

int send_request(int command_no, int arg, int sfd){
	Request re = {command_no, arg};
	int ret = send(sfd, &re, sizeof(Request), 0);
	if (ret < 0){
		perror("Send request");
		exit(ret);
	} 
	return ret;
}

void show_game(int npc_flag, int player_flag){

    if (npc_flag)
    {
        /* code */
    
        printf("All is peaceful in the land of Mordor.\n");
        printf("Sauron and his minions are blissfully going about their business:\n");
        for (int i = 0; i < 10; ++i)
        {
            printf("%d: [%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n", i, 
                npcs[i].name, npcs[i].hp, armors[npcs[i].armor_index].name, 
                weapons[npcs[i].weapon_index].name, npcs[i].level, npcs[i].xp);
        }
    }
    if (player_flag)
    {
      
        printf("Also at the scene are some adventurers looking for trouble:\n");
        for (int i = 0; i < num_of_players; ++i)
        {
            printf("%d: [%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n", i, 
                players[i].name, players[i].hp, armors[players[i].armor_index].name, 
                weapons[players[i].weapon_index].name, players[i].level, players[i].xp);
        }
    }
    

}

void show_fight_round(Fight_round fr){

	Player *pa = &(fr.pa);
	Player *npc = &(fr.pb);

	if (fr.attack_result_a == HIT)		
		printf("%s hits %s for %d damage (attack roll %d)\n", pa->name,  npc->name, fr.damage_a, fr.roll_result_a);
	else
         printf("%s misses %s (attack roll %d)\n", pa->name,  npc->name, fr.roll_result_a);

     if (fr.attack_result_b == HIT)		
		printf("%s hits %s for %d damage (attack roll %d)\n", npc->name,  pa->name, fr.damage_b, fr.roll_result_b);
	else
         printf("%s misses %s (attack roll %d)\n", npc->name,  pa->name, fr.roll_result_b);
}

void handle_fight_result(Fight_round fr, int sfd){

	Player *npc = &fr.pb;
	Player *pa = &fr.pa; 
	int orignal_level = pa->level;
	char input[50];
	int pickup = 0;


	if (npc->hp <= 0 && pa->hp > 0) 
    {
        
        printf("\n%s is killed by %s.\n", npc->name, pa->name);
        printf("Get %s's %s, exchanging %s's current armor %s (y/n)? ", npc->name, armors[npc->armor_index].name, pa->name,  armors[pa->armor_index].name);
        fgets(input, 49, stdin);
        
        while(1){
            
            if (strcmp(input, "y\n") == 0)
            {
              
                pickup += 1;
                break;
                
            }
            if (strcmp(input, "n\n") == 0)
            {

                break;
            }
            printf("Get %s's %s, exchanging %s's current armor %s (y/n)? ", npc->name, armors[npc->armor_index].name, pa->name,  armors[pa->armor_index].name);
            fgets(input, 49, stdin);
        }
        

        printf("Get %s's %s, exchanging %s's current weapon %s (y/n)? ", npc->name, weapons[npc->weapon_index].name, pa->name,  weapons[pa->weapon_index].name);
        
        fgets(input, 49, stdin);
        while(1){
            //getchar();
            //while(fgetc(stdin) != '\n');
            if (strcmp(input, "y\n") == 0)
            {
                
                pickup += 2;
                break;
                
            }
            if (strcmp(input, "n\n") == 0)
            {

                break;
            }
            printf("Get %s's %s, exchanging %s's current weapon %s (y/n)? ", npc->name, weapons[npc->weapon_index].name, pa->name,  weapons[pa->weapon_index].name);
            fgets(input, 49, stdin);
        }
        send_request(PICKUP, pickup, sfd);

        
        recv(sfd, pa, sizeof(Player), 0);
        if(pa->level > orignal_level){
        	printf("\n%s levels up to level %d!\n", pa->name, pa->level );
        }

        printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",
        	pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);

        printf("Respawning %s ...\n", npc->name);
        
        recv(sfd, npc, sizeof(Player), 0);

        printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",
        	npc->name, npc->hp, armors[npc->armor_index].name, weapons[npc->weapon_index].name, npc->level, npc->xp);

    }

    else if (npc->hp > 0 && pa->hp <= 0)
    {
        printf("\n%s is killed by %s.\n",pa->name,  npc->name);
        printf("Respawning %s ...\n", pa->name);
        recv(sfd, pa, sizeof(Player), 0);

        printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",
        	pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);

    }
    else{
        // both die
        printf("\n%s and %s are killed by each other.\n",pa->name,  npc->name);
        printf("Respawning %s ...\n", pa->name);
       
        recv(sfd, pa, sizeof(Player), 0);
        printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",
        	pa->name, pa->hp, armors[pa->armor_index].name, weapons[pa->weapon_index].name, pa->level, pa->xp);
       
        
        printf("Respawning %s ...\n", npc->name);
        recv(sfd, npc, sizeof(Player), 0);
        printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",
        	npc->name, npc->hp, armors[npc->armor_index].name, weapons[npc->weapon_index].name, npc->level, npc->xp);
    }
}