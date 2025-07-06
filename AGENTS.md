- Keep headers focused, exposing only what will be directly used by callers.
- Don't write comments; instead refactor and refine identifiers to make comments unnecessary.
- Always use braces with if,for,do,while,etc.
- Keep pointer `*`s snug with the variable name on the right,
  or with the type to the left if there is no variable name:

    - void *component_data(struct component * comp, int entity);
    + void* component_data(struct component *comp, int entity);

- Don't disable -Wpadded.  Instead rearrange struct fields so there is no
  padding, or if that's not possible, insert anonymous padding bitfields:

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
