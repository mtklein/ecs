General guidelines:
1) Preserve existing style; avoid purely stylistic changes like whitespace tweaks.
2) Mimic existing style and established conventions for new code.
3) Don't proactively defend against bugs; trust the sanitizers will do their job.
4) Write comments only as a last resort for clarity, preferring to refactor and
   refine identifiers to make the code itself clear and comments unnecessary.
5) Keep headers focused, exposing only what will be directly used by callers.
6) If you would use an external dependency instead recreate the functionality
   you'd need from that dependency from scratch in a new build target.
7) If asked to do more than you can handle in one PR instead act as an architect,
   sketching the code interfaces and not-yet-working tests.  Leave follow-up TODOs.

When creating PRs for this project,
  - always run the exact command `ninja` to build and test all targets; and
  - add any changes to coverage.report to the PR, making sure coverage stays at 100%.

Specific C style notes:
- Allow lines up to 100 columns.
- Always use braces with if, for, do, while, etc.
- Prefer normal nested if/else logic over early return,
  but carefully factor the logic so it is clear and not too deeply nested.
- Count bytes with `size_t` and everything else with `int`.
- Use const liberally, especially with descriptively named local variables...
- ... but the constness of a pointer is much less important than the constness
  of its pointee, and multiple consts in a single declaration can be difficult
  to parse, so prefer not to mark pointers themselves const except when it is
  unusually important to signal a pointer will not be changing.
- Use singular names for arrays and array-style pointers and the plural for their
  paired integer count.  E.g. `struct table const *table[]` and `int tables`.
- Keep pointer `*`s snug with the variable name on the right,
  or with the type to the left if there is no variable name:

    - void *component_data(struct component * comp, int entity);
    + void* component_data(struct component *comp, int entity);

- Don't disable -Wpadded.  Rearrange struct fields so there is no padding,
  or insert anonymous padding bitfields if that's not possible:

    struct foo {
        int   len;
        int   :32;
        void *ptr;
    };

- When working with generic code and void*, use implicit casting to provide the
  true types and descriptive names:

     void sum(void const *data, void *ctx) {
    -    *(int *)ctx += *(int *)data;
    +    int const *val = data;
    +    int       *sum = ctx;
    +    *sum += *val;
     }

- Generally declare related function parameters on a single line together if there is room
  and pass related function arguments in close grouping with no whitespace between.
  Always list a system's components last, declared one per line, passed as related arguments:

    void work(int unit[], int units,
              void (*cb)(int, void *ctx), void *ctx) { ... }
    ...
    work(unit,units, my_cb,ctx);

    void some_system(int id,
                     struct component *a,
                     struct component *b,
                     struct component *c);
    ...
    some_system(id, a,b,c);

    void some_other_system(int x, int y,
                           struct component *a,
                           struct component *b,
                           struct component *c,
                           struct component *d);
    ...
    some_other_system(x,y, a,b,c,d);
