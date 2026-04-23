grammar qcis;

/**** Parser grammar ****/

program
    :  (quantumStatement | measurementStatement)*
    ;


circuitName
    : Identifier
    ;

/*** Quantum Statements ***/

quantumStatement
    : quantumGateCall
    | quantumBarrier SEMICOLON
    ;

measurementStatement
    : quantumMeasurement SEMICOLON
    ;

/*** Quantum Gates and Instructions ***/

quantumGateCall
    : quantumGateName  QubitId ( gateArgumentList | QubitId )? 
    ;

quantumBarrier
    : 'B' QubitId+
    ;

quantumGateName
    : Identifier
    ;

quantumMeasurement
    : 'M'  measurementArgumentList
    ;


/*** Argument Lists ***/

gateArgumentList
    : ( expression COMMA )* expression
    ;

measurementArgumentList
    : QubitId
    ;

gateArgument
    : expression      # expressionArgument
    | Integer                    # IntegerGateArgument
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


/*** Identifiers and References ***/

quantumCount
    : Integer
    ;

classicalCount
    : Integer
    ;

qubitReference
    : 'q' '[' Integer ']'
    | QubitId
    ;

gateParameter
    : RealNumber
    | Integer
    | Constant
    | MINUS gateParameter
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

QubitId : 'Q' Digit+ ;

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