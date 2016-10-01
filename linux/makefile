CFLAGS = -std=c++14 -c

pckmain: pckfile.o pckitem.o pckmain.o
	g++ -o pckmain pckfile.o pckitem.o pckmain.o -lz -pthread -lstdc++fs

pckfile.o: pckfile.cpp
	g++ $(CFLAGS) pckfile.cpp

pckitem.o: pckitem.cpp
	g++ $(CFLAGS) pckitem.cpp

pckmain.o: pckmain.cpp
	g++ $(CFLAGS) pckmain.cpp

clean:
	rm *.o

