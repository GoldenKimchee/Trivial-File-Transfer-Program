create: ./SERVER/Server.cpp ./CLIENT/Client.cpp
				g++ ./SERVER/Server.cpp -std=c++17
				g++ ./CLIENT/Client.cpp -o c.out

clean:
				rm *.out