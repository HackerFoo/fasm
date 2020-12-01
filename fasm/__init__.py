#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2017-2020  The SymbiFlow Authors.
#
# Use of this source code is governed by a ISC-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/ISC
#
# SPDX-License-Identifier: ISC

from __future__ import print_function
import os.path
import argparse
from collections import namedtuple
import enum

from antlr4 import *
from fasm_lex.FasmLexer import FasmLexer
from fasm.FasmParser import FasmParser
from fasm.FasmVisitor import FasmVisitor

class ValueFormat(enum.Enum):
    PLAIN = 0
    VERILOG_DECIMAL = 1
    VERILOG_HEX = 2
    VERILOG_BINARY = 3
    VERILOG_OCTAL = 4


# Python version of a SetFasmFeature line.
# feature is a string
# start and end are ints.  When FeatureAddress is missing, start=None and
# end=None.
# value is an int.
#
# When FeatureValue is missing, value=1.
# value_format determines what to output the value.
# Should be a ValueFormat or None.
# If None, value must be 1 and the value will be omited.
SetFasmFeature = namedtuple(
    'SetFasmFeature', 'feature start end value value_format')

Annotation = namedtuple('Annotation', 'name value')

# Python version of FasmLine.
# set_feature should be a SetFasmFeature or None.
# annotations should be a tuple of Annotation or None.
# comment should a string or None.
FasmLine = namedtuple('FasmLine', 'set_feature annotations comment')


def parse_fasm(s):
    """ Parse FASM from file or string, returning list of FasmLine named tuples."""
    lexer = FasmLexer(s)
    stream = CommonTokenStream(lexer)
    parser = FasmParser(stream)
    tree = parser.fasmFile()
    return FasmTupleVisitor().visit(tree)


def parse_fasm_string(s):
    """ Parse FASM string, returning list of FasmLine named tuples."""
    return parse_fasm(InputStream(s))


def parse_fasm_filename(filename):
    """ Parse FASM file, returning list of FasmLine named tuples."""
    return parse_fasm(FileStream(filename))


def fasm_value_to_str(value, width, value_format):
    """ Convert value from SetFasmFeature to a string. """
    if value_format == ValueFormat.PLAIN:
        return '{}'.format(value)
    elif value_format == ValueFormat.VERILOG_HEX:
        return "{}'h{:X}".format(width, value)
    elif value_format == ValueFormat.VERILOG_DECIMAL:
        return "{}'d{}".format(width, value)
    elif value_format == ValueFormat.VERILOG_OCTAL:
        return "{}'o{:o}".format(width, value)
    elif value_format == ValueFormat.VERILOG_BINARY:
        return "{}'b{:b}".format(width, value)
    else:
        assert False, value_format


def set_feature_width(set_feature):
    if set_feature.end is None:
        return 1
    else:
        assert set_feature.start is not None
        assert set_feature.start >= 0
        assert set_feature.end >= set_feature.start

        return set_feature.end - set_feature.start + 1


def set_feature_to_str(set_feature, check_if_canonical=False):
    """ Convert SetFasmFeature tuple to string. """
    feature_width = set_feature_width(set_feature)
    max_feature_value = 2**feature_width
    assert set_feature.value < max_feature_value

    if check_if_canonical:
        assert feature_width == 1
        assert set_feature.end is None
        if set_feature.start is not None:
            assert set_feature.start != 0
        assert set_feature.value_format is None

    feature = set_feature.feature
    address = ''
    feature_value = ''

    if set_feature.start is not None:
        if set_feature.end is not None:
            address = '[{}:{}]'.format(set_feature.end, set_feature.start)
        else:
            address = '[{}]'.format(set_feature.start)

    if set_feature.value_format is not None:
        feature_value = ' = {}'.format(
            fasm_value_to_str(
                value=set_feature.value,
                width=feature_width,
                value_format=set_feature.value_format))

    return '{}{}{}'.format(feature, address, feature_value)


def canonical_features(set_feature):
    """ Yield SetFasmFeature tuples that are of canonical form.

    EG width 1, and value 1.
    """
    if set_feature.value == 0:
        return

    if set_feature.start is None:
        assert set_feature.value == 1
        assert set_feature.end is None
        yield SetFasmFeature(
            feature=set_feature.feature,
            start=None,
            end=None,
            value=1,
            value_format=None,
        )

        return

    if set_feature.start is not None and set_feature.end is None:
        assert set_feature.value == 1

        if set_feature.start == 0:
            yield SetFasmFeature(
                feature=set_feature.feature,
                start=None,
                end=None,
                value=1,
                value_format=None,
            )
        else:
            yield SetFasmFeature(
                feature=set_feature.feature,
                start=set_feature.start,
                end=None,
                value=1,
                value_format=None,
            )

        return

    assert set_feature.start is not None
    assert set_feature.start >= 0
    assert set_feature.end >= set_feature.start

    for address in range(set_feature.start, set_feature.end + 1):
        value = (set_feature.value >> (address - set_feature.start)) & 1
        if value:
            if address == 0:
                yield SetFasmFeature(
                    feature=set_feature.feature,
                    start=None,
                    end=None,
                    value=1,
                    value_format=None,
                )
            else:
                yield SetFasmFeature(
                    feature=set_feature.feature,
                    start=address,
                    end=None,
                    value=1,
                    value_format=None,
                )

class FasmTupleVisitor(FasmVisitor):
    def visitFasmFile(self, ctx):
        return map(self.visit, ctx.getChildren())
        
    def visitSetFasmFeature(self, ctx):
        start = None
        end = None
        value = 1
        value_format = None

        if ctx.value():
            value_format, value = self.visit(ctx.value())
        
        if ctx.featureAddress():
            start, end = self.visit(ctx.featureAddress())

        return SetFasmFeature(
            feature=ctx.FEATURE().getText(),
            start=start,
            end=end,
            value=value,
            value_format=value_format
        )

    def visitFeatureAddress(self, ctx):
        start = int(ctx.INT(0).getText(), 10)
        end = None if not ctx.INT(1) else int(ctx.INT(1).getText(), 10)
        return start, end

    def visitVerilogValue(self, ctx):
        value_format, value = self.visit(ctx.verilogDigits())
        if True:
            assert value < 2 ** int(ctx.INT().getText()), "value larger than bit width"
        return value_format, value
    
    def visitHexValue(self, ctx):
        return ValueFormat.VERILOG_HEX, int(ctx.HEXADECIMAL_VALUE().getText()[2:].replace('_', ''), 16);

    def visitBinaryValue(self, ctx):
        return ValueFormat.VERILOG_BINARY, int(ctx.BINARY_VALUE().getText()[2:].replace('_', ''), 2);

    def visitDecimalValue(self, ctx):
        return ValueFormat.VERILOG_DECIMAL, int(ctx.DECIMAL_VALUE().getText()[2:].replace('_', ''), 10);

    def visitOctalValue(self, ctx):
        return ValueFormat.VERILOG_OCTAL, int(ctx.OCTAL_VALUE().getText()[2:].replace('_', ''), 8);

    def visitPlainDecimal(self, ctx):
        return ValueFormat.PLAIN, int(ctx.INT().getText(), 10);

    def visitAnnotations(self, ctx):
        return map(self.visit, ctx.getChildren())

    def visitAnnotation(self, ctx):
        return Annotation(
            name=ctx.ANNOTATION_NAME().getText(),
            value='' if not ctx.ANNOTATION_NAME() else ctx.ANNOTATION_NAME().getText()
        )


def fasm_line_to_string(fasm_line, canonical=False):
    if canonical:
        if fasm_line.set_feature:
            for feature in canonical_features(fasm_line.set_feature):
                yield set_feature_to_str(feature, check_if_canonical=True)

        return

    parts = []

    if fasm_line.set_feature:
        parts.append(set_feature_to_str(fasm_line.set_feature))

    if fasm_line.annotations and not canonical:
        annotations = '{{ {} }}'.format(
            ', '.join(
                '{} = "{}"'.format(annotation.name, annotation.value)
                for annotation in fasm_line.annotations))

        parts.append(annotations)

    if fasm_line.comment is not None and not canonical:
        comment = '#{}'.format(fasm_line.comment)
        parts.append(comment)

    if len(parts) == 0 and canonical:
        return

    yield ' '.join(parts)


def fasm_tuple_to_string(model, canonical=False):
    """ Returns string of FASM file for the model given.

    Note that calling parse_fasm_filename and then calling fasm_tuple_to_string
    will result in all optional whitespace replaced with one space.
    """

    lines = []
    for fasm_line in model:
        for line in fasm_line_to_string(fasm_line, canonical=canonical):
            lines.append(line)

    if canonical:
        lines = list(sorted(set(lines)))

    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser('FASM tool')
    parser.add_argument('file', help='Filename to process')
    parser.add_argument(
        '--canonical',
        action='store_true',
        help='Return canonical form of FASM.')

    args = parser.parse_args()

    fasm_tuples = parse_fasm_filename(args.file)

    print(fasm_tuple_to_string(fasm_tuples, args.canonical))


if __name__ == '__main__':
    main()
