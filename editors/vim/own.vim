
syntax clear

" documentation for vim pattern matching:
"   https://vimhelp.org/pattern.txt.html

" TODO: highlight todo in .own files?

syn keyword ownKeyword
    \ def
    \ else
    \ extern
    \ fn
    \ for
    \ if
    \ import
    \ in
    \ let
    \ switch
    \ type

syn keyword ownStmt
    \ break
    \ continue
    \ defer
    \ return
    \ using
    \ yield

syn keyword ownLabel
    \ case
    \ default

syn keyword ownStructLike
    \ enum
    \ raw_union
    \ struct

syn match ownFunctionCall display "\zs\w*\ze([^<]"

syn match ownIntType display "\v<[u|i][1-9]\d*>"
syn match ownFloatType display "\v<f[1-9]\d*>"
syn match ownTypeType display "\v<Type>"

syn match ownIntTypeError display "\v<[i|u]\d\d*\zs\D\D*\ze\d*>"
syn match ownIntTypeError display "v<0\d\d*>"
syn match ownFloatTypeError display "\v<f\d\d*\zs\h*\ze>"

syn keyword ownPreludeType
    \ usize

syn match ownIntLiteral display "\v<0>"
syn match ownIntLiteral display "\v<[1-9](_*\d)*>"
syn match ownIntLiteral display "\v<0x\x(_*\x)*>"
syn match ownIntLiteral display "\v<0o\o(_*\o)*>"
syn match ownIntLiteral display "\v<0b[0-1](_*[0-1])*>"

syn match ownFloatLiteral display "\v<[1-9](_*\d)*>\.\v<\d(_*\d)*>"

syn match ownDelimiter display "[{},()\[\]]"
syn match ownDelimiter display "(<"
syn match ownDelimiter display ">)"

syn match ownOperator display "\v[\+\-\*\/\&\=\%\!\^\|]"
syn match ownOperator display "[\<]"  " TODO: this also catches < in (<, which could be wrong 
                                      " (however, this may be fine, because operators and delimeters are often the same color anyway)
syn match ownOperator display "\v\zs[\>]\ze[^\)]"
syn match ownOperator display "\v\zs[\.]\ze\h"

syn keyword ownOperator
    \ countof
    \ sizeof
    \ unsafe_cast

syn match ownComment display "\v\/\/.*$"
syn region ownComment display start="\v\/\*" end="\v\*\/"

syn match ownStringLiteral display "\v\"(\\.|[^"])*\""

syn match ownCharLiteral display "\v'.'"
syn match ownCharLiteral display "\v'\\.'"
syn match ownCharLiteral display "\v'\\x\x\x'"
syn match ownCharLiteral display "\v'\\o\o\o\o'"

"
" hi def links
"

hi def link ownIntLiteral     Number
hi def link ownFloatLiteral   Number
hi def link ownKeyword        Keyword
hi def link ownStmt           Statement
hi def link ownLabel          Label
hi def link ownIntType        Type
hi def link ownFloatType      Type
hi def link ownTypeType       Type
hi def link ownPreludeType    Type
hi def link ownFunctionCall   Function
hi def link ownStringLiteral  String
hi def link ownCharLiteral    String
hi def link ownDelimiter      Delimiter
hi def link ownOperator       Operator
hi def link ownStructLike     Structure
hi def link ownComment        Comment

hi def link ownIntTypeError Error
hi def link ownFloatTypeError Error

let b:current_syntax = "own"

