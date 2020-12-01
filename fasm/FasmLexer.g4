lexer grammar FasmLexer;

S : [ \t] -> skip ;
NEWLINE : [\n\r] ;
fragment NON_ESCAPE_CHARACTERS : ~[\\"] ;
fragment ESCAPE_SEQUENCES : [\\] [\\"] ;
fragment IDENTIFIER : [a-zA-Z] [0-9a-zA-Z_]* ;
FEATURE : IDENTIFIER ('.' IDENTIFIER)* ;
HEXADECIMAL_VALUE : '\'h' [ \t]* [0-9a-fA-F_]+ ;
BINARY_VALUE      : '\'b' [ \t]* [01_]+ ;
DECIMAL_VALUE     : '\'d' [ \t]* [0-9_]+ ;
OCTAL_VALUE       : '\'o' [ \t]* [0-7_]+ ;
INT : [0-9]+ ;
COMMENT_CAP : '#' (~[\n\r])* ;

EQUAL : '=' ;
OPEN_BRACKET : '[' ;
COLON : ':' ;
CLOSE_BRACKET : ']' ;

BEGIN_ANNOTATION : '{' -> pushMode(ANNOTATION_MODE) ;

mode ANNOTATION_MODE;
ANNOTATION_S : [ \t] -> skip ;
ANNOTATION_NAME : [.a-zA-Z] [0-9a-zA-Z_]* ;
ANNOTATION_VALUE : '"' (NON_ESCAPE_CHARACTERS | ESCAPE_SEQUENCES)* '"' ;
END_ANNOTATION : '}' -> popMode ;
ANNOTATION_EQUAL : '=' ;
ANNOTATION_COMMA : ',' ;

