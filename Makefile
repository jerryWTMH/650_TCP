TARGETS= player ringmaster

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
player: player.c potato.h
	g++ -g -o $@ $<

ringmaster: ringmaster.c potato.h
	g++ -g -o $@ $<

