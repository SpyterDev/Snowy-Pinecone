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
#include <stdio.h>
#include <stdint.h>

void Error(char * message) {
    printf(message);
    exit(1);
}

int64_t Is_Line_Instruction(char * text) {
    char is_label = 0;
    uint64_t offset = 0;
    for (; text[offset] != '\n' && text[offset]; offset++) if (text[offset] == ':') is_label++;
    if (is_label) return -(offset + 1);
    return offset + 1;
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
    if (!asm_text) Error("Couldn't allocate memory for assembly file in heap");
    fread(asm_text, 1, size, asm_file);

    // Getting the amount of lines

    uint64_t total_lines = 1;
    uint64_t total_instructions = 0;
    while (i < size) {
        if (asm_text[i] == '\n') total_lines++;
        i++;
    }

    i = 0;
    
    // Getting the amount of instructions

    while (i < size) {
        int64_t line_jump = Is_Line_Instruction(asm_text + i);
        if (line_jump > 0) {
            total_instructions++; 
            i += line_jump;
            continue;
        }
        i -= line_jump;
    }
    
    // Allocating space of binary on heap

    uint64_t * binary = malloc(sizeof(uint64_t) * total_instructions);
    if (!binary) Error("Couldn't allocate memory for assembly file in heap");

    // Creating White Pine Binary (excutable)

    FILE * output_binary = fopen("binary.wpb", "wb");
    if (!output_binary) Error("Couldn't write binary file");

    // Writing binary to disk

    fwrite(binary, 8, total_instructions, output_binary);

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