# Copyright (C) 2017-2020  The SymbiFlow Authors.
#
# Use of this source code is governed by a ISC-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/ISC
#
# SPDX-License-Identifier: ISC

CXXFLAGS += -Wno-attributes -g -O0
LDFLAGS += -lantlr4-runtime

.PHONY: all
all: fasm_parse

PYTHON_FORMAT ?= yapf
format:
	$(IN_ENV) find . -name \*.py $(FORMAT_EXCLUDE) -print0 | xargs -0 -P $$(nproc) yapf -p -i

check-license:
	@./.github/check_license.sh
	@./.github/check_python_scripts.sh


fasm_parse: cpp/FasmParse.o cpp/FasmParser.o cpp/FasmLexer.o
	$(CXX) $(LDFLAGS) $^ -o $@

cpp/FasmParser.cpp \
cpp/FasmParser.h \
cpp/FasmParser.interp \
cpp/FasmParser.tokens \
cpp/FasmParserBaseListener.cpp \
cpp/FasmParserBaseListener.h \
cpp/FasmParserBaseVisitor.cpp \
cpp/FasmParserBaseVisitor.h \
cpp/FasmParserListener.cpp \
cpp/FasmParserListener.h \
cpp/FasmParserVisitor.cpp \
cpp/FasmParserVisitor.h : fasm/FasmParser.g4 cpp/FasmLexer.tokens
	antlr -Dlanguage=Cpp -visitor -Xexact-output-dir -o cpp $<

cpp/FasmLexer.cpp \
cpp/FasmLexer.h \
cpp/FasmLexer.interp \
cpp/FasmLexer.tokens : fasm/FasmLexer.g4
	antlr -Dlanguage=Cpp -Xexact-output-dir -o cpp $<

.PHONY: clean
clean:
	rm -f cpp/FasmLexer* cpp/FasmParser*
	rm -rf build
