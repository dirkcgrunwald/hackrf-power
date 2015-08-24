CFLAGS = -DGPS_POWER -O -g -Wall

OBJ = hackrf_power.o hackrf_buffer.o convenience.o


hackrf_power: $(OBJ)
	$(CC) -o hackrf_power $(OBJ) -lhackrf -lpthread -lm -lgps

clean:
	-rm -f hackrf_power *.o
