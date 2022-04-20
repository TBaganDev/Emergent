# Emergent
Simple Cellular Automata Programming Language, made using LLVM in C++.

## Structure
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

## Operator Precedence

```
---------------|
negation       | -X not X
---------------|
multiplication | X*X 
division       | X/X 
modulus        | X%X
---------------|
addition       | X+X 
subtraction    | X-X
---------------|
comparison     | X<X X<=X X>X X>=X
---------------|
equality       | X==X X!=X
---------------|
logical and    | X and X
---------------|
logical xor    | X xor X
---------------|
logical or     | X or X
---------------|
```

