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


def send_byte(r, code):
    r.send(bytes([code]))


def create_pair():
    host = connect()
    send_byte(host, PKG_CREATE_ROOM)
    send_byte(host, 1)
    host.recvn(1)
    invite_code = host.recvn(7).decode("ascii")
    client = connect()
    send_byte(client, PKG_JOIN_ROOM)
    client.send(invite_code)
    client.recvn(1)  # PKG TYPE
    client.recvn(1)  # recv is_white
    host.recvn(1)
    return host, client


class Game(object):
    def __init__(self):
        (self.white, self.black) = create_pair()
        self.white_move = True

    def get_current_dst(self):
        if self.white_move:
            return self.white
        else:
            return self.black

    def make_move(self, move_type, move, recv=True):
        move = bytes(move)
        dst = self.get_current_dst()
        send_byte(dst, move_type)
        dst.send(move)
        self.white_move = not self.white_move
        if recv:
            dst = self.get_current_dst()
            dst.recvn(1)
            dst.recvn(len(move))

    def close(self):
        self.black.close()
        self.white.close()


class TestServer(unittest.TestCase):

    def anti_cheat_test(self, game, move_type, pos, should_recv):
        game.make_move(move_type, pos, False)
        player = game.get_current_dst()
        if len(player.recvn(1, timeout=0.1)) == 0:
            if should_recv:
                self.fail("Anticheat not works, valid packet banned")
        else:
            if not should_recv:
                self.fail("Anticheat not works, cheat packet passed")
            else:
                self.assertEqual(bytes(pos), player.recvn(4, timeout=2))
        game.close()

    def test_status(self):
        r = connect()
        send_byte(r, PKG_STATUS)
        self.assertEqual(p_byte(r.recvn(1)), PKG_CLIENT_STATUS)
        count = uint16_to_pint(r.recvn(2))
        self.assertTrue(0 <= count <= 100)
        r.close()

    def test_room_flow(self):
        host = connect()
        send_byte(host, PKG_CREATE_ROOM)
        send_byte(host, 1)
        self.assertEqual(p_byte(host.recvn(1)), PKG_CLIENT_CODE)
        invite_code = host.recvn(7).decode("ascii")
        self.assertRegex(invite_code, re.compile('[a-zA-Z0-9]{3}-[a-zA-Z0-9]{3}'))
        client = connect()
        send_byte(client, PKG_JOIN_ROOM)
        client.send(invite_code)
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_JOINED)
        self.assertEqual(p_byte(client.recvn(1)), 0)
        self.assertEqual(p_byte(host.recvn(1)), PKG_CLIENT_ROOM_FULL)
        client.close()
        host.close()

    def test_move(self):
        host, client = create_pair()
        send_byte(host, PKG_MOVE)
        host.send(bytes([6, 6, 6, 5]))
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_MOVE)
        self.assertEqual(str(client.recvn(4)), str(bytes([6, 6, 6, 5])))
        client.close()
        host.close()

    def test_chat(self):
        host, client = create_pair()
        send_byte(host, PKG_CHAT_MSG)
        message = b'Hello, world!\0'
        host.send_raw(message)
        self.assertEqual(p_byte(client.recvn(1)), PKG_CLIENT_CHAT_MSG)
        self.assertEqual(client.recvuntil('\0'), message)
        client.close()
        host.close()

    #
    #       0   1   2   3   4   5   6   7
    #   0   r   n   b   q   k   b   n   r
    #   1   p   p   p   p   p   p   p   p
    #   2
    #   3
    #   4
    #   5
    #   6   P   P   P   P   P   P   P   P
    #   7   R   N   B   Q   K   B   N   R

    def test_king_jump(self):
        self.anti_cheat_test(Game(), PKG_MOVE, [4, 7, 3, 0], should_recv=False)

    def test_pawn_jump(self):
        self.anti_cheat_test(Game(), PKG_MOVE, [6, 6, 0, 5], should_recv=False)

    def test_pawn_jump2(self):
        self.anti_cheat_test(Game(), PKG_MOVE, [6, 6, 5, 5], should_recv=False)

    def test_complicated_pos(self):
        game = Game()
        game.make_move(PKG_MOVE, [3, 6, 3, 4])
        game.make_move(PKG_MOVE, [4, 1, 4, 3])
        self.anti_cheat_test(game, PKG_MOVE, [3, 4, 4, 3], should_recv=True)

    # def test_ddos(self):
    #    for i in range(0, 10000000):
    #        host, client = create_pair()
    #        host.close()
    #        client.close()


if __name__ == '__main__':
    unittest.main()
