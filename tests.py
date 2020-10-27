import ctypes
import os
import unittest

os.environ["TERM"] = "linux"

from pwn import *

ADDRESS = '127.0.0.1'
PORT = 8081

PKG_CREATE_ROOM = 0
PKG_JOIN_ROOM = 1
PKG_MOVE = 2
PKG_CASTLING = 3
PKG_EN_PASSANT = 4
PKG_PROMOTION = 5
PKG_CHAT_MSG = 100
PKG_STATUS = 200

PKG_CLIENT_CODE = 0
PKG_CLIENT_ROOM_FULL = 1
PKG_CLIENT_MOVE = 2
PKG_CLIENT_CASTLING = 3
PKG_CLIENT_EN_PASSANT = 4
PKG_CLIENT_ROOM_NOT_FOUND = 5
PKG_CLIENT_JOINED = 6
PKG_CLIENT_PROMOTION = 7
PKG_CLIENT_CHAT_MSG = 100
PKG_CLIENT_STATUS = 200

if os.getenv("CI") is not None:
    p = subprocess.Popen(['./chess-server'], stdout=subprocess.DEVNULL)

context.log_level = logging.FATAL
unittest.TestLoader.sortTestMethodsUsing = None


def p_byte(bytes_data):
    return ctypes.c_ubyte.from_buffer_copy(bytes_data).value.real


def uint16_to_pint(bytes_data):
    return socket.ntohs(ctypes.c_uint16.from_buffer_copy(bytes_data).value.real)


def connect():
    r = remote(ADDRESS, PORT)
    r.send_raw(b'CHESS_PROTO/1.0')
    return r


def send_code(r, code):
    r.send(bytes([code]))


def create_pair():
    host = connect()
    send_code(host, PKG_CREATE_ROOM)
    host.recvn(1)
    invite_code = host.recvn(7).decode("ascii")
    client = connect()
    send_code(client, PKG_JOIN_ROOM)
    client.send(invite_code)
    client.recvn(1)
    host.recvn(1)
    return host, client


class TestServer(unittest.TestCase):

    def test_status(self):
        r = connect()
        send_code(r, PKG_STATUS)
        self.assertEqual(p_byte(r.recvn(1)), PKG_CLIENT_STATUS)
        count = uint16_to_pint(r.recvn(2))
        self.assertTrue(0 <= count <= 100)
        r.close()

    def test_room_flow(self):
        host = connect()
        send_code(host, PKG_CREATE_ROOM)
        self.assertEqual(p_byte(host.recvn(1)), PKG_CLIENT_CODE)
        invite_code = host.recvn(7).decode("ascii")
        self.assertRegex(invite_code, re.compile('[a-zA-Z0-9]{3}-[a-zA-Z0-9]{3}'))
        client = connect()
        send_code(client, PKG_JOIN_ROOM)
        client.send(invite_code)
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_JOINED)
        self.assertEqual(p_byte(host.recvn(1)), PKG_CLIENT_ROOM_FULL)
        client.close()
        host.close()

    def test_move(self):
        host, client = create_pair()
        send_code(host, PKG_MOVE)
        host.send(bytes([6, 6, 6, 5]))
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_MOVE)
        self.assertEqual(str(client.recvn(4)), str(bytes([6, 6, 6, 5])))
        client.close()
        host.close()

    def test_chat(self):
        host, client = create_pair()
        send_code(host, PKG_CHAT_MSG)
        message = b'Hello, world!\0'
        host.send_raw(message)
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_CHAT_MSG)
        self.assertEqual(client.recvuntil('\0'), message)
        client.close()
        host.close()


if __name__ == '__main__':
    unittest.main()
