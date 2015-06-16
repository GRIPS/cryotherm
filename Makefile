CC = gcc
INCLUDE = -I/usr/include/libusb-1.0
LIB = -lm -lusb-1.0
EXE = cryotherm
OBJ = cryotherm.o

$(EXE): $(OBJ) libsub/libsub.o
	$(CC) -o $@ $^ $(LIB)

libsub/libsub.o:
	make -C libsub

*.o: *.c
	$(CC) -c $^

clean:
	make -C libsub clean
	rm -f $(EXE) $(OBJ)
