CC = gcc
CFLAGS = -Wall -g
TARGETS = molecule_supplier atom_supplier molecule_requester

all: $(TARGETS)

molecule_supplier: molecule_supplier.c
	$(CC) $(CFLAGS) -o $@ $<

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) -o $@ $<

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)