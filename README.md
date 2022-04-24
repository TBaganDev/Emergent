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

## Grammar
```
program -> model program_tail
program -> neighbourhood program_tail
program_tail -> model program_tail
program_tail -> neighbourhood program_tail
program_tail ->

model -> MODEL ID COLON ID LBRACE states RBRACE

neighbourhood -> NEIGHBOURHOOD ID COLON INT_LIT LBRACE neighbours RBRACE

neighbours ->  neighbour neighbours_tail
neighbours_tail -> COMMA neighbour neighbours_tail
neighbours_tail -> 

neighbour -> ID coord 
neighbour -> coord

states -> state states_tail
states_tail -> state states_tail
states_tail ->

state -> DEFAULT STATE ID
state -> STATE ID LBRACE pred RBRACE
state -> STATE ID LBRACE RBRACE

pred -> ex_disj pred_tail
pred_tail -> OR ex_disj pred_tail
pred_tail ->

ex_disj -> conj ex_disj_tail
ex_disj_tail -> XOR conj ex_disj_tail
ex_disj_tail ->

conj -> equiv conj_tail
conj_tail -> AND equiv conj_tail
conj_tail -> 

equiv -> rel equiv_tail
equiv_tail -> EQ rel equiv_tail
equiv_tail -> NE rel equiv_tail
equiv_tail ->

rel -> trans rel_tail
rel_tail -> LE trans rel_tail
rel_tail -> LT trans rel_tail
rel_tail -> GE trans rel_tail
rel_tail -> GT trans rel_tail
rel_tail ->

trans -> scale trans_tail
trans_tail -> ADD scale trans_tail
trans_tail -> SUB scale trans_tail
trans_tail ->

scale -> element scale_tail
scale_tail -> MULT element scale_tail
scale_tail -> DIV element scale_tail
scale_tail -> MOD element scale_tail
scale_tail ->

element -> SUB element
element -> NOT element
element -> LPAREN pred RPAREN
element -> INT_LIT
element -> DEC_LIT
element -> PIPE set PIPE
element -> THIS
element -> ID
element -> coord

set -> SET ID IN ANY COLON pred
set -> SET ID IN coords COLON pred

coord -> LSQUAR vector RSQUAR

vector -> INT_LIT vector_tail
vector_tail -> COMMA INT_LIT vector_tail
vector_tail -> 

coords -> coord coords_tail
coords_tail -> COMMA coord coords_tail
coords_tail -> 
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