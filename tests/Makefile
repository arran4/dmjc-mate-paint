CC = gcc
CFLAGS = -I../src $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0)

test_gp_point_array: test_gp_point_array.o ../src/gp_point_array.o
	$(CC) -o $@ $^ $(LIBS)

test_gp_point_array.o: test_gp_point_array.c
	$(CC) $(CFLAGS) -c $< -o $@

../src/gp_point_array.o: ../src/gp_point_array.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o test_gp_point_array ../src/*.o

run: test_gp_point_array
	./test_gp_point_array
