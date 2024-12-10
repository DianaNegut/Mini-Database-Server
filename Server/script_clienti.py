import socket
import threading
import time
import random

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8120

def client_simulation(client_id, commands):
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((SERVER_HOST, SERVER_PORT))
        print(f"Client {client_id} conectat la server.")

        # Alege o comandă aleatorie din listă
        command = random.choice(commands)
        print(f"Client {client_id} a ales comanda: {command}")

        # Buclă pentru trimiterea continuă a comenzii
        while True:
            client_socket.sendall(command.encode('utf-8'))
            print(f"Client {client_id} a trimis: {command}")

            response = client_socket.recv(1024).decode('utf-8')
            if not response:
                break  # Serverul a închis conexiunea

            print(f"Client {client_id} a primit răspuns: \n{response}")

            # Condiție pentru a decide să trimită comanda "QUIT" și a se deconecta
            if random.random() < 0.1:  # Probabilitate de 10% să trimită "QUIT"
                quit_message = "QUIT"
                client_socket.sendall(quit_message.encode('utf-8'))
                print(f"Client {client_id} a trimis: {quit_message}")
                break

            time.sleep(random.uniform(0.5, 2))  # Pauză între comenzile trimise

        client_socket.close()
        print(f"Client {client_id} a închis conexiunea.")
    except Exception as e:
        print(f"Eroare la clientul {client_id}: {e}")

commands = [
    "SELECT * FROM angajati",
    "QUIT"
]

# Crește numărul de clienți pentru a depăși capacitatea thread pool-ului și a cozii
NUM_CLIENTS = 20

threads = []
for i in range(NUM_CLIENTS):
    t = threading.Thread(target=client_simulation, args=(i + 1, commands))
    threads.append(t)
    t.start()
    time.sleep(0.5)  # Mică întârziere pentru a trimite clienții progresiv

# Așteaptă finalizarea tuturor thread-urilor
for t in threads:
    t.join()

print("Toți clienții au terminat.")
