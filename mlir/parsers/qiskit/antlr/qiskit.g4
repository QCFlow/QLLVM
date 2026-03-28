grammar qiskit;

/**** Parser grammar ****/

program
    : quantumCircuitDeclaration 
      (quantumStatement | measurementStatement)*
    ;

quantumCircuitDeclaration
    : circuitName '=' 'QuantumCircuit' '(' quantumCount (',' classicalCount)? ')' SEMICOLON
    ;

circuitName
    : Identifier
    ;

/*** Quantum Statements ***/

quantumStatement
    : quantumGateCall SEMICOLON
    | quantumBarrier SEMICOLON
    ;

measurementStatement
    : quantumMeasurement SEMICOLON
    ;

/*** Quantum Gates and Instructions ***/

quantumGateCall
    : circuitName '.' quantumGateName '(' gateArgumentList? ')' SEMICOLON
    ;

quantumGateName
    : 'h'
    | 'x'
    | 'y'
    | 'z'
    | 'cx'
    | 'CX'
    | 'U'
    | 'reset'
    | Identifier
    ;

quantumMeasurement
    : circuitName '.' 'measure' '(' measurementArgumentList ')' SEMICOLON
    ;

quantumBarrier
    : circuitName '.' 'barrier' '(' gateArgumentList? ')' SEMICOLON
    ;

/*** Argument Lists ***/

gateArgumentList
    : ( expression COMMA )* expression
    ;

measurementArgumentList
    : measurementArgument (',' measurementArgument)*
    ;

expression
    : expressionTerminator
    ;

expressionTerminator
    : Constant
    | Integer
    | RealNumber
    | Identifier
    | StringLiteral
    | listExpression
    | MINUS expressionTerminator
    ;

listExpression
    : LBRACKET ( Integer COMMA )* Integer RBRACKET
    ;

measurementArgument
    : Integer                    # IntegerMeasurementArgument
    | qubitReference            # QubitMeasurementArgument
    ;

/*** Quantum Operations ***/

quantumOperation
    : quantumGateName
    | 'measure'
    | 'barrier'
    | Identifier
    ;

/*** Expressions ***/

expression_1
    : term
    | expression_1 '+' term
    | expression_1 '-' term
    ;

term
    : factor
    | term '*' factor
    | term '/' factor
    ;

factor
    : Integer
    | '(' expression_1 ')'
    | qubitReference
    | classicalReference
    ;

/*** Identifiers and References ***/

quantumCount
    : Integer
    ;

classicalCount
    : Integer
    ;

qubitReference
    : 'q' '[' Integer ']'
    ;

classicalReference
    : 'c' '[' Integer ']'
    ;

/*** Lexer grammar ***/

SEMICOLON : ';' ;
LPAREN : '(' ;
RPAREN : ')' ;
LBRACKET : '[' ;
RBRACKET : ']' ;
COLON : ':' ;
COMMA : ',' ;
DOT : '.' ;
EQUALS : '=' ;
PLUS : '+' ;
MINUS : '-' ;
STAR : '*' ;
SLASH : '/' ;

fragment Digit : [0-9] ;
Integer : Digit+ ;

Constant : ( 'pi' | 'π' | 'tau' | '𝜏' | 'euler' | 'ℇ' );
fragment SciNotation : [eE] ;
fragment PlusMinus : [-+] ;
fragment Float : Digit+ DOT Digit* | DOT Digit*;

RealNumber : Float (SciNotation PlusMinus? Integer )? ;

fragment ValidUnicode : [\p{Lu}\p{Ll}\p{Lt}\p{Lm}\p{Lo}\p{Nl}] ; // valid unicode chars
fragment Letter : [A-Za-z] ;
fragment FirstIdCharacter : '_' | '$' | ValidUnicode | Letter ;
fragment GeneralIdCharacter : FirstIdCharacter | Integer;

StringLiteral
    : '"' ~["\r\t\n]+? '"'
    | '\'' ~['\r\t\n]+? '\''
    ;


Identifier : FirstIdCharacter GeneralIdCharacter* ;

Whitespace : [ \t\r\n]+ -> skip ;

LineComment : '#' ~[\r\n]* -> skip ;