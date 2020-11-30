grammar fasm;

// Lexing Rules
S: [ \t] -> skip ;
NEWLINE: [\n\r] -> skip ;
NON_ESCAPE_CHARACTERS: [^\\"] ;
ESCAPE_SEQUENCES: [\\][\\"] ;
IDENTIFIER: [a-zA-Z][0-9a-zA-Z_]+ ;
HEXIDECIMAL_VALUE: [0-9a-fA-F_]+ ;
BINARY_VALUE: [01_]+ ;
DECIMAL_VALUE: [0-9_]+ ;
OCTAL_VALUE: [0-7_]+ ;
INT: [0-9]+ ;
ANNOTATION_NAME: [.a-zA-Z][0-9a-zA-Z_]* ;
COMMENT_CAP: '#' [^\n\r] -> skip ;

// Parsing Rules
fasmFile: fasmLine* EOF ;
fasmLine: (S* setFasmFeature)?
          (S* annotations)?
          (S* COMMENT_CAP)?
          S* NEWLINE ;

setFasmFeature: S* feature featureAddress? S* ('=' S* verilogValue S*)? ;
feature: IDENTIFIER ('.' IDENTIFIER)* ;
featureAddress: '[' DECIMAL_VALUE (':' DECIMAL_VALUE)? ']' ;
verilogValue: INT? S* '\'' ( 'h' S* HEXIDECIMAL_VALUE
                           | 'b' S* BINARY_VALUE
                           | 'd' S* DECIMAL_VALUE
                           | 'o' S* OCTAL_VALUE
                           |        DECIMAL_VALUE ) ;

annotations: '{' S* annotation (',' S* annotation)* S* '}' ;
annotation: ANNOTATION_NAME S* '=' S* '"' annotationValue '"' ;
annotationValue: (NON_ESCAPE_CHARACTERS | ESCAPE_SEQUENCES)* ;

