import socket
import struct


def get_connection(ipaddr):
    connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        connection.connect(ipaddr)
    except Exception as e:
        return None
    return connection


def send_command(connection, cmd):
    sendbuf = struct.pack('i')
    try:
        connection.send(sendbuf)
    except socket.error as msg:
        return False
    return True
