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
## Analysis of the Grammar
There was a common pattern in the Grammar, where:
```
X      -> A X_TAIL
X_TAIL -> t A X_TAIL
       ...
X_TAIL -> epsilon
```
With `X`, `X_TAIL` and `A` non-terminals, and terminal(s) `t`.
This pattern is used frequently, particularly to explore binary/list syntax.
So, this pattern is used within the code to allow for general use for the parsing of any binary infix operation, or list of items with/without a delimeter.