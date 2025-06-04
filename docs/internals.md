# node types
## Uast
### UAST_LABEL_DEF
- the name of the label is placed in the parent scope of the block after the label
- the label has a separate field `block_scope` that contains the actual scope that the label breaks out of
- `block_scope` is equal to SCOPE_NOT if label does not actually point to anything
## Tast
### TAST_ENUM_CALLEE
- in right hand side of assignment (after expr to left of () is checked)
- note: function to type check expr to right of assignment will emit TAST_ENUM_LIT if inner type is void, and TAST_ENUM_CALLEE otherwise
- `try_set_function_call_types` will later convert `TAST_ENUM_CALLEE` to `TAST_ENUM_LIT`
### TAST_ENUM_LIT
in right hand side of assignment
eg. .num is a Tast_enum_callee
'''c
let token Token = .num(76)
'''
### TAST_ENUM_CASE
in switch cases
eg. .num in the case below is a Tast_enum_case
'''c
let token Token = .num(76)
switch token {
    case .num(num): break
}
'''
## Llvm
