#ifndef ccvm_assembler_assembler
#define ccvm_assembler_assembler

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

typedef struct cca_file_content {
	unsigned int fileSize;
	char* content;
} cca_file_content;

typedef struct cca_token {
	char type;
	union value {
		unsigned int numeric;
		char* string;
	} value;
} cca_token;

char* cca_token_type_str(char type) {
	switch (type) {
		case 0: return "identifier";
		case 1: return "number";
		case 2: return "divider";
		case 3: return "opcode";
		case 4: return "register";
		case 5: return "label";
		case 6: return "end";
		default: return "unknown";
	}
}

void cca_token_print(cca_token tok) {
	if (tok.type == 1) {
		printf("Token[type: %s, value: %d]\n", cca_token_type_str(tok.type), tok.value.numeric);
	} else {
		printf("Token[type: %s, value: %s]\n", cca_token_type_str(tok.type), tok.value.string);
	}
}

cca_file_content ccvm_program_load(char *filename) {
    cca_file_content content;

    // open file
    FILE *fp = fopen(filename, "r");

    // error detection
    if (fp == NULL) {
        printf("[ERROR] could not open file: %s\n", filename);
        exit(1);
    }

    // get buffer size
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // buffer allocation
    char *buffer = (char *) malloc(size);

    if(fread(buffer, sizeof *buffer, size, fp) != size) {
        printf("[ERROR] while reading file: %s\n", filename);
        exit(1);
    }

    content.fileSize = size;
    content.content = buffer;

    return content;
}

// recognizer functions
char cca_is_identifier(char character) {
	return (isalpha(character) || character == '_');
}

char cca_is_number(char character) {
	return isnumber(character);
}

char cca_is_ignorable(char character) {
	return (character == ' ' || character == '\n' || character == '\t');
}

char cca_is_divider(char character) {
	return character == ',';
}

char cca_is_comment(char character) {
	return character == ';';
}

// parse functions
cca_token cca_parse_identifier(char* code, unsigned int* readingPos) {
	cca_token tok;
	tok.type = 0;

	unsigned int stringCap = 100;
	unsigned int stringLen = 0;
	char* string = malloc(stringCap * sizeof(char));

	while(cca_is_identifier(code[*readingPos])) {
		++stringLen;

		if (stringLen >= stringCap) {
			stringCap *= 2;
			string = realloc(string, stringCap * sizeof(char));
		}

		string[stringLen-1] = code[*readingPos];

		++*readingPos;
	}

	--*readingPos;
	string = realloc(string, stringLen+1 * sizeof(char));

	string[stringLen] = '\0';

	tok.value.string = string;

	return tok;
}

cca_token cca_parse_number(char* code, unsigned int* readingPos) {
	unsigned int n = 0;

	cca_token tok;
	tok.type = 1;

	while(cca_is_number(code[*readingPos])) {
		n = n*10 + code[*readingPos] - 48;
		++*readingPos;
	}

	tok.value.numeric = n;

	return tok;
}

cca_token* cca_assembler_lex(cca_file_content content) {
	// file data
	unsigned int size = content.fileSize;
	char* assembly = content.content;

	// lex data
	unsigned int readingPos = 0;
	unsigned int tokCapacity = 100;
	unsigned int tokCount = 0;
	cca_token* tokens = malloc(tokCapacity * sizeof(cca_token));
	
	// first lexing loop
	while(readingPos < size) {
		char current = assembly[readingPos];
		
		if (cca_is_ignorable(current)) {

		} else if (cca_is_divider(current)) {
			cca_token newTok;
			newTok.type = 2;
			newTok.value.string = ",";
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
		} else if (cca_is_identifier(current)){
			cca_token newTok = cca_parse_identifier(assembly, &readingPos);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
		} else if (cca_is_number(current)) {
			cca_token newTok = cca_parse_number(assembly, &readingPos);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
		} else if (cca_is_comment(current)) {
			puts("[INFO] found comment");
		}

		else {
			printf("[ERROR] unknown syntax: %c\n", current);
		}
		++readingPos;
	}

	// shrink tokens array
	tokens = realloc(tokens, (tokCount+1) * sizeof(cca_token));
	cca_token end;
	end.type = 6;
	tokens[tokCount] = end;

	return tokens;
}

char strContainedIn(char* string, char** array) {
	unsigned int arrayLength = sizeof(array) / sizeof(char*);

	for (int i = 0; i <= arrayLength; i++) {
		if (strcmp(string, array[i]) == 0) {
			return 1;
		}
	}

	return 0;
}

void cca_assembler_recognize(cca_token* tokens) {
	char* mnemonics[] = {"mov", "stp", "psh"};
	unsigned int i = 0;

	while(tokens[i].type != 6) {
		if (tokens[i].type == 0) {
			if (strContainedIn(tokens[i].value.string, (char**)mnemonics)) {
				tokens[i].type = 3;
			}

			if (strcmp(tokens[i].value.string, "a") == 0 || strcmp(tokens[i].value.string, "b") == 0 || strcmp(tokens[i].value.string, "c") == 0 || strcmp(tokens[i].value.string, "d") == 0) {
				tokens[i].type = 4;
			}
		}

		++i;
	}
	return;
}

typedef struct cca_bytecode {
	char* bytecode;
	unsigned int bytecodeCapacity;
	unsigned int bytecodeLength;
} cca_bytecode;

void cca_bytecode_add_byte(cca_bytecode* bytecode, char byte) {
	bytecode->bytecodeLength += 1;
	
	if (bytecode->bytecodeLength >= bytecode->bytecodeCapacity) {
		bytecode->bytecodeCapacity *= 2;
		bytecode->bytecode = realloc(bytecode->bytecode, bytecode->bytecodeLength);
	}

	bytecode->bytecode[bytecode->bytecodeLength - 1] = byte;
}

void cca_bytecode_add_reg(cca_bytecode* bytecode, char* reg) {
	switch(reg[0]) {
		case 'a': {cca_bytecode_add_byte(bytecode, 0x00);break;}
		case 'b': {cca_bytecode_add_byte(bytecode, 0x01);break;}
		case 'c': {cca_bytecode_add_byte(bytecode, 0x02);break;}
		case 'd': {cca_bytecode_add_byte(bytecode, 0x03);break;}
	}
}

void cca_bytecode_add_uint(cca_bytecode* bytecode, unsigned int n) {
	bytecode->bytecodeLength += 4;
	
	if (bytecode->bytecodeLength >= bytecode->bytecodeCapacity) {
		bytecode->bytecodeCapacity *= 2;
		bytecode->bytecode = realloc(bytecode->bytecode, bytecode->bytecodeLength);
	}

	bytecode->bytecode[bytecode->bytecodeLength - 4] = (n >> 24) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 3] = (n >> 16) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 2] = (n >> 8) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 1] = n & 0xff;
}

void cca_assembler_bytegeneration(cca_token* tokens) {
	FILE* fp = fopen("test.ccb", "wb+");
	cca_bytecode bytecode;
	bytecode.bytecodeCapacity = 100;
	bytecode.bytecodeLength = 0;
	bytecode.bytecode = malloc(bytecode.bytecodeCapacity);

	cca_bytecode_add_uint(&bytecode, 0x1d1d1d1d);

	unsigned int i = 0;
	while(tokens[i].type != 6) {
		if (strcmp(tokens[i].value.string, "stp") == 0) {
			cca_bytecode_add_byte(&bytecode, 0x00);
			i += 1;
		} else if (strcmp(tokens[i].value.string, "psh") == 0) {
			if (tokens[i + 1].type == 1) {
				cca_bytecode_add_byte(&bytecode, 0x01);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
			} else if (tokens[i + 1].type == 4){
				cca_bytecode_add_byte(&bytecode, 0x02);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
			}
			i += 2;
		} else if (strcmp(tokens[i].value.string, "mov") == 0) {
			i += 4;
		} else {
			printf("[ERROR] unknown opcode '%s'\n", tokens[i].value.string);
			exit(1);
		}
	}

	fwrite(bytecode.bytecode, 1, bytecode.bytecodeLength, fp);
	fclose(fp);
	return;
}

void cca_assemble(char* fileName) {
	// optain the assembly code
	cca_file_content content = ccvm_program_load(fileName);
	
	// lex the assembly code into tokens
	cca_token* tokens = cca_assembler_lex(content);

	// recognise opcodes
	cca_assembler_recognize(tokens);

	// generate bytecode
	cca_assembler_bytegeneration(tokens);

	free(tokens);
	free(content.content);
}

#endif