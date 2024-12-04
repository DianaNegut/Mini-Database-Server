import socket
import threading
import time


SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8112

def client_simulation(client_id, message):
    try:

        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((SERVER_HOST, SERVER_PORT))
        print(f"Client {client_id} conectat la server.")


        client_socket.sendall(message.encode('utf-8'))
        print(f"Client {client_id} a trimis: {message}")


        response = client_socket.recv(1024).decode('utf-8')
        print(f"Client {client_id} a primit răspuns: {response}")


        client_socket.close()
    except Exception as e:
        print(f"Eroare la clientul {client_id}: {e}")

commands = [
    "SELECT * FROM angajati",
    "INSERT INTO angajati VALUES (5,'Alice', 30, 7);",
    "SELECT * FROM TEST_29.11.2024",
    "UPDATE angajati SET varsta = 36 WHERE name = 'Ion Popescu'"
]


NUM_CLIENTS = 10


threads = []
for i in range(NUM_CLIENTS):
    message = commands[i % len(commands)]  
    t = threading.Thread(target=client_simulation, args=(i + 1, message))
    threads.append(t)
    t.start()


for t in threads:
    t.join()

print("Toți clienții au terminat.")
