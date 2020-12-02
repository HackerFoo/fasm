#include "antlr4-runtime.h"
#include "FasmLexer.h"
#include "FasmParser.h"
#include "FasmParserVisitor.h"

using namespace antlr4;
using namespace antlrcpp;

#define DATA                                    \
  std::ostringstream data;                      \
  data << std::uppercase << std::hex;           \
  data

#define GET(x) (context->x() ? visit(context->x()).as<std::string>() : "")

#define EMIT(x)                                 \
  std::ostringstream header;                    \
  header << #x << std::uppercase << std::hex;   \
  header << std::setfill('0') << std::setw(4) << data.str().size();      \
  std::cout << header.str() << data.str() << std::endl; \
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
    it++;
  }
  return count;
}

char hex_digit(int x) {
  assert(x >= 0 && x < 16);
  return (x < 10 ? '0' : 'A' - 10) + x;
}    

class FasmParserBaseVisitor : public FasmParserVisitor {

public:

  static constexpr size_t kHeaderSize = 5;


  FasmParserBaseVisitor(std::ostream &out) : out(out) {}

  /**
   * Visit parse trees produced by FasmParser.
   */
  virtual antlrcpp::Any visitFasmFile(FasmParser::FasmFileContext *context) override {
    visitChildren(context);
    return {};
  }

  virtual antlrcpp::Any visitFasmLine(FasmParser::FasmLineContext *context) override {
    DATA << GET(setFasmFeature)
         << GET(annotations);

    if (context->COMMENT_CAP()) {
      std::string c = context->COMMENT_CAP()->getText();
      c.erase(0, 1); // remove the leading #
      data << "C" << std::setfill('0') << std::setw(4) << c.size() << c;
    }

    data << std::endl; // just to make it more readable
    EMIT(L);
  }

  virtual antlrcpp::Any visitSetFasmFeature(FasmParser::SetFasmFeatureContext *context) override {
    DATA << context->FEATURE()->getText()
         << GET(featureAddress)
         << GET(value);
    EMIT(S);
  }

  virtual antlrcpp::Any visitFeatureAddress(FasmParser::FeatureAddressContext *context) override {
    DATA << std::setfill('0') << std::setw(4) << stol(context->INT(0)->getText());
    if (context->INT(1)) {
      data << std::stoul(context->INT(1)->getText());
    }
    EMIT(r);
  }

  virtual antlrcpp::Any visitVerilogValue(FasmParser::VerilogValueContext *context) override {
    std::string value = "";
    if (context->verilogDigits()) {
      value = visit(context->verilogDigits()).as<std::string>();
      // if (context->INT()) {
      //   int width = std::stoi(context->INT()->getText());
      //   assert(value << );
      // }
    }
    return value;
  }

  virtual antlrcpp::Any visitPlainDecimal(FasmParser::PlainDecimalContext *context) override {
    DATA << std::stol(context->INT()->getText());
    EMIT(p);
  }

  virtual antlrcpp::Any visitHexValue(FasmParser::HexValueContext *context) override {
    DATA;
    std::string value = context->HEXADECIMAL_VALUE()->getText();
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

  virtual antlrcpp::Any visitBinaryValue(FasmParser::BinaryValueContext *context) override {
    DATA;
    std::string value = context->BINARY_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'b
    int bits = (4 - count_without(it, value.end(), '_')) % 4;
    int digit = 0;
    while (it != value.end()) {
      if (*it != '_') {
        digit = (digit << 1) | (*it == '1' ? 1 : 0);
        bits++;
        if (bits == 4) {
          data.put(hex_digit(digit));
          digit = 0;
          bits = 0;
        }
      }
      it++;
    }
    EMIT(b);
  }

  virtual antlrcpp::Any visitDecimalValue(FasmParser::DecimalValueContext *context) override {
    long long integer = 0;
    std::string value = context->DECIMAL_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'd
    while (it != value.end()) {
      if (*it != '_') {
        integer = (integer * 10) + (*it - '0');
      }
      it++;
    }
    DATA << integer;
    EMIT(d);
  }

  virtual antlrcpp::Any visitOctalValue(FasmParser::OctalValueContext *context) override {
    DATA;
    std::string value = context->OCTAL_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'b
    int bits = (4 - (count_without(it, value.end(), '_') * 3) % 4) % 4;
    int digit = 0;
    while (it != value.end()) {
      if (*it != '_') {
        digit = (digit << 3) | (*it - '0');
        bits += 3;
        std::cout << bits << ": " << *it << " " << digit << std::endl;
        if (bits >= 4) {
          data.put(hex_digit(digit >> (bits - 4)));
          digit >>= 4;
          bits -= 4;
        }
      }
      it++;
    }
    assert(!digit);
    EMIT(o);
  }

  virtual antlrcpp::Any visitAnnotations(FasmParser::AnnotationsContext *context) override {
    DATA;
    for (auto &a : context->annotation()) {
      data << visit(a).as<std::string>();
    }
    EMIT(A);
  }

  virtual antlrcpp::Any visitAnnotation(FasmParser::AnnotationContext *context) override {
    std::string name = context->ANNOTATION_NAME()->getText();
    DATA << 'n' << std::setfill('0') << std::setw(4) << name.size() << name;
    if (context->ANNOTATION_VALUE()) {
      std::string value = context->ANNOTATION_VALUE()->getText();
      value.erase(0, 1); // "value" -> value
      value.pop_back();
      data << 'v' << std::setfill('0') << std::setw(4) << value.size() << value;
    }
    EMIT(a);
  }

private:
  std::ostream &out;
};


void parse_fasm(std::istream &in, std::ostream &out) {
  ANTLRInputStream stream(in);
  FasmLexer lexer(&stream);
  CommonTokenStream tokens(&lexer);
  FasmParser parser(&tokens);
  auto *tree = parser.fasmFile();
  FasmParserBaseVisitor(out).visit(tree);
}

/*
char *parse_fasm_string(s) {
  
    return parse_fasm(InputStream(s))
}


  char *parse_fasm_filename(filename) {
    """ Parse FASM file, returning list of FasmLine named tuples."""
    return parse_fasm(FileStream(filename))
      }
*/

int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::ifstream in;
    in.open(argv[1]);
    parse_fasm(in, std::cout);
    return 0;
  } else {
    std::cout << "fasm_parse [filename]" << std::endl;
    return -1;
  }
}
