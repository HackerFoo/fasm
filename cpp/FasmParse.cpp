#include "antlr4-runtime.h"
#include "FasmLexer.h"
#include "FasmParser.h"
#include "FasmParserVisitor.h"

using namespace antlr4;
using namespace antlrcpp;

#define GET(x) (context->x() ? visit(context->x()).as<std::string>() : "")

std::string withHeader(char tag, std::string data) {
  std::ostringstream header;
  header << tag;
  header << std::uppercase << std::hex;
  header << std::setfill('0') << std::setw(8) << data.size();
  return header.str() + data;
}

template<typename T>
class Num {
public:
  T num;
  Num(T num) : num(num) {}
};

template<typename T>
std::ostream &operator<<(std::ostream &s, const Num<T> &num) {
  s << std::uppercase << std::hex;
  s << std::setfill('0') << std::setw(8);
  s << num.num;
  return s;
}

struct Str {
  char tag;
  std::string data;
  Str(char tag, std::string data) : tag(tag), data(data) {}
};

std::ostream &operator<<(std::ostream &s, const Str &str) {
  s << str.tag << Num<size_t>(str.data.size()) << str.data;
  return s;
}

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
    for (auto &line : context->fasmLine()) {
      std::string str = visit(line);
      if (!str.empty()) {
        out << str << std::endl;
      }
    }
    return {};
  }

  virtual antlrcpp::Any visitFasmLine(FasmParser::FasmLineContext *context) override {
    std::ostringstream data;
    data << GET(setFasmFeature)
         << GET(annotations);

    if (context->COMMENT_CAP()) {
      std::string c = context->COMMENT_CAP()->getText();
      c.erase(0, 1); // remove the leading #
      data << Str('#', c);
    }

    if (!data.str().empty()) {
      return withHeader('l', data.str());
    } else {
      return std::string();
    }
  }

  virtual antlrcpp::Any visitSetFasmFeature(FasmParser::SetFasmFeatureContext *context) override {
    std::ostringstream data;
    data << context->FEATURE()->getText()
         << GET(featureAddress)
         << GET(value);
    return withHeader('f', data.str());
  }

  virtual antlrcpp::Any visitFeatureAddress(FasmParser::FeatureAddressContext *context) override {
    std::ostringstream data;
    data << Num<unsigned long>(std::stoul(context->INT(0)->getText()));

    if (context->INT(1)) {
      data << Num<unsigned long>(std::stoul(context->INT(1)->getText()));
    }
    return withHeader(':', data.str());
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
    std::ostringstream data;
    data << std::stol(context->INT()->getText());
    return withHeader('p', data.str());
  }

  virtual antlrcpp::Any visitHexValue(FasmParser::HexValueContext *context) override {
    std::ostringstream data;
    std::string value = context->HEXADECIMAL_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'h
    while (it != value.end()) {
      if (*it != '_') {
        data << *it;
      }
      it++;
    }
    return withHeader('h', data.str());
  }

  virtual antlrcpp::Any visitBinaryValue(FasmParser::BinaryValueContext *context) override {
    std::ostringstream data;    
    std::string value = context->BINARY_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'b
    int bits = (4 - count_without(it, value.end(), '_') % 4) % 4;
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
    return withHeader('b', data.str());
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
    std::ostringstream data;
    data << std::uppercase << std::hex << integer;
    return withHeader('d', data.str());
  }

  virtual antlrcpp::Any visitOctalValue(FasmParser::OctalValueContext *context) override {
    std::ostringstream data;   
    std::string value = context->OCTAL_VALUE()->getText();
    auto it = value.begin();
    it += 2; // skip 'b
    int bits = (4 - (count_without(it, value.end(), '_') * 3) % 4) % 4;
    int digit = 0;
    while (it != value.end()) {
      if (*it != '_') {
        digit = (digit << 3) | (*it - '0');
        bits += 3;
        if (bits >= 4) {
          data.put(hex_digit(digit >> (bits - 4)));
          digit >>= 4;
          bits -= 4;
        }
      }
      it++;
    }
    assert(!digit);
    return withHeader('o', data.str());
  }

  virtual antlrcpp::Any visitAnnotations(FasmParser::AnnotationsContext *context) override {
    std::ostringstream data;   
    for (auto &a : context->annotation()) {
      data << visit(a).as<std::string>();
    }
    return withHeader('{', data.str());
  }

  virtual antlrcpp::Any visitAnnotation(FasmParser::AnnotationContext *context) override {
    std::ostringstream data;
    data << Str('.', context->ANNOTATION_NAME()->getText());
    if (context->ANNOTATION_VALUE()) {
      std::string value = context->ANNOTATION_VALUE()->getText();
      value.erase(0, 1); // "value" -> value
      value.pop_back();
      data << Str('=', value);
    }
    return withHeader('a', data.str());
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

static std::ostringstream output_buffer;

const char *parse_fasm_string(const char *in) {
  output_buffer.str(std::string()); // clear the output
  std::istringstream input(in);
  parse_fasm(input, output_buffer);
  return output_buffer.str().c_str();
}

const char *parse_fasm_filename(const char *path) {
  output_buffer.str(std::string()); // clear the output
  std::fstream input(std::string(path), input.in);
  if(!input.is_open()) return nullptr;
  parse_fasm(input, output_buffer);
  return output_buffer.str().c_str();
}

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
