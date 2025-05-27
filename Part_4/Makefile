CC = gcc
CFLAGS = -Wall -Wextra -g
TARGETS = drinks_bar atom_supplier molecule_requester

all: $(TARGETS)

drinks_bar: drinks_bar.c
	$(CC) $(CFLAGS) -o $@ $<

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) -o $@ $<

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)