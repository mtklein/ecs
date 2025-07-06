- Don't write comments; instead refactor and refine identifiers to make comments unnecessary.
- Always use braces with if,for,do,while,etc.
- Keep the pointer `*` with the variable name to the right, or with the type to the left if there is no name, e.g. `void* component_data(struct component *comp, int entity);`
- Instead of disabling -Wpadded, rearrange fields so there is no padding, and if that's not
  possible, insert anonymous padding bitfields, e.g.

    struct foo {
        int   len;
        int   :32;
        void *ptr;
    };

- Rename struct leaf to struct branch, and move its definition into ecs.c.  The header just needs a pointer declaration, not a definition.
- Instead of explicit casts from void*, give names to the cast values and use implicit casting, e.g.

 static void sum_fn(int entity, void *data, void *ctx) {
     (void)entity;
-    *(int *)ctx += *(int *)data;
+    int       *sum = ctx;
+    int const *val = data;
+    *sum += *val;
 }
