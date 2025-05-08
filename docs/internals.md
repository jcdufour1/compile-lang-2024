# node types
## Uast
## Tast
### TAST_SUM_CALLEE
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
