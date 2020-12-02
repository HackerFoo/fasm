#include "antlr4-runtime.h"
#include "FasmParser.h"

#define DATA                                    \
  std::stringstream data;                       \
  data

#define GET(x) (context->x() ? visit(context->x()) : "")

#define EMIT(x)                                                         \
  std::stringstream header;                                             \
  header << #x << std::hex << std::setw(4) << data.str().size();        \
  return header.str() + data.str()

int count_without(std::string::iterator start,
                  std::string::iterator end,
                  char c) {
  int count = 0;
  auto it = start;
  while (it != end) {
    if (*it != c) {
      count++;
    }
  }
  return count;
}

char hex_digit(int x) {
  assert(x >= 0 && x < 16);
  return (x < 10 ? '0' : 'A' - 10) + x;
}    

/**
 * This class defines an abstract visitor for a parse tree
 * produced by FasmParser.
 */
class  FasmDataVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  static constexpr size_t kHeaderSize = 5;


  FasmDataVisitor(std::ostream &out) : out(out) {}

  /**
   * Visit parse trees produced by FasmParser.
   */
  antlrcpp::Any visitFasmFile(FasmParser::FasmFileContext *context) override {
    visitChildren(context);
    return {};
  }

  antlrcpp::Any visitFasmLine(FasmParser::FasmLineContext *context) override {
    DATA << GET(setFasmFeature)
         << GET(annotations)
         << GET(comment);
    EMIT(L);
  }

  antlrcpp::Any visitSetFasmFeature(FasmParser::SetFasmFeatureContext *context) override {
    DATA << context->FEATURE()->GetText()
         << GET(featureAddress)
         << GET(value);
    EMIT(S);
  }

  antlrcpp::Any visitFeatureAddress(FasmParser::FeatureAddressContext *context) override {
    DATA << std::hex << std::setw(4) << stol(context->INT[0]->GetText());
    if (context->INT[1]) {
      data << stoul(context->INT[1]->GetText());
    }
    EMIT(A);
  }

  antlrcpp::Any visitVerilogValue(FasmParser::VerilogValueContext *context) override {
    std::string value = "";
    if (context->verilogDigits()) {
      value = visit(context.verilogDigits());
      // if (context->INT()) {
      //   int width = std::stoi(context->INT()->getText());
      //   assert(value << );
      // }
    }
    return value;
  }

  antlrcpp::Any visitPlainDecimal(FasmParser::PlainDecimalContext *context) override {
    DATA << std::hex << std::stol(context->INT[0]->GetText());
    EMIT(p);
  }

  antlrcpp::Any visitHexValue(FasmParser::HexValueContext *context) override {
    DATA;
    std::string value = context->HEXADECIMAL_VALUE().getText();
    auto it = value.begin();
    it += 2; // skip 'h
    while (it != value.end()) {
      if (*it != '_') {
        data << *it;
      }
      it++;
    }
    EMIT(h);
  }

  antlrcpp::Any visitBinaryValue(FasmParser::BinaryValueContext *context) override {
    DATA;
    std::string value = context->BINARY_VALUE().getText();
    auto it = value.begin();
    it += 2; // skip 'b
    int bits = count_without(it, value.end(), '_');
    int digit = 0;
    while (it != value.end()) {
      if (*it != '_') {
        digit = (digit << 1) | (*it == '1' ? 1 : 0);
        if (!(--bits % 4)) {
          data.put(hex_digit(digit));
          digit = 0;
        }
      }
    }
    EMIT(b);
  }

  antlrcpp::Any visitDecimalValue(FasmParser::DecimalValueContext *context) override {
    long long integer = 0;
    std::string value = context->DECIMAL_VALUE().getText();
    auto it = value.begin();
    it += 2; // skip 'd
    while (it != value.end()) {
      if (*it != '_') {
        integer = (integer * 10) + (*it - '0');
      }
    }
    DATA << std::uppercase << std::hex << integer;
    EMIT(d);
  }

  antlrcpp::Any visitOctalValue(FasmParser::OctalValueContext *context) override {
        return ValueFormat.VERILOG_OCTAL, int(ctx.OCTAL_VALUE().getText()[2:].replace('_', ''), 8);

  }

  antlrcpp::Any visitAnnotations(FasmParser::AnnotationsContext *context) override {
        return map(self.visit, ctx.annotation())

  }

  antlrcpp::Any visitAnnotation(FasmParser::AnnotationContext *context) override {
        return Annotation(
            name=ctx.ANNOTATION_NAME().getText(),
            value='' if not ctx.ANNOTATION_NAME() else ctx.ANNOTATION_VALUE().getText()[1:-1]
        )
          }

private:
  std::ostream &out;
};


def parse_fasm(s):
    """ Parse FASM from file or string, returning list of FasmLine named tuples."""
    lexer = FasmLexer(s)
    stream = CommonTokenStream(lexer)
    parser = FasmParser(stream)
    tree = parser.fasmFile()
    return FasmTupleVisitor().visit(tree)


  char *parse_fasm_string(s) {
    """ Parse FASM string, returning list of FasmLine named tuples."""
    return parse_fasm(InputStream(s))
}


  char *parse_fasm_filename(filename) {
    """ Parse FASM file, returning list of FasmLine named tuples."""
    return parse_fasm(FileStream(filename))
      }
