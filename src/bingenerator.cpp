#include "bingenerator.h"
#include "ParseException.h"
#include "token.h"
#include "opcodes.h"

BinGenerator::BinGenerator(const AsmLexer& lexer, const AsmOpts& opts) :
    lexer(lexer), opts(opts), fwriter(opts.out_file, FileBit::BINARY),
    curr_label(), curr_addr(0x0200) {}

void BinGenerator::parse()
{
    Token tok;
    do {
        tok = lexer.get_next_token();
        std::string str = tok.second;
        VLOG(opts, "Retrieved token: " << str);

        auto type = tok.first;
        if (type == TokenType::LABEL) {
            process_label(str);
        } else if (type == TokenType::OPERATOR) {
            process_operator(str);
        } else {
            throw ParseException(str + " is not a valid starting token! (OPERATOR|LABEL) expected!");
        }
    } while (!tok.second.empty());
}

void BinGenerator::dump_asm() const
{
    std::cout << "-------- Asm Dump --------\n";
    for (const auto& i : instructions) {
        std::cout << from_hex(i.addr) << " -- " << from_hex(get_opcode(i.op, i.args)) << " ; ";
        std::string s = i.op + " ";
        for (const auto& a : i.args) {
            s += a;
            s += ", ";
        }
        s.pop_back();
        std::cout << s << "\n";
    }
    std::cout << "-------- End Dump --------\n";
}

void BinGenerator::generate_bin()
{
    /* First create the instruction set and assign addresses to labels and operations */
    parse();

    /* Now perform the binary conversion and write it to the file */
    for (const auto& i : instructions) {
        fwriter.write(get_opcode(i.op, i.args));
    }

    if (opts.verbose) {
        VLOG(opts, "-------- Label Addresses --------");
        for (const auto& it : label_addrs) {
            VLOG(opts, it.first << " -> " << from_hex(it.second));
        }
        VLOG(opts, "-------- End Addresses --------");
    }

    /* Now dump assembly to the screen if requested */
    if (opts.dump_asm) {
        dump_asm();
    }
}

bool BinGenerator::label_exists(const std::string& lbl) const
{
    return label_addrs.find(lbl) != label_addrs.end();
}

uint16_t BinGenerator::get_opcode(const std::string& op, const std::vector<std::string>& args) const
{
    auto itr = OPERATORS.find(op);
    return itr->second(args);
}

void BinGenerator::process_label(const std::string& label)
{
    if (label_exists(label)) {
        throw ParseException(label + " label is redefined!");
    }

    curr_label = label;
    if (!curr_label.empty()) {
        label_addrs.insert({curr_label, curr_addr});
    }
}

void BinGenerator::process_operator(const std::string& str)
{
    /* Not implemented */
    if (str == "SYS") {
        return;
    }

    std::vector<std::string> args;

    /* Expects no arguments */
    if (one_of<std::string>(str, {"CLR", "RET"})) {

        /* Expects label or hex */
    } else if (one_of<std::string>(str, {"JMP", "CALL", "ZJMP", "ILOAD"})) {
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::LABEL && t1.first != TokenType::HEX) {
            throw ParseException(str + " expects a label or hex address as an operand!");
        }
        args.push_back(t1.second);

        /* Expects register, comma, and hex */
    } else if (one_of<std::string>(str, {"SKE", "SKNE", "SKRE", "LOAD", "ADD", "RAND"})) {
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + str + "!\n");
        }
        auto t2 = lexer.get_next_token();
        if (t2.first != TokenType::COMMA) {
            throw ParseException("COMMA expected after " + t1.second + "!\n");
        }
        auto t3 = lexer.get_next_token();
        if (t3.first != TokenType::HEX) {
            throw ParseException("HEX expected after " + t2.second + "!\n");
        }
        args.push_back(t1.second);
        args.push_back(t3.second);

        /* Expects 2 registers */
    } else if (one_of<std::string>(str, {"ASN", "OR", "AND", "XOR", "RADD", "SUB", "RSUB", "SKRNE"})) {
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + str + "!\n");
        }
        auto t2 = lexer.get_next_token();
        if (t2.first != TokenType::COMMA) {
            throw ParseException("COMMA expected after " + t1.second + "!\n");
        }
        auto t3 = lexer.get_next_token();
        if (t3.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + t2.second + "!\n");
        }
        args.push_back(t1.second);
        args.push_back(t3.second);

        /* Expects one register */
    } else if (one_of<std::string>(str, {"SHR", "SHL", "SKK", "SKNK", "DELA", "KEYW", "DELR", "SNDR", "IADD", "SILS", "BCD", "DUMP", "IDUMP"})) {
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + str + "!\n");
        }
        args.push_back(t1.second);

    } else if (str == "DRAW") { /* DRAW is the only operator to take three operands */
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + str + "!\n");
        }
        auto t2 = lexer.get_next_token();
        if (t2.first != TokenType::COMMA) {
            throw ParseException("COMMA expected after " + t1.second + "!\n");
        }
        auto t3 = lexer.get_next_token();
        if (t3.first != TokenType::REGISTER) {
            throw ParseException("REGISTER expected after " + t2.second + "!\n");
        }
        auto t4 = lexer.get_next_token();
        if (t4.first != TokenType::COMMA) {
            throw ParseException("COMMA expected after " + t3.second + "!\n");
        }
        auto t5 = lexer.get_next_token();
        if (t5.first != TokenType::HEX) {
            throw ParseException("HEX expected after " + t4.second + "!\n");
        }
        args.push_back(t1.second);
        args.push_back(t3.second);
        args.push_back(t5.second);

    } else { /* Must be special instruction LB */
        auto t1 = lexer.get_next_token();
        if (t1.first != TokenType::HEX) {
            throw ParseException("HEX expected after " + str + "!\n");
        }
        args.push_back(t1.second);
    }

    /* Now put it into the symbol table */
    instructions.push_back({curr_label, str, args, curr_addr});
    curr_addr += 16;
}
