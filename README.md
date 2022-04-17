# Emergent
Simple Cellular Automata Programming Language, made using LLVM in C++.

```
neighbourhood <neighbourhood_id> : <dimensions> {
    ..., <id> [<number>, ...], ...
}

model <model_id> : <neighbourhood_id> {
    state <state_id> {
        ... some boolean predicate ...
    }
}
```