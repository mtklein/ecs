// TODO: store components' sparseset struct just before the data,
//       so that it can be tracked as just a (typed) pointer
//
//       struct component {
//           ...
//       } *component;

static int events;

struct system {
    void* (*fn)(int event, void *state);
    void                        *state;
};

static void drain_events(struct system *system, int systems) {
    for (int event = 0; event < events; event++) {
        for (int i = 0; i < systems; i++) {
            system[i].state = system[i].fn(event, system[i].state);
        }
    }
    events = 0;
}

struct key_event {
    int key;
};
struct key_event *key_event;
sparseset         key_event_meta;

struct config_event {
    _Bool *running;
    void (*d20)(void *rng);
    void  *rng;
};
struct config_event *config_event;
sparseset            config_event_meta;

struct resize_event {
    int w,h;
};
struct resize_event *resize_event;
sparseset            resize_meta;

struct attack_event {
    int attacker, defender;
};

static int d20(void *ctx) {
    unsigned *seed = ctx;
    ...
    return ...;
}

static void* game_state(int event, void *ctx) {
    struct {
        _Bool *running;
    } *state = ctx;
    if (!state) {
        state = calloc(1, sizeof *state);
    }

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            state->running = e->running;
        }
    }

    {
        struct key_event const *e = get(event, key_event);
        if (e && e->key == 'q') {
            *state->running = 0;
        }
    }

    return state;
}


static void* movement(int event, void *ctx) {
    struct {
        int   w,h;
        int (*d20)(void *rng);
        void *rng;
    } *state = ctx;
    if (!state) {
        state = calloc(1, sizeof *state);
    }

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            state->d20 = e->d20;
            state->rng = e->rng;
        }
    }

    {
        struct resize_event const *e = get(event, resize_event);
        if (e) {
            state->w = e->w;
            state->h = e->h;
        }
    }

    {
        struct key_event const *e = get(event, key_event);
        int dx=0, dy=0;
        switch (e ? e->key : 'x') {
            case 'h': dx = -1; break;
            case 'j': dy = -1; break;
            case 'k': dy = +1; break;
            case 'l': dx = +1; break;
        }

        if (move(dx,dy, state->w,state->h, state->d20,state->rng)) {
            int const id = events++;
            set(id, attack_event) = (struct attack_event){...};
        }
    }
}

static void* combat(int event, void *ctx) {
    (void*)ctx;

    {
        struct attack_event const *e = get(event, attack_event);
        if (e) {
            ...
        }
        del(event, attack_event);
    }

    return ctx;
}


int main(void) {
    struct system system[] = {
        {.fn=game_state},
        {.fn=movement},
        {.fn=combat},
    };

    _Bool running = 1;
    unsigned seed = 0;
    int w=10, h=5;

    {
        int const event = events++;
        set(event, config_event) = (struct config_event){&running,d20,&seed};
        set(event, resize_event) = (struct resize_event){w,h};

        drain_events(system, len(system));
        del(event, config_event);
        del(event, resize_event);
    }

    while (running) {
        int const event = events++;
        set(event, key_event).key = getch();

        drain_events(system, len(system));
        del(event, key_event);
    }
}
