parser grammar FasmParser;
options { tokenVocab=FasmLexer; }

fasmFile : fasmLine (NEWLINE fasmLine)* EOF ;
fasmLine : setFasmFeature?
           annotations?
           COMMENT_CAP?
         ;

setFasmFeature : FEATURE featureAddress? (EQUAL value)? ;
featureAddress : '[' INT (':' INT)? ']' ;

value : INT? verilogDigits # VerilogValue
      | INT                # PlainDecimal
      ;

verilogDigits : HEXADECIMAL_VALUE # HexValue
              | BINARY_VALUE      # BinaryValue
              | DECIMAL_VALUE     # DecimalValue
              | OCTAL_VALUE       # OctalValue
              ;

annotations : BEGIN_ANNOTATION annotation (',' annotation)* END_ANNOTATION ;
annotation : ANNOTATION_NAME ANNOTATION_EQUAL ANNOTATION_VALUE ;
