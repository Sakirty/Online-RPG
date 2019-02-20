all: rpg_client rpg_server

rpg_client: rpg_client.c common.h
	gcc -o $@ $^ -lm -std=c99

rpg_server: rpg_server.c common.h
	gcc -o $@ $^ -lpthread -lm -std=c99

.PHONY: clean

clean:
	rm -f rpg_server rpg_client
