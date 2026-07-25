# miniz

Potato statically links miniz 3.1.2, vendored in-tree as `miniz.c` / `miniz.h`.
These are the upstream *amalgamation* (the single-file form upstream publishes
for embedding), generated from commit
`77d0dce8627735138c51770d1799a1ef48f2117d` and otherwise unmodified.

Vendored rather than fetched so the build needs no network and stays
reproducible, per the "explicitly vendored, licensed components" rule in
`docs/PROJECT.md`.

Upstream: https://github.com/richgel999/miniz

## Regenerating

```sh
git clone https://github.com/richgel999/miniz.git
cd miniz && git checkout 77d0dce8627735138c51770d1799a1ef48f2117d
cmake -H. -B_build -DAMALGAMATE_SOURCES=ON -G"Unix Makefiles"
cp _build/amalgamation/miniz.{c,h} <potato>/third_party/miniz/
```

Expected SHA-256 of the files produced by that commit:

```
e2c1aeb66eef9191d8c3feb164db2def2335a61d039bf04ed849f6b042433b30  miniz.c
b53b62ed122e559b8f679e3cb787a0b0035fe87a58f909da0e44931678f4e85f  miniz.h
```
