tar = chat_cli chat_srv
CLASF = -Wall -g

all: $(tar)
	
%.o: %.cpp
	g++ $(CLASF) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o $(tar)
