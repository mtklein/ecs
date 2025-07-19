import sqlite3
import sys
import termios
import tty

NONE = 0
NEXT_ID = 1

PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED = range(5)

COLOR = {
    PARTY: "\033[32m",
    FRIENDLY: "\033[34m",
    NEUTRAL: "\033[33m",
    HOSTILE: "\033[31m",
    MADDENED: "\033[35m",
}

def rng(seed):
    seed = (seed * 1103515245 + 12345) & 0xFFFFFFFF
    return seed

def d20(ctx):
    ctx[0] = rng(ctx[0])
    return 1 + ctx[0] % 20

def getch():
    fd = sys.stdin.fileno()
    prev = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    try:
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, prev)
    return ch

def init_db(conn):
    conn.execute("CREATE TABLE pos(id INTEGER PRIMARY KEY, x INTEGER, y INTEGER)")
    conn.execute("CREATE TABLE stats(id INTEGER PRIMARY KEY, hp INTEGER, ac INTEGER, atk INTEGER, dmg INTEGER)")
    conn.execute("CREATE TABLE glyph(id INTEGER PRIMARY KEY, ch TEXT)")
    conn.execute("CREATE TABLE disp(id INTEGER PRIMARY KEY, disp INTEGER)")
    conn.execute("CREATE TABLE is_controlled(id INTEGER PRIMARY KEY)")

def attach(conn, table, id, values):
    cols = ",".join(values.keys())
    params = ",".join("?" for _ in values)
    if cols:
        conn.execute(
            f"INSERT OR REPLACE INTO {table}(id,{cols}) VALUES(?,{params})",
            [id, *values.values()],
        )
    else:
        conn.execute(
            f"INSERT OR REPLACE INTO {table}(id) VALUES(?)",
            (id,),
        )


def detach(conn, table, id):
    conn.execute(f"DELETE FROM {table} WHERE id=?", (id,))

def lookup(conn, table, id, fields="*"):
    cur = conn.execute(f"SELECT {fields} FROM {table} WHERE id=?", (id,))
    return cur.fetchone()

def draw(conn, w, h):
    fb = [NONE] * (w * h)
    for id_, x, y in conn.execute("SELECT id,x,y FROM pos"):
        fb[y * w + x] = id_
    for y in range(h):
        row = []
        for x in range(w):
            id_ = fb[y * w + x]
            if id_:
                g = lookup(conn, "glyph", id_, "ch")
                d = lookup(conn, "disp", id_, "disp")
                color = COLOR.get(d[0]) if d else "\033[0m"
                ch = g[0] if g else '.'
            else:
                color = "\033[0m"
                ch = '.'
            row.append(f"{color}{ch}")
        print("".join(row))

def entity_at(conn, x, y):
    row = conn.execute("SELECT id FROM pos WHERE x=? AND y=?", (x, y)).fetchone()
    return row[0] if row else NONE

def alive(conn):
    row = conn.execute(
        "SELECT 1 FROM disp JOIN stats USING(id) WHERE disp=? AND hp>0 LIMIT 1",
        (PARTY,),
    ).fetchone()
    return bool(row)

def kill(conn, id_):
    detach(conn, "stats", id_)
    detach(conn, "disp", id_)
    detach(conn, "is_controlled", id_)
    attach(conn, "glyph", id_, {"ch": "x"})


def combat(conn, attacker, defender, rng_ctx):
    as_ = lookup(conn, "stats", attacker, "hp,ac,atk,dmg")
    ds = lookup(conn, "stats", defender, "hp,ac,atk,dmg")
    if as_ and ds:
        roll = d20(rng_ctx)
        if roll > 1:
            if roll == 20 or roll + as_[2] >= ds[1]:
                hp = ds[0] - as_[3]
                if hp <= 0:
                    kill(conn, defender)
                else:
                    conn.execute("UPDATE stats SET hp=? WHERE id=?", (hp, defender))


def move(conn, dx, dy, w, h, rng_ctx):
    for (id_,) in conn.execute("SELECT id FROM is_controlled"):
        x, y = lookup(conn, "pos", id_, "x,y")
        x_new = x + dx
        y_new = y + dy
        if x_new < 0 or y_new < 0 or x_new >= w or y_new >= h:
            continue
        found = entity_at(conn, x_new, y_new)
        if found:
            combat(conn, id_, found, rng_ctx)
        else:
            conn.execute("UPDATE pos SET x=?, y=? WHERE id=?", (x_new, y_new, id_))


def main(argv):
    seed = int(argv[1]) if len(argv) > 1 else 0
    rng_ctx = [seed]
    conn = sqlite3.connect(":memory:")
    init_db(conn)

    global NEXT_ID
    player = NEXT_ID; NEXT_ID += 1
    attach(conn, "pos", player, {"x": 1, "y": 1})
    attach(conn, "stats", player, {"hp": 10, "ac": 10, "atk": 2, "dmg": 4})
    attach(conn, "glyph", player, {"ch": "@"})
    attach(conn, "disp", player, {"disp": PARTY})
    attach(conn, "is_controlled", player, {})

    imp = NEXT_ID; NEXT_ID += 1
    attach(conn, "pos", imp, {"x": 3, "y": 1})
    attach(conn, "stats", imp, {"hp": 4, "ac": 12, "atk": 3, "dmg": 2})
    attach(conn, "glyph", imp, {"ch": "i"})
    attach(conn, "disp", imp, {"disp": HOSTILE})

    w, h = 10, 5
    while alive(conn):
        draw(conn, w, h)
        ch = getch()
        if ch == 'q':
            return
        elif ch == 'h':
            move(conn, -1, 0, w, h, rng_ctx)
        elif ch == 'j':
            move(conn, 0, 1, w, h, rng_ctx)
        elif ch == 'k':
            move(conn, 0, -1, w, h, rng_ctx)
        elif ch == 'l':
            move(conn, 1, 0, w, h, rng_ctx)
        print(f"\033[{h}A", end="")

if __name__ == "__main__":
    main(sys.argv)
