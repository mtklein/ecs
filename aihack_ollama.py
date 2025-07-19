#!/usr/bin/env python3
import json
import sys
import termios
import tty
from urllib import request

OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL = "llama2"

def talk(prompt):
    data = json.dumps({"model": MODEL, "prompt": prompt}).encode()
    req = request.Request(OLLAMA_URL, data=data,
                          headers={"Content-Type": "application/json"})
    with request.urlopen(req) as resp:
        parts = []
        for line in resp:
            line = line.strip()
            if not line:
                continue
            obj = json.loads(line)
            parts.append(obj.get("response", ""))
        return "".join(parts)

def getch():
    fd = sys.stdin.fileno()
    prev = termios.tcgetattr(fd)
    tty.setraw(fd)
    try:
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, prev)
    return ch

def main():
    intro = (
        "You are a game engine running a simple ascii roguelike. "
        "The map is 10 columns by 5 rows. "
        "The player '@' starts at x=1 y=1 with 10 hp, ac 10, attack 2, damage 4. "
        "There is one imp 'i' at x=3 y=1 with 4 hp, ac 12, attack 3, damage 2. "
        "Use exactly five lines of ten characters to describe the board."
    )
    board = talk(intro)
    sys.stdout.write(board + "\n")
    while True:
        c = getch()
        if c == 'q':
            break
        if c not in 'hjkl':
            continue
        moves = {'h': 'left', 'j': 'down', 'k': 'up', 'l': 'right'}
        board = talk(f"move {moves[c]}")
        sys.stdout.write("\033[5A")
        sys.stdout.write(board + "\n")
        sys.stdout.flush()

if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        sys.exit(str(exc))
