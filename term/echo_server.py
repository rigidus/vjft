#!/usr/bin/python3
import socket
import select
import sys

def echo_server(host='127.0.0.1', port=8888):
    # Инициализация сокета сервера
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Listening on {host}:{port}...")

    # Список сокетов для отслеживания
    sockets_list = [server_socket, sys.stdin]

    try:
        while True:
            # Ждем, пока не будет доступен ввод или подключение
            readable, _, _ = select.select(sockets_list, [], [])
            for notified_socket in readable:
                if notified_socket == server_socket:
                    # Обработка нового подключения
                    client_socket, addr = server_socket.accept()
                    print(f"Connected by {addr}")
                    sockets_list.append(client_socket)
                elif notified_socket == sys.stdin:
                    # Считывание ввода с консоли и отправка его всем клиентам
                    message = sys.stdin.readline()
                    for sock in sockets_list:
                        if sock != server_socket and sock != sys.stdin:
                            sock.send(message.encode())
                else:
                    # Обработка входящего сообщения от клиента
                    data = notified_socket.recv(1024)
                    if not data:
                        print(f"Connection with {notified_socket.getpeername()} closed")
                        sockets_list.remove(notified_socket)
                        notified_socket.close()
                    else:
                        print(f"Received: {data.decode().strip()} from {notified_socket.getpeername()}")
                        notified_socket.sendall(data)
    except KeyboardInterrupt:
        print("Server is shutting down...")
    finally:
        server_socket.close()
        for sock in sockets_list:
            if sock != server_socket and sock != sys.stdin:
                sock.close()

if __name__ == "__main__":
    echo_server()
