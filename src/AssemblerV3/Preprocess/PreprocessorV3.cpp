#include "PreprocessorV3.h"
#include <../src/util/Logger.h>
#include <../src/util/StringUtil.h>
#include <../src/util/VectorUtil.h>

#include <regex>
#include <fstream>
#include <filesystem>

/**
 * Constructs a preprocessor object with the given file.
 * 
 * @param process the build process object.
 * @param file the file to preprocess.
 * @param outputFilePath the path to the output file, default is the inputfile path with .bi extension.
 */
Preprocessor::Preprocessor(Process* process, File* inputFile, std::string outputFilePath) {
    m_process = process;
    m_inputFile = inputFile;

	// default output file path if not supplied in the constructor
	if (outputFilePath.empty()) {
        m_outputFile = new File(inputFile->getFileName(), PROCESSED_EXTENSION, inputFile->getFileDirectory(), true);
	} else {
        m_outputFile = new File(outputFilePath, true);
	}

	EXPECT_TRUE(m_process->isValidSourceFile(inputFile), ERROR, std::stringstream() << "Preprocessor::Preprocessor() - Invalid source file: " << inputFile->getExtension());

	m_state = State::UNPROCESSED;
	m_tokens = Tokenizer::tokenize(inputFile);
}

/**
 * Destructs a preprocessor object.
 */
Preprocessor::~Preprocessor() {
    delete m_outputFile;
}

/**
 * Preprocesses the file.
 */
void Preprocessor::preprocess() {
	log(DEBUG, std::stringstream() << "Preprocessor::preprocess() - Preprocessing file: " << m_inputFile->getFileName());

	EXPECT_TRUE(m_state == State::UNPROCESSED, ERROR, std::stringstream() << "Preprocessor::preprocess() - Preprocessor is not in the UNPROCESSED state");
	m_state = State::PROCESSING;

    // clearing intermediate output file
    std::ofstream ofs;
    ofs.open(m_outputFile->getFilePath(), std::ofstream::out | std::ofstream::trunc);
    ofs.close();

    // create writer for intermediate output file
    m_writer = new FileWriter(m_outputFile);

	// parses the tokens
	int currentIndentLevel = 0;
	int targetIndentLevel = 0;
	for (int i = 0; i < m_tokens.size(); ) {
		Tokenizer::Token& token = m_tokens[i];
        // log(DEBUG, std::stringstream() << "Preprocessor::preprocess() - Processing token " << i << ": " << token.toString());
		log(DEBUG, std::stringstream() << "Preprocessor::preprocess() - Indent Level: " << currentIndentLevel << " " << token.toString());
        
        // skip back to back newlines
        if (token.type == Tokenizer::WHITESPACE_NEWLINE && m_writer->lastByteWritten() == '\n') {
            i++;
            continue;
        }

		// update current indent level
		if (token.type == Tokenizer::WHITESPACE_TAB) {
			currentIndentLevel++;
		} else if (token.type == Tokenizer::WHITESPACE_NEWLINE) {
			currentIndentLevel = 0;
		}

		// update target indent level
		if (token.type == Tokenizer::ASSEMBLER_SCEND) {
			targetIndentLevel--;
		}

		// format the output with improved indents
		if (currentIndentLevel < targetIndentLevel && token.type == Tokenizer::WHITESPACE_SPACE) {
			// don't output whitespaces if a tab is expected
			continue;
		} else if (currentIndentLevel < targetIndentLevel 
				&& token.type != Tokenizer::WHITESPACE_TAB && token.type != Tokenizer::WHITESPACE_NEWLINE) {
			// append tabs
			while (currentIndentLevel < targetIndentLevel) {
				m_writer->writeString("\t");
				currentIndentLevel++;
			}
		}

		// if token is valid preprocessor, call the preprocessor function
		if (preprocessors.find(token.type) != preprocessors.end()) {
			(this->*preprocessors[token.type])(i);
		} else {
            // check if this is a defined symbol
            if (token.type == Tokenizer::SYMBOL && m_symbols.find(token.value) != m_symbols.end()) {
                // replace symbol with value
                consume(i);
                m_tokens.insert(m_tokens.begin() + i, m_symbols[token.value].begin(), m_symbols[token.value].end());
            } else {
                m_writer->writeString(consume(i).value);
            }
		}

		// update target indent level
		if (token.type == Tokenizer::ASSEMBLER_SCOPE) {
			targetIndentLevel++;
		}
	}

	m_state = State::PROCESSED_SUCCESS;
	m_writer->close();

	log(DEBUG, std::stringstream() << "Preprocessor::preprocess() - Preprocessed file: " << m_inputFile->getFileName());

    // log macros
    for (std::pair<std::string, Macro*> macroPair : m_macros) {
        log(DEBUG, std::stringstream() << "Preprocessor::preprocess() - Macro: " << macroPair.second->toString());
    }
}


/**
 * Returns the macros that match the given macro name and arguments list.
 * 
 * @param macroName the name of the macro.
 * @param arguments the arguments passed to the macro.
 * 
 * TODO: possibly in the future should consider filtering for macros that have the same order of argument types.
 * 		This would require us knowing the types of symbols and expressions in the preprocessor state 
 * 		which is not ideal
 * 
 * @return the macros with the given name and number of arguments.
 */
std::vector<Preprocessor::Macro*> Preprocessor::macrosWithHeader(std::string macroName, std::vector<std::vector<Tokenizer::Token>> arguments) {
	std::vector<Macro*> possibleMacros;
	for (std::pair<std::string, Macro*> macroPair : m_macros) {
		if (macroPair.second->name == macroName && macroPair.second->arguments.size() == arguments.size()) {
			possibleMacros.push_back(macroPair.second);
		}
	}
	return possibleMacros;
}


/**
 * Skips tokens that match the given regex.
 * 
 * @param regex matches tokens to skip.
 * @param tokenI the index of the current token.
 */
void Preprocessor::skipTokens(int& tokenI, const std::string& regex) {
	while (tokenI < m_tokens.size() && std::regex_match(m_tokens[tokenI].value, std::regex(regex))) {
		tokenI++;
	}
}

/**
 * Skips tokens that match the given types.
 * 
 * @param tokenI the index of the current token.
 * @param tokenTypes the types to match.
 */
void Preprocessor::skipTokens(int& tokenI, const std::set<Tokenizer::Type>& tokenTypes) {
    while (tokenI < m_tokens.size() && tokenTypes.find(m_tokens[tokenI].type) != tokenTypes.end()) {
        tokenI++;
    }
}

/**
 * Expects the current token to exist.
 * 
 * @param tokenI the index of the expected token.
 * @param errorMsg the error message to throw if the token does not exist.
 */
bool Preprocessor::expectToken(int& tokenI, const std::string& errorMsg) {
	EXPECT_TRUE(tokenI < m_tokens.size(), ERROR, std::stringstream(errorMsg));
    return true;
}

bool Preprocessor::expectToken(int& tokenI, const std::set<Tokenizer::Type>& expectedTypes, const std::string& errorMsg) {
	EXPECT_TRUE(tokenI < m_tokens.size(), ERROR, std::stringstream(errorMsg));
	EXPECT_TRUE(expectedTypes.find(m_tokens[tokenI].type) != expectedTypes.end(), ERROR, std::stringstream(errorMsg));
    return true;
}

/**
 * Returns whether the current token matches the given types.
 * 
 * @param tokenI the index of the current token.
 * @param tokenTypes the types to match.
 * 
 * @return true if the current token matches the given types.
 */
bool Preprocessor::isToken(int& tokenI, const std::set<Tokenizer::Type>& tokenTypes, const std::string& errorMsg) {
    expectToken(tokenI, errorMsg);
    return tokenTypes.find(m_tokens[tokenI].type) != tokenTypes.end();
}

/**
 * Consumes the current token.
 * 
 * @param tokenI the index of the current token.
 * @param errorMsg the error message to throw if the token does not exist.
 * 
 * @returns the value of the consumed token.
 */
Tokenizer::Token& Preprocessor::consume(int& tokenI, const std::string& errorMsg) {
    expectToken(tokenI, errorMsg);
    return m_tokens[tokenI++];
}

/**
 * Consumes the current token and checks it matches the given types.
 * 
 * @param tokenI the index of the current token.
 * @param expectedTypes the expected types of the token.
 * @param errorMsg the error message to throw if the token does not have the expected type.
 * 
 * @returns the value of the consumed token.
 */
Tokenizer::Token& Preprocessor::consume(int& tokenI, const std::set<Tokenizer::Type>& expectedTypes, const std::string& errorMsg) {
    expectToken(tokenI, errorMsg);
	EXPECT_TRUE(expectedTypes.find(m_tokens[tokenI].type) != expectedTypes.end(), ERROR, std::stringstream() << errorMsg << " - Unexpected end of file.");
    return m_tokens[tokenI++];
}


/**
 * Inserts the file contents into the current file.
 * 
 * USAGE: #include "filepath"|<filepath>
 * 
 * "filepath": looks for files located in the current directory.
 * <filepath>: prioritizes files located in the include directory, if not found, looks in the
 * current directory.
 * 
 * @param tokenI the index of the include token.
 */
void Preprocessor::_include(int& tokenI) {
	consume(tokenI); // '#include'
	skipTokens(tokenI, "[ \t]");

	// the path to the included file
	std::string fullPathFromWorkingDirectory;

    if (isToken(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_include() - Missing include filename.")) {
        // local include
		std::string localFilePath = consume(tokenI).value;
		localFilePath = localFilePath.substr(1, localFilePath.length() - 2);
        fullPathFromWorkingDirectory = m_inputFile->getFileDirectory() + File::SEPARATOR + localFilePath;
    } else {
        // expect <"...">
        consume(tokenI, {Tokenizer::OPERATOR_LOGICAL_LESS_THAN}, "Preprocessor::_include() - Missing '<'.");
        std::string systemFilePath = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_include() - Expected string literal.").value;
        consume(tokenI, {Tokenizer::OPERATOR_LOGICAL_GREATER_THAN}, "Preprocessor::_include() - Missing '>'.");

        // check if file exists in system include directories
		bool foundSystemFile = false;
        for (Directory* directory : m_process->getSystemDirectories()) {
            if (directory->subfileExists(systemFilePath)) {
				if (foundSystemFile) {
					// already found file
					log(ERROR, std::stringstream() << "Preprocessor::_include() - Multiple matching files found in system include directories: " << systemFilePath);
				}

                fullPathFromWorkingDirectory = directory->getDirectoryPath() + File::SEPARATOR + systemFilePath;
				foundSystemFile = true;
			}
        }

		if (!foundSystemFile) {
			log(ERROR, std::stringstream() << "Preprocessor::_include() - File not found in system include directories: " << systemFilePath);
		}
	}

	// process included file
	File* includeFile = new File(fullPathFromWorkingDirectory);
	EXPECT_TRUE(includeFile->exists(), ERROR, std::stringstream() << "Preprocessor::_include() - Include file does not exist: " << fullPathFromWorkingDirectory);

	// instead of writing all the contents to the output file, simply tokenize the file and insert into the current token list
	Preprocessor includedPreprocessor(m_process, includeFile, m_outputFile->getFilePath());
    delete includeFile;
	
	// yoink the tokens from the included file and insert
	m_tokens.insert(m_tokens.begin() + tokenI, includedPreprocessor.m_tokens.begin(), includedPreprocessor.m_tokens.end());
}

/**
 * Defines a macro symbol with n arguments and optionally a return type.
 * 
 * USAGE: #macro [symbol]([arg1 ?: TYPE, arg2 ?: TYPE,..., argn ?: TYPE]) ?: TYPE
 * 
 * If a return type is specified and the macro definition does not return a value an error is thrown.
 * There cannot be a macro definition within this macro definition.
 * Note that the macro symbol is separate from label symbols and will not be present after preprocessing.
 * 
 * @param tokenI The index of the macro token.
 */
void Preprocessor::_macro(int& tokenI) {
	consume(tokenI); // '#macro'
	skipTokens(tokenI, "[ \t]");

	// parse macro name
	std::string macroName = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_macro() - Expected macro name.").value;
    Macro* macro = new Macro(macroName);

    // start of invoked arguments
	skipTokens(tokenI, "[ \t\n]");
	consume(tokenI, {Tokenizer::OPEN_PARANTHESIS}, "Preprocessor::_macro() - Expected '('.");

    // parse arguments
    while (!isToken(tokenI, {Tokenizer::CLOSE_PARANTHESIS}, "Preprocessor::_macro() - Expected macro header.")) {
        skipTokens(tokenI, "[ \t\n]");
        std::string argName = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_macro() - Expected argument name.").value;

        // parse argument type if there is one
        skipTokens(tokenI, "[ \t\n]");
        if (isToken(tokenI, {Tokenizer::COLON})) {
            consume(tokenI);
            skipTokens(tokenI, "[ \t\n]");
            macro->arguments.push_back(Argument(argName, consume(tokenI, Tokenizer::VARIABLE_TYPES, "Preprocessor::_macro() - Expected argument type").type));
        } else {
            macro->arguments.push_back(Argument(argName));
        }

        // parse comma or expect closing parenthesis
        skipTokens(tokenI, "[ \t\n]");
        if (isToken(tokenI, {Tokenizer::COMMA})) {
            consume(tokenI);
        }
    }

    // consume the closing parenthesis
    consume(tokenI, {Tokenizer::CLOSE_PARANTHESIS}, "Preprocessor::_macro() - Expected ')'.");
    skipTokens(tokenI, "[ \t\n]");

    // parse return type if there is one
    if (isToken(tokenI, {Tokenizer::COLON})) {
        consume(tokenI);
        skipTokens(tokenI, "[ \t\n]");
        macro->returnType = consume(tokenI, Tokenizer::VARIABLE_TYPES, "Preprocessor::_macro() - Expected return type.").type;
    }

    // parse macro definition
    skipTokens(tokenI, "[ \t\n]");
    while (!isToken(tokenI, {Tokenizer::PREPROCESSOR_MACEND}, "Preprocessor::_macro() - Expected macro definition." )) {
        macro->definition.push_back(consume(tokenI));
    }
    consume(tokenI, {Tokenizer::PREPROCESSOR_MACEND}, "Preprocessor::_macro() - Expected '#macend'.");

    // check if macro declaration is unique
	EXPECT_TRUE(m_macros.find(macro->header()) == m_macros.end(), ERROR, std::stringstream() << "Preprocessor::_macro() - Macro already defined: " << macro->header());

    // add macro to list of macros
    m_macros.insert(std::pair<std::string,Macro*>(macro->header(), macro));
}

/**
 * Stops processing the macro and returns the value of the expression.
 * 
 * USAGE: #macret [?expression]
 * 
 * If the macro does not have a return type the macret must return nothing.
 * If the macro has a return type the macret must return a value of that type
 * 
 * @param tokenI The index of the macro return token
 */
void Preprocessor::_macret(int& tokenI) {
	consume(tokenI); // '#macret'
	skipTokens(tokenI, "[ \t]");

	std::vector<Tokenizer::Token> return_value;
	if (m_macroStack.empty()) {
		log(ERROR, std::stringstream() << "Preprocessor::_macret() - Unexpected macret token.");
	}

	// macro contains a return value
	bool doesMacroReturn = m_macroStack.top().second->returnType != Tokenizer::UNKNOWN;
	if (doesMacroReturn) {
		while (!isToken(tokenI, {Tokenizer::WHITESPACE_NEWLINE})) {
			return_value.push_back(consume(tokenI));
		}
	}

	// skip all the tokens after this till the end of the current macro's definition
	// we can achieve this by counting the number of scope levels, incrementing if we reach a .scope token and decrementing
	// if we reach a .scend token. If we reach 0, we know we have reached the end of the macro definition.
	
	int currentRelativeScopeLevel = 0;
	while (tokenI < m_tokens.size()) {
		if (isToken(tokenI, {Tokenizer::ASSEMBLER_SCOPE})) {
			currentRelativeScopeLevel++;
		} else if (isToken(tokenI, {Tokenizer::ASSEMBLER_SCEND})) {
			currentRelativeScopeLevel--;
		}
		consume(tokenI);

		if (currentRelativeScopeLevel == 0) {
			break;
		}
	}

	if (currentRelativeScopeLevel != 0) {
		log(ERROR, std::stringstream() << "Preprocessor::_macret() - Unclosed scope.");
	}

	// add'.equ current_macro_output_symbol expression' to tokens
	if (doesMacroReturn) {
		std::vector<Tokenizer::Token> set_return_statement;
		vector_util::append(set_return_statement, Tokenizer::tokenize(string_util::format(".equ {} ", m_macroStack.top().first)));
		vector_util::append(set_return_statement, return_value);
		vector_util::append(set_return_statement, Tokenizer::tokenize(string_util::format(" : {}\n", Tokenizer::VARIABLE_TYPE_TO_NAME_MAP.at(m_macroStack.top().second->returnType))));
		m_tokens.insert(m_tokens.begin() + tokenI, set_return_statement.begin(), set_return_statement.end());
	}

	// pop the macro from the stack
	m_macroStack.pop();
}

/**
 * Closes a macro definition.
 * 
 * USAGE: #macend
 * 
 * If a macro is not closed an error is thrown.
 * 
 * @param tokenI The index of the macro end token
 */
void Preprocessor::_macend(int& tokenI) {
    // should never reach this. This should be consumed by the _macro function.
    log(ERROR, std::stringstream() << "Preprocessor::_macend() - Unexpected macro end token.");
}

/**
 * Invokes the macro with the given arguments.
 * 
 * USAGE: #invoke [symbol]([arg1, arg2,..., argn]) [?symbol]
 * 
 * If provided an output symbol, the symbol will be associated with the return value of the macro.
 * If the macro does not return a value but an output symbol is provided, an error is thrown.
 * 
 * @param tokenI The index of the invoke token
 */
void Preprocessor::_invoke(int& tokenI) {
	consume(tokenI);
	skipTokens(tokenI, "[ \t]");

	// parse macro name
	std::string macroName = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_invoke() - Expected macro name.").value;
	
	// parse arguments
	skipTokens(tokenI, "[ \t\n]");
	consume(tokenI, {Tokenizer::OPEN_PARANTHESIS}, "Preprocessor::_invoke() - Expected '('.");
	std::vector<std::vector<Tokenizer::Token>> arguments;
	while (!isToken(tokenI, {Tokenizer::CLOSE_PARANTHESIS}, "Preprocessor::_invoke() - Expected ')'.")) {
		skipTokens(tokenI, "[ \t\n]");
		
		std::vector<Tokenizer::Token> argumentValues;
		while (!isToken(tokenI, {Tokenizer::COMMA, Tokenizer::CLOSE_PARANTHESIS, Tokenizer::WHITESPACE_NEWLINE}, "Preprocessor::_invoke() - Expected ')'.")) {
			argumentValues.push_back(consume(tokenI));
		}
		arguments.push_back(argumentValues);

		if (isToken(tokenI, {Tokenizer::COMMA})) {
			consume(tokenI);
		}
	}
	consume(tokenI, {Tokenizer::CLOSE_PARANTHESIS}, "Preprocessor::_invoke() - Expected ')'.");
	skipTokens(tokenI, "[ \t\n]");

	// parse the output symbol if there is one
	bool hasOutput = isToken(tokenI, {Tokenizer::SYMBOL});
	std::string outputSymbol = "";
	if (hasOutput) {
		outputSymbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_invoke() - Expected output symbol.").value;
	}

	// check if macro exists
	std::vector<Macro*> possibleMacros = macrosWithHeader(macroName, arguments);
	if (possibleMacros.size() == 0) {
		log(ERROR, std::stringstream() << "Preprocessor::_invoke() - Macro does not exist: " << macroName);
	} else if (possibleMacros.size() > 1) {
		log(ERROR, std::stringstream() << "Preprocessor::_invoke() - Multiple macros with the same name and number of arguments: " << macroName);
	}
	Macro* macro = possibleMacros[0];

	// replace the '_invoke symbol(arg1, arg2,..., argn) ?symbol' with the macro definition
	std::vector<Tokenizer::Token> expanded_macro_invoke;
	
	// check if the macro returns something, if so add a equate statement to store the output
	if (hasOutput) {
		vector_util::append(expanded_macro_invoke, Tokenizer::tokenize(string_util::format(".equ {} 0 : {}\n", outputSymbol, Tokenizer::VARIABLE_TYPE_TO_NAME_MAP.at(macro->returnType))));
	}

	// append a new '.scope' symbol to the tokens list
	expanded_macro_invoke.push_back(Tokenizer::Token(Tokenizer::ASSEMBLER_SCOPE, ".scope"));
	expanded_macro_invoke.push_back(Tokenizer::Token(Tokenizer::WHITESPACE_NEWLINE, "\n"));

	// then for each argument, add an '.equ argname argval' statement
	for (int i = 0; i < arguments.size(); i++) {
		vector_util::append(expanded_macro_invoke, Tokenizer::tokenize(string_util::format(".equ {} ", macro->arguments[i].name)));
		vector_util::append(expanded_macro_invoke, arguments[i]);
		vector_util::append(expanded_macro_invoke, Tokenizer::tokenize(string_util::format(" : {}\n", Tokenizer::VARIABLE_TYPE_TO_NAME_MAP.at(macro->arguments[i].type))));
	}

	// then append the macro definition
	expanded_macro_invoke.insert(expanded_macro_invoke.end(), macro->definition.begin(), macro->definition.end());

	// finally end with a '.scend' symbol
	expanded_macro_invoke.push_back(Tokenizer::Token(Tokenizer::WHITESPACE_NEWLINE, "\n"));
	expanded_macro_invoke.push_back(Tokenizer::Token(Tokenizer::ASSEMBLER_SCEND, ".scend"));

	// push the macro and output symbol if any onto the macro stack
	m_macroStack.push(std::pair<std::string, Macro*>(outputSymbol, macro));

	// print out expanded macro
	std::stringstream ss;
	for (Tokenizer::Token& token : expanded_macro_invoke) {
		ss << token.value;
	}
	log(DEBUG, std::stringstream() << "Preprocessor::_invoke() - Expanded macro: " << ss.str());

	// insert into the tokens list
	m_tokens.insert(m_tokens.begin() + tokenI, expanded_macro_invoke.begin(), expanded_macro_invoke.end());
}

/**
 * Associates the symbol with a value
 * 
 * USAGE: #define [symbol] [?value]
 * 
 * Replaces all instances of symbol with the value.
 * If value is not specified, the default is empty.
 * 
 * @param tokenI The index of the define token.
 */
void Preprocessor::_define(int& tokenI) {
    consume(tokenI); // '#define'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_define() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // value
    std::vector<Tokenizer::Token> tokens;
    if (!isToken(tokenI, {Tokenizer::WHITESPACE_NEWLINE})) {
        while (!isToken(tokenI, {Tokenizer::WHITESPACE_NEWLINE})) {
            tokens.push_back(consume(tokenI));
        }
    }
    
    // add to symbols mapping
    m_symbols.insert(std::pair<std::string, std::vector<Tokenizer::Token>>(symbol, tokens));
}


void Preprocessor::conditionalBlock(int& tokenI, bool conditionMet) {
    int relativeScopeLevel = 0;
    int currentTokenI = tokenI;
    int nextBlockTokenI = -1;
    int endIfTokenI = -1;
    while (currentTokenI < m_tokens.size()) {
        std::cout << relativeScopeLevel << " " << m_tokens[currentTokenI].value << std::endl;

        if (relativeScopeLevel == 0 && isToken(currentTokenI, {Tokenizer::PREPROCESSOR_ENDIF})) {
            endIfTokenI = currentTokenI;
            break;
        } else if (relativeScopeLevel == 0 && isToken(currentTokenI, {Tokenizer::PREPROCESSOR_ELSE, Tokenizer::PREPROCESSOR_ELSEDEF, Tokenizer::PREPROCESSOR_ELSENDEF})) {
            if (nextBlockTokenI == -1) {
                nextBlockTokenI = currentTokenI;
            }

            // start of next conditional block that should be checked if the current conditional block was
            // not entered
            if (!conditionMet) {
                break;
            }
        }

        if (isToken(currentTokenI, {Tokenizer::PREPROCESSOR_IFDEF, Tokenizer::PREPROCESSOR_IFNDEF})) {
            relativeScopeLevel++;
        } else if (isToken(currentTokenI, {Tokenizer::PREPROCESSOR_ENDIF})) {
            relativeScopeLevel--;
        }
        currentTokenI++;
    }

    if ((conditionMet && endIfTokenI == -1) || (!conditionMet && nextBlockTokenI == -1)) {
        log(DEBUG, std::stringstream() << "condition=" << conditionMet << " | endIf=" << endIfTokenI << " | nextBlockTokenI=" << nextBlockTokenI);
        log(ERROR, std::stringstream() << "Preprocessor::_ifdef() - Unclosed ifdef block." );
    }

    if (conditionMet) {
        log(DEBUG, std::stringstream() << " | endIf=" << endIfTokenI << " | nextBlockTokenI=" << nextBlockTokenI);
        
        if (nextBlockTokenI != -1) {
            // remove all tokens from the next block to the endif
            m_tokens.erase(m_tokens.begin() + nextBlockTokenI, m_tokens.begin() + endIfTokenI);
        } else { // assert, endIfTokenI != -1
            // don't need to do anything, because there are no linked conditional blocks after this
        }
        
        m_tokens.insert(m_tokens.begin() + tokenI, Tokenizer::Token(Tokenizer::COMMENT_SINGLE_LINE, "; conditional"));
    } else {
        // move token index to the start of the next conditional block (or endif)
        tokenI = nextBlockTokenI;
    }
}

/**
 * Begins a top conditional block. 
 * Determines whether to include the following text block if the symbol is defined.
 * 
 * USAGE: #ifdef [symbol]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * 
 * @param tokenI The index of the ifdef token.
 */
void Preprocessor::_ifdef(int& tokenI) {
    consume(tokenI); // '#ifdef'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifdef() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    bool isDefined = m_symbols.find(symbol) != m_symbols.end();
    conditionalBlock(tokenI, isDefined);
}

/**
 * Begins a top conditional block. 
 * Determines whether to include the following text block if the symbol is not defined.
 * 
 * USAGE: #ifndef [symbol]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * 
 * @param tokenI The index of the ifndef token.
 */
void Preprocessor::_ifndef(int& tokenI) {
    consume(tokenI); // '#ifndef'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifndef() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    bool isNotDefined = m_symbols.find(symbol) == m_symbols.end();
    conditionalBlock(tokenI, isNotDefined);
}

/**
 * Begins a top conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically equal to a value.
 * 
 * USAGE: #ifequ [symbol] [value]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the ifequ token.
 */
void Preprocessor::_ifequ(int& tokenI) {
    consume(tokenI); // '#ifequ'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifequ() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_ifequ() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isEqual = string_util::trimString(value, 1, 1) == symbolValue;
    conditionalBlock(tokenI, isEqual);
}

/**
 * Begins a top conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically not equal to a value.
 * 
 * USAGE: #ifnequ [symbol] [value]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the ifnequ token.
 */
void Preprocessor::_ifnequ(int& tokenI) {
    consume(tokenI); // '#ifnequ'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifnequ() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_ifnequ() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isNotEqual = string_util::trimString(value, 1, 1) != symbolValue;
    conditionalBlock(tokenI, isNotEqual);
}

/**
 * Begins a top conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically less than a value.
 * 
 * USAGE: #ifless [symbol] [value]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the ifless token.
 */
void Preprocessor::_ifless(int& tokenI) {
    consume(tokenI); // '#ifless'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifless() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_ifless() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isLess = symbolValue < string_util::trimString(value, 1, 1);
    conditionalBlock(tokenI, isLess);
}

/**
 * Begins a top conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically greater than a value.
 * 
 * USAGE: #ifmore [symbol] [value]
 * 
 * The conditional block must be closed by a lower conditional block or an #endif.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the ifmore token.
 */
void Preprocessor::_ifmore(int& tokenI) {
    consume(tokenI); // '#ifmore'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_ifmore() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_ifmore() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isMore = symbolValue > string_util::trimString(value, 1, 1);
    conditionalBlock(tokenI, isMore);
}

/**
 * Closure of a top or lower conditional block, only includes the following text if all previous 
 * conditional blocks were not included.
 * 
 * USAGE: #else
 * 
 * Must be preceded by a top or inner conditional block.
 * Must not be proceeded by an inner conditional block or closure.
 * 
 * @param tokenI The index of the else token.
 */
void Preprocessor::_else(int& tokenI) {
    consume(tokenI); // '#else'
    skipTokens(tokenI, "[ \t]");
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol is defined and all previous
 * top or inner conditional blocks were not included.
 * 
 * USAGE: #elsedef [symbol]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * 
 * @param tokenI The index of the elsedef token.
 */
void Preprocessor::_elsedef(int& tokenI) {
    consume(tokenI); // '#elsedef'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elsedef() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    bool isDefined = m_symbols.find(symbol) != m_symbols.end();
    conditionalBlock(tokenI, isDefined);
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol is not defined and all previous
 * top or inner conditional blocks were not included.
 * 
 * USAGE: #elsendef [symbol]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * 
 * @param tokenI The index of the elsendef token.
 */
void Preprocessor::_elsendef(int& tokenI) {
    consume(tokenI); // '#elsendef'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elsendef() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    bool isNotDefined = m_symbols.find(symbol) == m_symbols.end();
    conditionalBlock(tokenI, isNotDefined);
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically equal to the value and all previous top or inner conditional blocks 
 * were not included.
 * 
 * USAGE: #elseequ [symbol] [value]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the elseequ token.
 */
void Preprocessor::_elseequ(int& tokenI) {
    consume(tokenI); // '#elseequ'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elseequ() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_elseequ() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isEqual = string_util::trimString(value, 1, 1) == symbolValue;
    conditionalBlock(tokenI, isEqual);
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically not equal to the value and all previous top or inner conditional blocks 
 * were not included.
 * 
 * USAGE: #elsenequ [symbol] [value]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the elsenequ token.
 */
void Preprocessor::_elsenequ(int& tokenI) {
    consume(tokenI); // '#elsenequ'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elsenequ() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_elsenequ() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isNotEqual = string_util::trimString(value, 1, 1) != symbolValue;
    conditionalBlock(tokenI, isNotEqual);
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically less than the value and all previous top or inner conditional blocks 
 * were not included.
 * 
 * USAGE: #elseless [symbol] [value]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the elseless token.
 */
void Preprocessor::_elseless(int& tokenI) {
    consume(tokenI); // '#elseless'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elseless() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_elseless() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isLess = symbolValue < string_util::trimString(value, 1, 1);
    conditionalBlock(tokenI, isLess);
}

/**
 * Begins an inner conditional block.
 * Determines whether to include the following text block if the symbol's value is 
 * lexicographically greater than the value and all previous top or inner conditional blocks 
 * were not included.
 * 
 * USAGE: #elsemore [symbol] [value]
 * 
 * Must be preceded by a top or inner conditional block.
 * Must be proceeded by an inner conditional block or closure.
 * The value must be a string literal.
 * 
 * @param tokenI The index of the elsemore token.
 */
void Preprocessor::_elsemore(int& tokenI) {
    consume(tokenI); // '#elsemore'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_elsemore() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // extract symbol's string value
    std::string symbolValue;
    if (m_symbols.find(symbol) != m_symbols.end()) {
        for (Tokenizer::Token& token : m_symbols[symbol]) {
            symbolValue += token.value;
        }
    }

    // value
    std::string value = consume(tokenI, {Tokenizer::LITERAL_STRING}, "Preprocessor::_elsemore() - Expected value.").value;
    skipTokens(tokenI, "[ \t]");

    bool isMore = symbolValue > string_util::trimString(value, 1, 1);
    conditionalBlock(tokenI, isMore);
}

/**
 * Closes a #ifdef, #ifndef, #else, #elsedef, or #elsendef.
 * 
 * USAGE: #endif
 * 
 * Must be preceded by a #ifdef, #ifndef, #else, #elsedef, or #elsendef.
 * 
 * @param tokenI The index of the endif token.
 */
void Preprocessor::_endif(int& tokenI) {
    consume(tokenI); // '#endif'
    skipTokens(tokenI, "[ \t]");
}

/**
 * Undefines a symbol defined by #define.
 * 
 * USAGE: #undefine [symbol]
 * 
 * This will still work if the symbol was never defined previously.
 * 
 * @param tokenI The index of the undefine token.
 */
void Preprocessor::_undefine(int& tokenI) {
    consume(tokenI); // '#define'
    skipTokens(tokenI, "[ \t]");

    // symbol
    std::string symbol = consume(tokenI, {Tokenizer::SYMBOL}, "Preprocessor::_define() - Expected symbol.").value;
    skipTokens(tokenI, "[ \t]");

    // remove from symbols mapping
    m_symbols.erase(symbol);
}

/**
 * Returns the state of the preprocessor.
 * 
 * @return the state of the preprocessor.
 */
Preprocessor::State Preprocessor::getState() {
	return m_state;
}