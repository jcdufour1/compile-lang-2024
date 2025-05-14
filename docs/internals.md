# node types
## Uast
## Tast
### TAST_SUM_CALLEE
- in right hand side of assignment (after expr to left of () is checked)
- note: function to type check expr to right of assignment will emit TAST_SUM_LIT if inner type is void, and TAST_SUM_CALLEE otherwise
- `try_set_function_call_types` will later convert `TAST_SUM_CALLEE` to `TAST_SUM_LIT`
### TAST_SUM_LIT
in right hand side of assignment
eg. .num is a Tast_sum_callee
'''c
let token Token = .num(76)
'''
### TAST_SUM_CASE
in switch cases
eg. .num in the case below is a Tast_sum_case
'''c
let token Token = .num(76)
switch token {
    case .num(num): break
}
'''
## Llvm
