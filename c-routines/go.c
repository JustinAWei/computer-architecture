#include "go.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

struct Routine;
typedef struct Routine Routine;
extern void magic(Routine *future);

#define STACK_ENTRIES (8192/sizeof(uint64_t))

typedef struct Queue {
    struct Routine *head;
    struct Routine *tail;
} Queue;

struct Channel {
    bool poison;
    Queue *sending;
    Queue *receiving;
};

struct Routine {
    uint64_t saved_rsp;
    struct Routine *next;
    uint64_t x[STACK_ENTRIES];
    Channel *ch;
    Value val;
    jmp_buf env;
    bool env_set;
};

void addQ(Queue *q, Routine *r) {
    r->next = 0;
    if (q->tail != 0) {
        q->tail->next = r;
    }
    q->tail = r;

    if (q->head == 0) {
        q->head = r;
    }
}

Routine *removeQ(Queue *q) {
    Routine *r = q->head;
    if (r != 0) {
        q->head = r->next;
        if (q->tail == r) {
            q->tail = 0;
        }
    }
    return r;
}

Routine *current_ = 0;
Queue ready = {0,0};
Queue zombies = {0,0};

Routine **current() {
    if (current_ == 0) {
        // empty routine
        current_ = (Routine *) calloc(1, sizeof(Routine));
        current_->ch = channel();
    }
    return &current_;
}

/*OSX is stuck in the past and prepends _ in front of external symbols */
Routine **_current() {
    return current();
}

Func global_go_func;
Routine *global_prev_routine;
void wrapper() {
    // grab the go func we set in go
    Func func = global_go_func;

    // go back to routine during go
    magic(global_prev_routine);

    // save initial registers
    if (!(*current())->env_set) {
        (*current())->env_set = true;
        setjmp((*current())->env);
    }

    // execute function
    func();

    // function ends, poison and block
    poison(me());
    receive(me());
}

void next_ready_routine() {
    // no more ready, exit
    if (ready.head == 0) {
        // free all zombie channels and routines
        while (zombies.head != 0) {
            Routine *r = removeQ(&zombies);
            free(r->ch);
            free(r);
        }
        exit(0);
    }
    magic(removeQ(&ready));
}

Channel *go(Func func) {
    // save the func and current
    global_go_func = func;
    global_prev_routine = *current();

    Routine *r = (Routine *) calloc(1, sizeof(Routine));
    r->x[STACK_ENTRIES - 1] = (uint64_t) wrapper;
    r->saved_rsp = (uint64_t)&(r->x[STACK_ENTRIES - 7]);
    r->ch = channel();

    addQ(&ready, r);
    magic(r);
    return r->ch;
}

Channel *me() {
    return (*current())->ch;
}

void again() {
    // jump to initial state
    if ((*current())->env_set)
        longjmp((*current())->env, 1);
}

Channel *channel() {
    Channel *ch = (Channel *) calloc(1, sizeof(Channel));
    ch->sending = (Queue *) calloc(1, sizeof(Queue));
    ch->receiving = (Queue *) calloc(1, sizeof(Queue));
    return ch;
}

bool isPoisoned(Channel *ch) {
    return ch->poison;
}

void poison(Channel *ch) {
    ch->poison = true;
    // zombify everything!
    while (ch->receiving->head != 0)
        addQ(&zombies, removeQ(ch->receiving));

    while (ch->sending->head != 0)
        addQ(&zombies, removeQ(ch->sending));
}

Value receive(Channel *ch) {
    if (isPoisoned(ch)) {
	    addQ(&zombies, *current());
		next_ready_routine();
	}
	if (ch->sending->head != 0) {
        Routine *r = removeQ(ch->sending);
        addQ(&ready, r);
        return r->val;
    } else {
        // just queue it up
        addQ(ch->receiving, *current());
        next_ready_routine();
        return (*current())->val;
    }
}

void send(Channel *ch, Value v) {
    if (isPoisoned(ch)) {
	    addQ(&zombies, *current());
		next_ready_routine();
	}
    // store val we're sending
    (*current())->val = v;
    if (ch->receiving->head != 0) {
        Routine *r = removeQ(ch->receiving);
        addQ(&ready, *current());
        // send over the val to receiver
        r->val = v;
        // switch to receiver
        magic(r);
    } else {
        // just queue it up
        addQ(ch->sending, *current());
        next_ready_routine();
    }
}

struct Stream {
    Channel *ch;
};

static void streamLogic(void) {
    StreamFunc fs = (StreamFunc) receive(me()).asPointer;
    Value v = receive(me());
    fs(v);
    poison(me());
}

Stream *stream(StreamFunc func, Value v) {
    Channel *ch = go(streamLogic);
    send(ch, asPointer(func));
    send(ch, v);
    Stream *out = (Stream *) calloc(sizeof(Stream), 1);
    out->ch = ch;
    return out;
}

Value next(Stream *s) {
    return receive(s->ch);
}

bool endOfStream(Stream *s) {
    return isPoisoned(s->ch);
}

void yield(Value v) {
    send(me(), v);
}
