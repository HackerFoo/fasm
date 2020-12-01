parser grammar FasmParser;
options { tokenVocab=FasmLexer; }

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

annotations : BEGIN_ANNOTATION S* annotation (',' S* annotation)* S* END_ANNOTATION ;
annotation : ANNOTATION_NAME S* '=' S* '"' ANNOTATION_VALUE '"' ;
