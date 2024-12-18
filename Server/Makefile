EXEC = server

SRCS = server.c SQLParser.c gestionareTabele.c BST.c threadPool.c cache.c

OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -g -pthread
INCLUDES = -I./

$(EXEC): $(OBJS)
	@gcc $(CFLAGS) -o $(EXEC) $(OBJS)
	@echo "Compilare finalizată cu succes!"

%.o: %.c
	@gcc $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@rm -f $(OBJS) $(EXEC)
	@echo "Fișierele obiect și executabilul au fost șterse!"

run: $(EXEC)
	@./$(EXEC)
