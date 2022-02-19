TARGETS=player ringmaster

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.c
	g++ -g -o $@ $<

ringmaster: ringmaster.c
	g++ -g -o $@ $<

