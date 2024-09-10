/*
    MIT License

    Copyright (c) 2024 Nicholas A. Fraidakis

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <stdlib.h>
#include <math.h>
#include "instr.h"
#include "vm_macros.h"
#include <stdio.h>
#include <stdint.h>

typedef struct {
    enum instr_set instruction: 8;
    uint8_t rD: 4;
    uint8_t rN: 4;
    uint8_t is_immediate: 1;
    uint8_t set_flag: 1;
    uint8_t condition: 4;
    int: 0;
    union {
        uint32_t immediate;
        uint8_t rM: 4;
    };
    
} Instruction;

asm_instruction total_instructions[] = {
    {"halt", halt, 0, 0, 0}, 
    {"ldr", ldr, 1, 1, 0}, 
    {"str", str, 1, 1, 0},
    {"push", push, 1, 1, 0},
    {"pop", pop, 0, 1, 0},
    {"add", add, 1, 1, 1},
    {"sub", sub, 1, 1, 1},
    {"mul", mul, 1, 1, 1},
    {"div", divd, 1, 1, 1},
    {"bsl", bsl, 1, 1, 1},
    {"bsr", bsr, 1, 1, 1},
    {"rnapi", rnapi, 0, 0, 0},
    {"mov", mov, 1, 1, 1},
    {"cmp", add, 0, 1, 1},
    {"clrpsr", clrpsr, 1, 1, 1}};
uint64_t current_line = 0;

uint8_t Contains_Substring_At_Start(char * base, char * substring) {
    uint64_t i = 0;
    while (base[i] == substring[i]) {
        if (base[i] == '\0' || substring[i] == '\0') break;
        i += 1;
    }
    if (substring[i] == '\0') return i;
    return 0;
}
uint8_t Are_Strings_Equal(char * s0, char * s1) {
    uint64_t i = 0;
    while (s0[i] == s1[i] && s0[i] != '\0' && s1[i] != '\0') i++;
    if (s0[i] == s1[i] == '\0') return 1;
    return 0;
}
void Error(char * message) {
    printf(message);
    exit(1);
}

inline uint8_t Is_Char_Number(char text) {
    return (text > 47 && text < 58);
}

uint32_t Parse_Number(char * text) {
    uint8_t i = 0;
    uint32_t current_number = 0;
    while (Is_Char_Number(text[i]))  current_number = current_number * 10 + text[i++] - '0';
    return current_number;
}

int64_t Is_Line_Instruction(char * text) {
    char is_label = 0;
    uint64_t offset = 0;
    for (; text[offset] != '\n' && text[offset]; offset++) if (text[offset] == ':') is_label++;
    if (is_label) return -(offset + 1);
    return offset + 1;
}
uint8_t Parse_Register(char * text) {
    if (text[0] != 'r' && !Is_Char_Number(text[1])) Error("Invalid Register! You can only use the general purpose registers r0-15\n");
    uint8_t register_number = text[1] - '0';
    if (text[2] == ' ' || text[2] == ',') return register_number;
    register_number *= 10;
    register_number += text[2] - '0';
    return register_number;
}
void Parse_Instruction(Instruction * address, char * text) {
    uint8_t operand_numbers = 1;
    
    // Gets number of operands (parameters)

    for (int i = 0; text[i] != '\n' && text[i] != '\0'; i++) operand_numbers = text[i] == ',' ? operand_numbers + 1 : operand_numbers; 

    // Skips white space

    while (*text == ' ') text++;

    // Checks if line is incomplete or nothing is on it

    if (*text == '\n' || *text == '\0') return;

    uint8_t instruction_number = 0; // Set to 255 because instruction_number is incremented at start

    // Searches through all instructions to find out current instruction
    do {
        // Sets bytecode instruction
        uint8_t is_instruction = Contains_Substring_At_Start(text, total_instructions[instruction_number].name); // Checks if the instruction in text
        address -> instruction = total_instructions[instruction_number].bytecode; // Sets the instruction's bytecode value
        if (!is_instruction) {
            instruction_number++; // Increments instruction_number
            continue; // Continutes searching through all instructions
        }
        text += is_instruction; // Adds instruction length
        
        // Checks for flags or conditions
        
        if (*text == 's') address->set_flag = 1, text+=1;
        if (*text == ' ') while (*text == ' ') text++;
        else if (Contains_Substring_At_Start(text, "ge")) address->condition = greater_than | equal_to, text+=2;
        else if (Contains_Substring_At_Start(text, "gt")) address->condition = greater_than, text+=2;
        else if (Contains_Substring_At_Start(text, "le")) address->condition = less_than | equal_to, text+=2;
        else if (Contains_Substring_At_Start(text, "lt")) address->condition = less_than, text+=2;
        else if (Contains_Substring_At_Start(text, "eq")) address->condition = equal_to, text+=2;
        else Error("Invalid Condition");
        

        // Parses the Register Destination
        if (total_instructions[instruction_number].can_include_register_n) {
            if (operand_numbers == 2) address -> rD = Parse_Register(text), address -> rN = address -> rD;
            else address -> rD = Parse_Register(text);
            text += address -> rD > 9 ? 3 : 2;
        }

        // Skips white space and commas

        while (*text == ' ' || *text == ',') text++;

        // Parses the Register Number if there are 3 operands

        if (operand_numbers == 3 && total_instructions[instruction_number].can_include_register_n) address -> rN = Parse_Register(text), text += address -> rN > 9 ? 3 : 2;

        // Skips white space and commas

        while (*text == ' ' || *text == ',') text++;

        // Parses the Register Modified

        if (*text == '=' && total_instructions[instruction_number].can_include_immediate) address->immediate = Parse_Number(text + 1), address -> is_immediate = 1, text += 1 + (int) log10f((float) address -> immediate);
        else if (total_instructions[instruction_number].can_include_register_m) address -> rM = Parse_Register(text), text += address -> rM > 9 ? 3 : 2;
        // Ends loop
        break;
    } while (instruction_number < sizeof(total_instructions)/sizeof(asm_instruction) - 1);
}

void Assemble(char * path) {
    uint64_t i = 0;
    // Opening assembly file

    FILE * asm_file = fopen(path, "rb");
    if (!asm_file) Error("Couldn't open file");

    // Geting the size of the White Pine asm file

    uint64_t size;
    fseek(asm_file, 0, SEEK_END);
    size = ftell(asm_file);
    fseek(asm_file, 0, SEEK_SET);

    // Copying asm file to heap

    char * asm_text = malloc(size);
    if (!asm_text) Error("Couldn't allocate memory for assembly file in heap\n");
    fread(asm_text, 1, size, asm_file);

    // Getting the amount of lines

    uint64_t total_lines = 1;
    uint64_t amount_of_instructions = 0;
    while (i < size) {
        if (asm_text[i] == '\n') total_lines++;
        i++;
    }

    i = 0;
    
    // Getting the amount of instructions

    while (i < size) {
        int64_t line_jump = Is_Line_Instruction(asm_text + i);
        if (line_jump > 0) {
            amount_of_instructions++; 
            i += line_jump;
            continue;
        }
        i -= line_jump;
    }
    
    // Allocating space of binary on heap

    uint64_t * binary = malloc(sizeof(uint64_t) * amount_of_instructions);
    if (!binary) Error("Couldn't allocate memory for assembly file in heap");

    // Creating White Pine Binary (excutable)

    FILE * output_binary = fopen("binary.wpb", "wb");
    if (!output_binary) Error("Couldn't write binary file");

    // Parsing White Pine assembly
    
    i = 0;
    char * current_asm_char = asm_text;
    while (current_line < total_lines) {
        Instruction instruction = {0, 0, 0, 0 ,0 , 15, 0};
        Parse_Instruction(&instruction, current_asm_char);
        binary[current_line] = 0;
        binary[current_line] |= (uint64_t)instruction.instruction << opcode_shift;
        binary[current_line] |= (uint64_t)instruction.rD << register_dest_shift;
        binary[current_line] |= (uint64_t)instruction.rN << register_num_shift;
        binary[current_line] |= (uint64_t)instruction.is_immediate ? instruction.immediate : instruction.rM;
        binary[current_line] |= (uint64_t)instruction.condition << condition_shift;
        binary[current_line] |= (uint64_t)instruction.set_flag << set_shift;
        binary[current_line] |= (uint64_t)instruction.is_immediate << immediate_shift;
        current_line++;
        while (*current_asm_char != '\n' && *current_asm_char != '\0' && current_asm_char - asm_text < sizeof(uint64_t) * amount_of_instructions) current_asm_char++;
        current_asm_char++;
    }

    // Writing binary to disk

    fwrite(binary, 8, amount_of_instructions, output_binary);

    // Closing, freeing and deinitizing variables

    fclose(asm_file);
    fclose(output_binary);
    free(asm_text);
    free(binary);
}
int main(int argc, char * argv[]) {
    if (argc == 1) printf("Usage:\n  sno_pinecone: [path-to-file] ");
    else Assemble(argv[1]);
    return 0;
}