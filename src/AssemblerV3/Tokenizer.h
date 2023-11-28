#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <../src/util/File.h>

#include <vector>
#include <string>
#include <map>
#include <set>

class Tokenizer;

class Tokenizer {
    public:
        enum Type {
            UNKNOWN,

            TEXT, WHITESPACE_SPACE, WHITESPACE_TAB, WHITESPACE_NEWLINE, WHITESPACE, 
            COMMENT_SINGLE_LINE, COMMENT_MULTI_LINE,

            // PREPROCESSOR DIRECTIVES
            PREPROCESSOR_INCLUDE, 
            PREPROCESSOR_MACRO, PREPROCESSOR_MACRET, PREPROCESSOR_MACEND, PREPROCESSOR_INVOKE, 
            PREPROCESSOR_DEFINE, PREPROCESSOR_UNDEF, 
            PREPROCESSOR_IFDEF, PREPROCESSOR_IFNDEF, 
            PREPROCESSOR_ELSE, PREPROCESSOR_ELSEDEF, PREPROCESSOR_ELSENDEF, 
            PREPROCESSOR_ENDIF,

            // VARIABLE TYPES
            VARIABLE_TYPE_BYTE, VARIABLE_TYPE_DBYTE, VARIABLE_TYPE_WORD, VARIABLE_TYPE_DWORD,
            VARIABLE_TYPE_CHAR, VARIABLE_TYPE_STRING, VARIABLE_TYPE_FLOAT, VARIABLE_TYPE_DOUBLE,
            VARIABLE_TYPE_BOOLEAN,

            // ASSEMBLER DIRECTIVES
            ASSEMBLER_GLOBAL,
            ASSEMBLER_EXTERN,
            ASSEMBLER_EQU,
            ASSEMBLER_ORG,
            ASSEMBLER_SCOPE,
            ASSEMBLER_SCEND,
            ASSEMBLER_DB_LOW_ENDIAN,
            ASSEMBLER_DDB_LOW_ENDIAN,
            ASSEMBLER_DDB_HIGH_ENDIAN,
            ASSEMBLER_DW_LOW_ENDIAN,
            ASSEMBLER_DW_HIGH_ENDIAN,
            ASSEMBLER_DDW_LOW_ENDIAN,
            ASSEMBLER_DDW_HIGH_ENDIAN,
            ASSEMBLER_ASCII,
            ASSEMBLER_ASCIZ,
            ASSEMBLER_ADVANCE,
            ASSEMBLER_FILL,
            ASSEMBLER_SPACE,
            ASSEMBLER_CHECKPC,
            ASSEMBLER_ALIGN,
            ASSEMBLER_BSS,
            ASSEMBLER_BSS_ABSOLUTE,
            ASSEMBLER_DATA,
            ASSEMBLER_DATA_ABSOLUTE,
            ASSEMBLER_CODE,
            ASSEMBLER_CODE_ABSOLUTE,
            ASSEMBLER_STOP,

            NUMBER_SIGN,
            LITERAL_NUMBER_BINARY, LITERAL_NUMBER_OCTAL, LITERAL_NUMBER_DECIMAL, LITERAL_NUMBER_HEXADECIMAL, 
            LITERAL_CHAR, LITERAL_STRING,

            SYMBOL, 
            COLON, COMMA, SEMICOLON,
            OPEN_PARANTHESIS, CLOSE_PARANTHESIS, OPEN_BRACKET, CLOSE_BRACKET, OPEN_BRACE, CLOSE_BRACE,

            OPERATOR_ADDITION, OPERATOR_SUBTRACTION, OPERATOR_MULTIPLICATION, OPERATOR_DIVISION, OPERATOR_MODULUS,
            OPERATOR_BITWISE_LEFT_SHIFT, OPERATOR_BITWISE_RIGHT_SHIFT, 
            OPERATOR_BITWISE_XOR, OPERATOR_BITWISE_AND, OPERATOR_BITWISE_OR, OPERATOR_BITWISE_COMPLEMENT,
            OPERATOR_LOGICAL_NOT, OPERATOR_LOGICAL_EQUAL, OPERATOR_LOGICAL_NOT_EQUAL,
            OPERATOR_LOGICAL_LESS_THAN, OPERATOR_LOGICAL_GREATER_THAN, 
            OPERATOR_LOGICAL_LESS_THAN_OR_EQUAL, OPERATOR_LOGICAL_GREATER_THAN_OR_EQUAL,
            OPERATOR_LOGICAL_OR, OPERATOR_LOGICAL_AND,
        };

        inline static const std::map<Type, std::string> TYPE_TO_NAME_MAP = {
            {UNKNOWN, "UNKNOWN"},

            {TEXT, "TEXT"}, 
            {WHITESPACE_SPACE, "WHITESPACE_SPACE"}, {WHITESPACE_TAB, "WHITE_SPACE_TAB"}, {WHITESPACE_NEWLINE, "WHITESPACE_NEWLINE"},
            {COMMENT_SINGLE_LINE, "COMMENT_SINGLE_LINE"}, {COMMENT_MULTI_LINE, "COMMENT_MULTI_LINE"},

            {PREPROCESSOR_INCLUDE, "PREPROCESSOR_INCLUDE"},
            {PREPROCESSOR_MACRO, "PREPROCESSOR_MACRO"}, {PREPROCESSOR_MACRET, "PREPROCESSOR_MACRET"}, 
            {PREPROCESSOR_MACEND, "PREPROCESSOR_MACEND"}, {PREPROCESSOR_INVOKE, "PREPROCESSOR_INVOKE"},
            {PREPROCESSOR_DEFINE, "PREPROCESSOR_DEFINE"}, {PREPROCESSOR_UNDEF, "PREPROCESSOR_UNDEF"},
            {PREPROCESSOR_IFDEF, "PREPROCESSOR_IFDEF"}, {PREPROCESSOR_IFNDEF, "PREPROCESSOR_IFNDEF"},
            {PREPROCESSOR_ELSE, "PREPROCESSOR_ELSE"}, {PREPROCESSOR_ELSEDEF, "PREPROCESSOR_ELSEDEF"}, 
            {PREPROCESSOR_ELSENDEF, "PREPROCESSOR_ELSENDEF"},
            {PREPROCESSOR_ENDIF, "PREPROCESSOR_ENDIF"},

            {VARIABLE_TYPE_BYTE, "VARIABLE_TYPE_BYTE"}, {VARIABLE_TYPE_DBYTE, "VARIABLE_TYPE_DBYTE"},
            {VARIABLE_TYPE_WORD, "VARIABLE_TYPE_WORD"}, {VARIABLE_TYPE_DWORD, "VARIABLE_TYPE_DWORD"},
            {VARIABLE_TYPE_CHAR, "VARIABLE_TYPE_CHAR"}, {VARIABLE_TYPE_STRING, "VARIABLE_TYPE_STRING"},
            {VARIABLE_TYPE_FLOAT, "VARIABLE_TYPE_FLOAT"}, {VARIABLE_TYPE_DOUBLE, "VARIABLE_TYPE_DOUBLE"},
            {VARIABLE_TYPE_BOOLEAN, "VARIABLE_TYPE_BOOLEAN"},

            {ASSEMBLER_GLOBAL, "ASSEMBLER_GLOBAL"},
            {ASSEMBLER_EXTERN, "ASSEMBLER_EXTERN"},
            {ASSEMBLER_EQU, "ASSEMBLER_EQU"},
            {ASSEMBLER_ORG, "ASSEMBLER_ORG"},
            {ASSEMBLER_SCOPE, "ASSEMBLER_SCOPE"},
            {ASSEMBLER_SCEND, "ASSEMBLER_SCEND"},
            {ASSEMBLER_DB_LOW_ENDIAN, "ASSEMBLER_DB_LOW_ENDIAN"},
            {ASSEMBLER_DDB_LOW_ENDIAN, "ASSEMBLER_DDB_LOW_ENDIAN"},
            {ASSEMBLER_DDB_HIGH_ENDIAN, "ASSEMBLER_DDB_HIGH_ENDIAN"},
            {ASSEMBLER_DW_LOW_ENDIAN, "ASSEMBLER_DW_LOW_ENDIAN"},
            {ASSEMBLER_DW_HIGH_ENDIAN, "ASSEMBLER_DW_HIGH_ENDIAN"},
            {ASSEMBLER_DDW_LOW_ENDIAN, "ASSEMBLER_DDW_LOW_ENDIAN"},
            {ASSEMBLER_DDW_HIGH_ENDIAN, "ASSEMBLER_DDW_HIGH_ENDIAN"},
            {ASSEMBLER_ASCII, "ASSEMBLER_ASCII"},
            {ASSEMBLER_ASCIZ, "ASSEMBLER_ASCIZ"},
            {ASSEMBLER_ADVANCE, "ASSEMBLER_ADVANCE"},
            {ASSEMBLER_FILL, "ASSEMBLER_FILL"},
            {ASSEMBLER_SPACE, "ASSEMBLER_SPACE"},
            {ASSEMBLER_CHECKPC, "ASSEMBLER_CHECKPC"},
            {ASSEMBLER_ALIGN, "ASSEMBLER_ALIGN"},
            {ASSEMBLER_BSS, "ASSEMBLER_BSS"},
            {ASSEMBLER_BSS_ABSOLUTE, "ASSEMBLER_BSS_ABSOLUTE"},
            {ASSEMBLER_DATA, "ASSEMBLER_DATA"},
            {ASSEMBLER_DATA_ABSOLUTE, "ASSEMBLER_DATA_ABSOLUTE"},
            {ASSEMBLER_CODE, "ASSEMBLER_CODE"},
            {ASSEMBLER_CODE_ABSOLUTE, "ASSEMBLER_CODE_ABSOLUTE"},
            {ASSEMBLER_STOP, "ASSEMBLER_STOP"},

            {NUMBER_SIGN, "NUMBER_SIGN"},
            {LITERAL_NUMBER_BINARY, "LITERAL_NUMBER_BINARY"}, {LITERAL_NUMBER_OCTAL, "LITERAL_NUMBER_OCTAL"},
            {LITERAL_NUMBER_DECIMAL, "LITERAL_NUMBER_DECIMAL"}, {LITERAL_NUMBER_HEXADECIMAL, "LITERAL_NUMBER_HEXADECIMAL"},
            {LITERAL_CHAR, "LITERAL_CHAR"}, {LITERAL_STRING, "LITERAL_STRING"},
            {SYMBOL, "SYMBOL"}, 
            {COLON, "COLON"}, {COMMA, "COMMA"}, {SEMICOLON, "SEMICOLON"},
            {OPEN_PARANTHESIS, "OPEN_PARANTHESIS"}, {CLOSE_PARANTHESIS, "CLOSE_PARANTHESIS"}, 
            {OPEN_BRACKET, "OPEN_BRACKET"}, {CLOSE_BRACKET, "CLOSE_BRACKET"}, 
            {OPEN_BRACE, "OPEN_BRACE"}, {CLOSE_BRACE, "CLOSE_BRACE"},

            {OPERATOR_ADDITION, "OPERATOR_ADDITION"}, {OPERATOR_SUBTRACTION, "OPERATOR_SUBTRACTION"},
            {OPERATOR_MULTIPLICATION, "OPERATOR_MULTIPLICATION"}, {OPERATOR_DIVISION, "OPERATOR_DIVISION"},
            {OPERATOR_MODULUS, "OPERATOR_MODULUS"}, {OPERATOR_BITWISE_LEFT_SHIFT, "OPERATOR_BITWISE_LEFT_SHIFT"},
            {OPERATOR_BITWISE_RIGHT_SHIFT, "OPERATOR_BITWISE_RIGHT_SHIFT"}, {OPERATOR_BITWISE_XOR, "OPERATOR_BITWISE_XOR"},
            {OPERATOR_BITWISE_AND, "OPERATOR_BITWISE_AND"}, {OPERATOR_BITWISE_OR, "OPERATOR_BITWISE_OR"},
            {OPERATOR_BITWISE_COMPLEMENT, "OPERATOR_BITWISE_COMPLEMENT"}, {OPERATOR_LOGICAL_NOT, "OPERATOR_LOGICAL_NOT"},
            {OPERATOR_LOGICAL_EQUAL, "OPERATOR_LOGICAL_EQUAL"}, {OPERATOR_LOGICAL_NOT_EQUAL, "OPERATOR_LOGICAL_NOT_EQUAL"},
            {OPERATOR_LOGICAL_LESS_THAN, "OPERATOR_LOGICAL_LESS_THAN"}, {OPERATOR_LOGICAL_GREATER_THAN, "OPERATOR_LOGICAL_GREATER_THAN"},
            {OPERATOR_LOGICAL_LESS_THAN_OR_EQUAL, "OPERATOR_LOGICAL_LESS_THAN_OR_EQUAL"}, {OPERATOR_LOGICAL_GREATER_THAN_OR_EQUAL, "OPERATOR_LOGICAL_GREATER_THAN_OR_EQUAL"},
            {OPERATOR_LOGICAL_OR, "OPERATOR_LOGICAL_OR"}, {OPERATOR_LOGICAL_AND, "OPERATOR_LOGICAL_AND"},
        };

        inline static const std::set<Type> WHITESPACES = {
            WHITESPACE_SPACE, WHITESPACE_TAB, WHITESPACE_NEWLINE
        };

        inline static const std::set<Type> COMMENTS = {
            COMMENT_SINGLE_LINE, COMMENT_MULTI_LINE
        };

        inline static const std::set<Type> PREPROCESSOR_DIRECTIVES = {
            PREPROCESSOR_INCLUDE, PREPROCESSOR_MACRO, PREPROCESSOR_MACRET, PREPROCESSOR_MACEND, PREPROCESSOR_INVOKE, 
            PREPROCESSOR_DEFINE, PREPROCESSOR_UNDEF, PREPROCESSOR_IFDEF, PREPROCESSOR_IFNDEF, PREPROCESSOR_ELSE, 
            PREPROCESSOR_ELSEDEF, PREPROCESSOR_ELSENDEF, PREPROCESSOR_ENDIF
        };

        inline static const std::set<Type> VARIABLE_TYPES = {
            VARIABLE_TYPE_BYTE, VARIABLE_TYPE_DBYTE, VARIABLE_TYPE_WORD, VARIABLE_TYPE_DWORD,
            VARIABLE_TYPE_CHAR, VARIABLE_TYPE_STRING, VARIABLE_TYPE_FLOAT, VARIABLE_TYPE_DOUBLE,
            VARIABLE_TYPE_BOOLEAN
        };

        inline static const std::set<Type> ASSEMBLER_DIRECTIVES = {
            ASSEMBLER_EQU, ASSEMBLER_ORG
        };

        inline static const std::set<Type> LITERAL_NUMBERS = {
            LITERAL_NUMBER_BINARY, LITERAL_NUMBER_OCTAL, LITERAL_NUMBER_DECIMAL, LITERAL_NUMBER_HEXADECIMAL
        };

        inline static const std::set<Type> LITERAL_VALUES = {
            LITERAL_NUMBER_BINARY, LITERAL_NUMBER_OCTAL, LITERAL_NUMBER_DECIMAL, LITERAL_NUMBER_HEXADECIMAL,
            LITERAL_CHAR, LITERAL_STRING
        };

        inline static const std::set<Type> OPERATORS = {
            OPERATOR_ADDITION, OPERATOR_SUBTRACTION, OPERATOR_MULTIPLICATION, OPERATOR_DIVISION, OPERATOR_MODULUS,
            OPERATOR_BITWISE_LEFT_SHIFT, OPERATOR_BITWISE_RIGHT_SHIFT, OPERATOR_BITWISE_XOR, OPERATOR_BITWISE_AND, 
            OPERATOR_BITWISE_OR, OPERATOR_BITWISE_COMPLEMENT, OPERATOR_LOGICAL_NOT, OPERATOR_LOGICAL_EQUAL, 
            OPERATOR_LOGICAL_NOT_EQUAL, OPERATOR_LOGICAL_LESS_THAN, OPERATOR_LOGICAL_GREATER_THAN, 
            OPERATOR_LOGICAL_LESS_THAN_OR_EQUAL, OPERATOR_LOGICAL_GREATER_THAN_OR_EQUAL, OPERATOR_LOGICAL_OR, 
            OPERATOR_LOGICAL_AND
        };

        inline static const std::vector<std::pair<std::string, Type>> TOKEN_SPEC = {
			{"^ ", WHITESPACE_SPACE}, {"^\\t", WHITESPACE_TAB}, {"^\\n", WHITESPACE_NEWLINE},
			{"^[\\s^[ \\n\\t]]+", WHITESPACE},
            {"^;\\*[^*]*\\*+(?:[^;*][^*]*\\*+)*;", COMMENT_MULTI_LINE}, {"^;.*", COMMENT_SINGLE_LINE},
			{"^\\{", OPEN_BRACE}, {"^\\}", CLOSE_BRACE},
			{"^\\[", OPEN_BRACKET}, 	{"^\\]", CLOSE_BRACKET},
			{"^\\(", OPEN_PARANTHESIS},{"^\\)", CLOSE_PARANTHESIS},
			{"^,", COMMA}, {"^:", COLON}, {"^;", SEMICOLON},

			{"^#include(?=\\s)", PREPROCESSOR_INCLUDE},
			{"^#macro(?=\\s)", PREPROCESSOR_MACRO},
			{"^#macret(?=\\s)", PREPROCESSOR_MACRET},
			{"^#macend(?=\\s)", PREPROCESSOR_MACEND},
			{"^#invoke(?=\\s)", PREPROCESSOR_INVOKE},
			{"^#define(?=\\s)", PREPROCESSOR_DEFINE},
			{"^#undef(?=\\s)", PREPROCESSOR_UNDEF},
			{"^#ifdef(?=\\s)", PREPROCESSOR_IFDEF},
			{"^#ifndef(?=\\s)", PREPROCESSOR_IFNDEF},
			{"^#else(?=\\s)", PREPROCESSOR_ELSE},
			{"^#elsedef(?=\\s)", PREPROCESSOR_ELSEDEF},
			{"^#elsendef(?=\\s)", PREPROCESSOR_ELSENDEF},
			{"^#endif(?=\\s)", PREPROCESSOR_ENDIF},

            {"^BYTE(?=[\\s,\\)])", VARIABLE_TYPE_BYTE}, {"^DBYTE(?=[\\s,\\)])", VARIABLE_TYPE_DBYTE},
            {"^WORD(?=[\\s,\\)])", VARIABLE_TYPE_WORD}, {"^DWORD(?=[\\s,\\)])", VARIABLE_TYPE_DWORD},

            {"^\\.global(?=\\s)", ASSEMBLER_GLOBAL},
            {"^\\.extern(?=\\s)", ASSEMBLER_EXTERN},
            {"^\\.equ(?=\\s)", ASSEMBLER_EQU},
            {"^\\.org(?=\\s)", ASSEMBLER_ORG},
            {"^\\.scope(?=\\s)", ASSEMBLER_SCOPE},
            {"^\\.scend(?=\\s)", ASSEMBLER_SCEND},
            {"^\\.db(?=\\s)", ASSEMBLER_DB_LOW_ENDIAN},
            {"^\\.ddb(?=\\s)", ASSEMBLER_DDB_LOW_ENDIAN},
            {"^\\.ddb\\*(?=\\s)", ASSEMBLER_DDB_HIGH_ENDIAN},
            {"^\\.dw(?=\\s)", ASSEMBLER_DW_LOW_ENDIAN},
            {"^\\.dw\\*(?=\\s)", ASSEMBLER_DW_HIGH_ENDIAN},
            {"^\\.ddw(?=\\s)", ASSEMBLER_DDW_LOW_ENDIAN},
            {"^\\.ddw\\*(?=\\s)", ASSEMBLER_DDW_HIGH_ENDIAN},
            {"^\\.ascii(?=\\s)", ASSEMBLER_ASCII},
            {"^\\.asciz(?=\\s)", ASSEMBLER_ASCIZ},
            {"^\\.advance(?=\\s)", ASSEMBLER_ADVANCE},
            {"^\\.fill(?=\\s)", ASSEMBLER_FILL},
            {"^\\.space(?=\\s)", ASSEMBLER_SPACE},
            {"^\\.checkpc(?=\\s)", ASSEMBLER_CHECKPC},
            {"^\\.align(?=\\s)", ASSEMBLER_ALIGN},
            {"^\\.bss(?=\\s)", ASSEMBLER_BSS},
            {"^\\.bss\\*(?=\\s)", ASSEMBLER_BSS_ABSOLUTE},
            {"^\\.data(?=\\s)", ASSEMBLER_DATA},
            {"^\\.data\\*(?=\\s)", ASSEMBLER_DATA_ABSOLUTE},
            {"^\\.code(?=\\s)", ASSEMBLER_CODE},
            {"^\\.code\\*(?=\\s)", ASSEMBLER_CODE_ABSOLUTE},
            {"^\\.stop(?=\\s)", ASSEMBLER_STOP},

            {"^#", NUMBER_SIGN},
            {"^%[0-1]+", LITERAL_NUMBER_BINARY},
            {"^@[0-7]+", LITERAL_NUMBER_OCTAL}, 
            {"^[0-9]+", LITERAL_NUMBER_DECIMAL},
            {"^\\$[0-9a-fA-F]+", LITERAL_NUMBER_HEXADECIMAL},

			{"^\'.\'", LITERAL_CHAR}, {"^\".*\"", LITERAL_STRING},
			{"^[a-zA-Z_][a-zA-Z0-9_]*", SYMBOL},

            
			{"^\\+", OPERATOR_ADDITION}, {"^\\-", OPERATOR_SUBTRACTION}, 
            {"^\\*", OPERATOR_MULTIPLICATION}, {"^\\/", OPERATOR_DIVISION}, 
            {"^\\%", OPERATOR_MODULUS},
            {"^\\|\\|", OPERATOR_LOGICAL_OR}, {"^\\&\\&", OPERATOR_LOGICAL_AND},
			{"^\\<\\<", OPERATOR_BITWISE_LEFT_SHIFT}, {"^\\>\\>", OPERATOR_BITWISE_RIGHT_SHIFT},
			{"^\\^", OPERATOR_BITWISE_XOR}, {"^\\&", OPERATOR_BITWISE_AND}, 
            {"^\\|", OPERATOR_BITWISE_OR}, {"^~", OPERATOR_BITWISE_COMPLEMENT},
            {"^==", OPERATOR_LOGICAL_EQUAL}, {"^!=", OPERATOR_LOGICAL_NOT_EQUAL},
			{"^!", OPERATOR_LOGICAL_NOT}, 
            {"^\\<=", OPERATOR_LOGICAL_LESS_THAN_OR_EQUAL}, {"^\\>=", OPERATOR_LOGICAL_GREATER_THAN_OR_EQUAL},
            {"^\\<", OPERATOR_LOGICAL_LESS_THAN}, {"^\\>", OPERATOR_LOGICAL_GREATER_THAN},
		};

        /**
		 * Base source code character set
		 * 
		 * a-z A-Z 0-9 _ { } [ ] ( ) < > % : ; . , ? * + - / ^ & | ~ ! = " ' \ # @ $
		 */
		struct Token {
			
			Type type;
			std::string value;

			Token(Type type, std::string value) {
				this->type = type;
				this->value = value;
			}

			std::string toString() {
				if (type == WHITESPACE_SPACE || type == WHITESPACE_TAB || type == WHITESPACE_NEWLINE) {
					std::string toString = TYPE_TO_NAME_MAP.at(type) + ":";
					for (auto i = 0; i < value.length(); i++) {
						toString += " " + std::to_string(value[i]);
					}
					return toString;
				} else if (type == COMMENT_SINGLE_LINE || type == COMMENT_MULTI_LINE) {
                    return TYPE_TO_NAME_MAP.at(type);
                }

				return TYPE_TO_NAME_MAP.at(type) + ": " + value;
			}
		};


        static void tokenize(File* srcFile, std::vector<Token>&);
};


#endif