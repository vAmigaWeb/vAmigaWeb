// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "OptionTypes.h"
#include "Parser.h"
#include <memory>

namespace vamiga {

class OptionParser {

public:

    virtual ~OptionParser() = default;

protected:

    Opt opt;
    i64 arg;
    string unit;

    OptionParser(Opt opt) : opt(opt), arg(0) { };
    OptionParser(Opt opt, const string &unit) : opt(opt), arg(0), unit(unit) { };
    OptionParser(Opt opt, i64 arg) : opt(opt), arg(arg) { };
    OptionParser(Opt opt, i64 arg, const string &unit) : opt(opt), arg(arg), unit(unit) { };

    // Factory method for creating the proper parser instance for an option
    static std::unique_ptr<OptionParser> create(Opt opt, i64 arg = 0);

    // Parses an argument provides as string
    virtual i64 parse(const string &arg) { return 0; }

    // Data providers
    virtual std::vector <std::pair<string,long>> pairs() { return {}; }
    virtual string asPlainString() { return asString(); }
    virtual string asString() = 0;
    virtual string keyList() = 0;
    virtual string argList() = 0;
    virtual string help(i64 arg) { return ""; }

public:

    static i64 parse(Opt opt, const string &arg);
    static std::vector<std::pair<string, long>> pairs(Opt opt);
    static string asPlainString(Opt opt, i64 arg);
    static string asString(Opt opt, i64 arg);
    static string keyList(Opt opt);
    static string argList(Opt opt);
    static string help(Opt opt, i64 arg);
};

class BoolParser : public OptionParser {

public:

    using OptionParser::OptionParser;

    virtual i64 parse(const string &s) override { arg = util::parseBool(s); return arg; }
    virtual string asString() override { return arg ? "true" : "false"; }
    virtual string keyList() override { return "true, false"; }
    virtual string argList() override { return "{ true | false }"; }
};

class NumParser : public OptionParser {

public:

    using OptionParser::OptionParser;

    virtual i64 parse(const string &s) override { arg = util::parseNum(s); return arg; }
    virtual string asPlainString() override { return std::to_string(arg); }
    virtual string asString() override { return asPlainString() + unit; }
    virtual string keyList() override { return "<value>"; }
    virtual string argList() override { return "<value>"; }
};

class HexParser : public OptionParser {

public:

    using OptionParser::OptionParser;

    virtual i64 parse(const string &s) override { arg = util::parseNum(s); return arg; }
    virtual string asPlainString() override;
    virtual string asString() override { return asPlainString() + unit; }
    virtual string keyList() override { return "<value>"; }
    virtual string argList() override { return "<value>"; }
};

template <class T, typename E>
class EnumParser : public OptionParser {

    using OptionParser::OptionParser;

    virtual i64 parse(const string &s) override { return (arg = util::parseEnum<T>(s)); }
    std::vector <std::pair<string,long>> pairs() override { return T::pairs(); }
    virtual string asString() override { return T::key(E(arg)); }
    virtual string keyList() override { return T::keyList(); }
    virtual string argList() override { return T::argList(); }
    virtual string help(i64 arg) override { return T::help(E(arg)); }
};

}
