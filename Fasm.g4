grammar Fasm;

// Lexing Rules
S : [ \t] -> skip ;
NEWLINE : [\n\r] -> skip ;
fragment NON_ESCAPE_CHARACTERS : [^\\"] ;
fragment ESCAPE_SEQUENCES : [\\] [\\"] ;
fragment IDENTIFIER : [a-zA-Z] [0-9a-zA-Z_]* ;
FEATURE : IDENTIFIER ('.' IDENTIFIER)* ;
HEXIDECIMAL_VALUE : '\'h' [ \t]* [0-9a-fA-F_]+ ;
BINARY_VALUE      : '\'b' [ \t]* [01_]+ ;
DECIMAL_VALUE     : '\'d' [ \t]* [0-9_]+ ;
OCTAL_VALUE       : '\'o' [ \t]* [0-7_]+ ;
INT : [0-9]+ ;
ANNOTATION_NAME : [.a-zA-Z] [0-9a-zA-Z_]* ;
COMMENT_CAP : '#' [^\n\r] -> skip ;
ANNOTATION_VALUE : (NON_ESCAPE_CHARACTERS | ESCAPE_SEQUENCES)+ ;

// Parsing Rules
fasmFile : fasmLine (NEWLINE fasmLine)* EOF ;
fasmLine : (S* setFasmFeature)?
           (S* annotations)?
           (S* COMMENT_CAP)?
           S* ;

setFasmFeature : S* FEATURE featureAddress? S* ('=' S* value S*)? ;
featureAddress : '[' INT (':' INT)? ']' ;

value : INT? S* verilogDigits # VerilogValue
      | INT                   # PlainDecimal
      ;

verilogDigits : HEXIDECIMAL_VALUE # HexValue
              | BINARY_VALUE      # BinaryValue
              | DECIMAL_VALUE     # DecimalValue
              | OCTAL_VALUE       # OctalValue
              ;

annotations : '{' S* annotation (',' S* annotation)* S* '}' ;
annotation : ANNOTATION_NAME S* '=' S* '"' ANNOTATION_VALUE '"' ;
