#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_LABELS 1024

// --- Global State ---
typedef struct {
    char name[64];
    uint32_t address;
} Label;

Label symbol_table[MAX_LABELS];
int label_count = 0;
int macro_counter = 0;

// --- Helper Functions ---
char* trim(char* str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void add_label(const char *name, uint32_t address) {
    if (label_count >= MAX_LABELS) {
        printf("Error: Max labels reached.\n");
        exit(1);
    }
    char clean_name[64];
    sscanf(name, "%s", clean_name);
    strcpy(symbol_table[label_count].name, clean_name);
    symbol_table[label_count].address = address;
    label_count++;
}

int get_label_address(const char *name, uint32_t *address) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            *address = symbol_table[i].address;
            return 1;
        }
    }
    return 0;
}

uint32_t parse_reg(char *reg_str) {
    if (!reg_str) return 0;
    reg_str = trim(reg_str);
    if (reg_str[0] == 'X' || reg_str[0] == 'x') {
        return (uint32_t)atoi(&reg_str[1]) & 0x1F;
    }
    return 0;
}

int32_t resolve_operand(char *str, uint32_t current_pc, int is_relative) {
    if (!str) return 0;
    str = trim(str);
    uint32_t addr;
    if (get_label_address(str, &addr)) {
        if (is_relative) return (int32_t)(addr - (current_pc + 4));
        return addr;
    }
    return strtol(str, NULL, 0);
}

void write_bytes(FILE *out, uint32_t value, int bytes) {
    uint8_t buffer[4];
    for (int i = 0; i < bytes; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
    fwrite(buffer, 1, bytes, out);
}

// ==========================================
// PREPROCESSOR STAGE
// ==========================================
void preprocess(FILE *in, FILE *out) {
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) {
        char original_line[MAX_LINE];
        strcpy(original_line, line);
        
        char *clean_line = trim(line);
        if (strlen(clean_line) == 0 || clean_line[0] == ';' || clean_line[0] == '#') {
            fprintf(out, "%s\n", original_line); 
            continue;
        }

        char *colon = strchr(clean_line, ':');
        if (colon) {
            *colon = '\0';
            fprintf(out, "%s:\n", clean_line);
            clean_line = trim(colon + 1);
            if (strlen(clean_line) == 0) continue;
        }

        char buffer[MAX_LINE];
        strcpy(buffer, clean_line);
        
        char *mnemonic = strtok(buffer, " \t,()");
        if (!mnemonic) continue;
        
        for (int i = 0; mnemonic[i]; i++) mnemonic[i] = toupper(mnemonic[i]);

        if (strcmp(mnemonic, "MUL") == 0) {
            char *rd = trim(strtok(NULL, ","));
            char *rs = trim(strtok(NULL, ","));
            char *rt = trim(strtok(NULL, ", \t\r\n"));
            macro_counter++;

            fprintf(out, "    ; --- BEGIN MACRO: MUL %s, %s, %s ---\n", rd, rs, rt);
            fprintf(out, "    ADDI %s, X0, 0\n", rd);               
            fprintf(out, "    ADD X29, X0, %s\n", rs);              
            fprintf(out, "    ADD X30, X0, %s\n", rt);              
            fprintf(out, "__MACRO_MUL_LOOP_%d:\n", macro_counter);
            fprintf(out, "    BEQ X30, X0, __MACRO_MUL_END_%d\n", macro_counter); 
            fprintf(out, "    ANDI X28, X30, 1\n");                 
            fprintf(out, "    BEQ X28, X0, __MACRO_MUL_SKIP_%d\n", macro_counter);
            fprintf(out, "    ADD %s, %s, X29\n", rd, rd);          
            fprintf(out, "__MACRO_MUL_SKIP_%d:\n", macro_counter);
            fprintf(out, "    ADDI X28, X0, 1\n");                  
            fprintf(out, "    SLL X29, X29, X28\n");                
            fprintf(out, "    SRL X30, X30, X28\n");                
            fprintf(out, "    J __MACRO_MUL_LOOP_%d\n", macro_counter);
            fprintf(out, "__MACRO_MUL_END_%d:\n", macro_counter);
        } 
        else if (strcmp(mnemonic, "DIV") == 0) {
            char *rd = trim(strtok(NULL, ","));
            char *rs = trim(strtok(NULL, ","));
            char *rt = trim(strtok(NULL, ", \t\r\n"));
            macro_counter++;

            fprintf(out, "    ; --- BEGIN MACRO: DIV %s, %s, %s ---\n", rd, rs, rt);
            fprintf(out, "    ADDI %s, X0, 0\n", rd);               
            fprintf(out, "    ADD X29, X0, %s\n", rs);              
            fprintf(out, "    ADD X30, X0, %s\n", rt);              
            fprintf(out, "    BEQ X30, X0, __MACRO_DIV_END_%d\n", macro_counter); 
            fprintf(out, "__MACRO_DIV_LOOP_%d:\n", macro_counter);
            fprintf(out, "    SLT X28, X29, X30\n");                
            fprintf(out, "    BNE X28, X0, __MACRO_DIV_END_%d\n", macro_counter); 
            fprintf(out, "    SUB X29, X29, X30\n");                
            fprintf(out, "    ADDI X28, X0, 1\n");                  
            fprintf(out, "    ADD %s, %s, X28\n", rd, rd);          
            fprintf(out, "    J __MACRO_DIV_LOOP_%d\n", macro_counter);
            fprintf(out, "__MACRO_DIV_END_%d:\n", macro_counter);
        } 
        else {
            fprintf(out, "%s\n", clean_line);
        }
    }
}

// ==========================================
// ASSEMBLER STAGE (2 Passes)
// ==========================================
void assemble(FILE *in, FILE *out) {
    char line[MAX_LINE];
    char line_copy[MAX_LINE];
    uint32_t pc = 0;
    label_count = 0; // Reset state

    // --- PASS 1 ---
    while (fgets(line, sizeof(line), in)) {
        line[strcspn(line, "\r\n")] = 0;
        char *comment = strpbrk(line, ";#");
        if (comment) *comment = '\0';

        char *colon = strchr(line, ':');
        char *code_ptr = line;
        if (colon) {
            *colon = '\0';
            add_label(line, pc);
            code_ptr = colon + 1;
        }

        strcpy(line_copy, code_ptr);
        char *mnemonic = strtok(line_copy, " \t,()");
        if (!mnemonic) continue;

        for (int i = 0; mnemonic[i]; i++) mnemonic[i] = toupper(mnemonic[i]);

        if (strcmp(mnemonic, ".ORG") == 0 || strcmp(mnemonic, "ORG") == 0) {
            pc = strtol(strtok(NULL, " \t"), NULL, 0);
        } else if (strcmp(mnemonic, ".DB") == 0 || strcmp(mnemonic, "DB") == 0) {
            while (strtok(NULL, " \t,")) pc += 1;
        } else if (strcmp(mnemonic, ".DW") == 0 || strcmp(mnemonic, "DW") == 0) {
            while (strtok(NULL, " \t,")) pc += 2;
        } else if (strcmp(mnemonic, ".DD") == 0 || strcmp(mnemonic, "DD") == 0) {
            while (strtok(NULL, " \t,")) pc += 4;
        } else {
            pc += 4;
        }
    }

    // --- PASS 2 ---
    fseek(in, 0, SEEK_SET);
    pc = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), in)) {
        line_num++;
        line[strcspn(line, "\r\n")] = 0;
        char *comment = strpbrk(line, ";#");
        if (comment) *comment = '\0';

        char *colon = strchr(line, ':');
        char *code_ptr = colon ? colon + 1 : line;

        char *mnemonic = strtok(code_ptr, " \t,()");
        if (!mnemonic) continue;
        for (int i = 0; mnemonic[i]; i++) mnemonic[i] = toupper(mnemonic[i]);

        if (strcmp(mnemonic, ".ORG") == 0 || strcmp(mnemonic, "ORG") == 0) {
            uint32_t target_pc = strtol(strtok(NULL, " \t"), NULL, 0);
            while (pc < target_pc) { fputc(0, out); pc++; }
            continue;
        } 
        if (strcmp(mnemonic, ".DB") == 0 || strcmp(mnemonic, "DB") == 0 ||
            strcmp(mnemonic, ".DW") == 0 || strcmp(mnemonic, "DW") == 0 ||
            strcmp(mnemonic, ".DD") == 0 || strcmp(mnemonic, "DD") == 0) {
            
            int bytes = (mnemonic[1] == 'B') ? 1 : ((mnemonic[1] == 'W') ? 2 : 4);
            char *val_str;
            while ((val_str = strtok(NULL, " \t,")) != NULL) {
                write_bytes(out, strtol(val_str, NULL, 0), bytes);
                pc += bytes;
            }
            continue;
        }

        uint32_t instr = 0, opcode = 0, rs = 0, rt = 0, rd = 0, funct = 0;
        int32_t imm = 0;

        if (strcmp(mnemonic, "ADD") == 0 || strcmp(mnemonic, "SUB") == 0 ||
            strcmp(mnemonic, "AND") == 0 || strcmp(mnemonic, "OR") == 0 ||
            strcmp(mnemonic, "XOR") == 0 || strcmp(mnemonic, "SLL") == 0 ||
            strcmp(mnemonic, "SRL") == 0 || strcmp(mnemonic, "SRA") == 0 ||
            strcmp(mnemonic, "SLT") == 0) {
            
            opcode = 0;
            rd = parse_reg(strtok(NULL, " \t,()"));
            rs = parse_reg(strtok(NULL, " \t,()"));
            rt = parse_reg(strtok(NULL, " \t,()"));

            if (strcmp(mnemonic, "ADD") == 0) funct = 0;
            else if (strcmp(mnemonic, "SUB") == 0) funct = 1;
            else if (strcmp(mnemonic, "AND") == 0) funct = 2;
            else if (strcmp(mnemonic, "OR")  == 0) funct = 3;
            else if (strcmp(mnemonic, "XOR") == 0) funct = 4;
            else if (strcmp(mnemonic, "SLL") == 0) funct = 5;
            else if (strcmp(mnemonic, "SRL") == 0) funct = 6;
            else if (strcmp(mnemonic, "SRA") == 0) funct = 7;
            else if (strcmp(mnemonic, "SLT") == 0) funct = 8;

            instr = (opcode << 26) | (rs << 21) | (rt << 16) | (rd << 11) | (funct & 0x7FF);
        }
        else if (strcmp(mnemonic, "ADDI") == 0 || strcmp(mnemonic, "ANDI") == 0 || strcmp(mnemonic, "ORI") == 0) {
            rt = parse_reg(strtok(NULL, " \t,()"));
            rs = parse_reg(strtok(NULL, " \t,()"));
            imm = resolve_operand(strtok(NULL, " \t,()"), pc, 0); 
            if (strcmp(mnemonic, "ADDI") == 0) opcode = 1;
            else if (strcmp(mnemonic, "ANDI") == 0) opcode = 2;
            else if (strcmp(mnemonic, "ORI")  == 0) opcode = 3;
            instr = (opcode << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF);
        }
        else if (strcmp(mnemonic, "LUI") == 0) {
            opcode = 4;
            rt = parse_reg(strtok(NULL, " \t,()"));
            imm = resolve_operand(strtok(NULL, " \t,()"), pc, 0);
            instr = (opcode << 26) | (0 << 21) | (rt << 16) | (imm & 0xFFFF);
        }
        else if (strcmp(mnemonic, "LW") == 0 || strcmp(mnemonic, "SW") == 0) {
            rt = parse_reg(strtok(NULL, " \t,()"));
            imm = resolve_operand(strtok(NULL, " \t,()"), pc, 0); 
            rs = parse_reg(strtok(NULL, " \t,()")); 
            opcode = (strcmp(mnemonic, "LW") == 0) ? 5 : 6;
            instr = (opcode << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF);
        }
        else if (strcmp(mnemonic, "BEQ") == 0 || strcmp(mnemonic, "BNE") == 0) {
            rs = parse_reg(strtok(NULL, " \t,()"));
            rt = parse_reg(strtok(NULL, " \t,()"));
            imm = resolve_operand(strtok(NULL, " \t,()"), pc, 1); 
            opcode = (strcmp(mnemonic, "BEQ") == 0) ? 7 : 8;
            instr = (opcode << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF);
        }
        else if (strcmp(mnemonic, "J") == 0 || strcmp(mnemonic, "JAL") == 0) {
            imm = resolve_operand(strtok(NULL, " \t,()"), pc, 0); 
            opcode = (strcmp(mnemonic, "J") == 0) ? 9 : 10;
            instr = (opcode << 26) | (imm & 0x3FFFFFF);
        }
        else if (strcmp(mnemonic, "JR") == 0) {
            opcode = 11;
            rs = parse_reg(strtok(NULL, " \t,()"));
            instr = (opcode << 26) | (rs << 21);
        }
        else {
            printf("Error: Unknown instruction '%s'\n", mnemonic);
            continue;
        }

        write_bytes(out, instr, 4);
        pc += 4;
    }
}

// ==========================================
// ENTRY POINT & CLI ROUTING
// ==========================================
void print_usage(char *prog_name) {
    printf("Usage: %s [options] <input.asm> <output>\n", prog_name);
    printf("Options:\n");
    printf("  -E    Preprocess only (output text assembly)\n");
    printf("  -S    Assemble only (skip preprocessing)\n");
}

int main(int argc, char *argv[]) {
    int opt_E = 0;
    int opt_S = 0;
    char *in_filename = NULL;
    char *out_filename = NULL;

    // Basic CLI parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-E") == 0) {
            opt_E = 1;
        } else if (strcmp(argv[i], "-S") == 0) {
            opt_S = 1;
        } else if (in_filename == NULL) {
            in_filename = argv[i];
        } else if (out_filename == NULL) {
            out_filename = argv[i];
        }
    }

    if (!in_filename || !out_filename) {
        print_usage(argv[0]);
        return 1;
    }

    if (opt_E && opt_S) {
        printf("Error: Cannot use -E and -S together.\n");
        return 1;
    }

    FILE *in_file = fopen(in_filename, "r");
    if (!in_file) {
        printf("Error: Cannot open input file %s\n", in_filename);
        return 1;
    }

    // Route 1: Preprocess Only (-E)
    if (opt_E) {
        FILE *out_file = fopen(out_filename, "w");
        preprocess(in_file, out_file);
        printf("Preprocessed output written to %s\n", out_filename);
        fclose(out_file);
        fclose(in_file);
        return 0;
    }

    // Route 2: Assemble Only (-S)
    if (opt_S) {
        FILE *out_file = fopen(out_filename, "wb");
        assemble(in_file, out_file);
        printf("Binary generated directly to %s\n", out_filename);
        fclose(out_file);
        fclose(in_file);
        return 0;
    }

    // Route 3: Full Pipeline (Default)
    char temp_filename[] = "__temp_asm_XXXXXX";
#ifdef _WIN32
    // Windows fallback for tmp file
    strcpy(temp_filename, "__temp_asm.tmp");
#else
    int fd = mkstemp(temp_filename);
    if (fd != -1) close(fd);
#endif

    FILE *temp_file = fopen(temp_filename, "w+");
    if (!temp_file) {
        printf("Error creating temporary file.\n");
        fclose(in_file);
        return 1;
    }

    // Step 3a: Preprocess to temp file
    preprocess(in_file, temp_file);
    fclose(in_file);

    // Step 3b: Assemble temp file to final binary output
    FILE *out_file = fopen(out_filename, "wb");
    fseek(temp_file, 0, SEEK_SET); // Rewind temp file to start
    assemble(temp_file, out_file);
    
    printf("Build completed successfully: %s\n", out_filename);

    fclose(temp_file);
    fclose(out_file);
    remove(temp_filename); // Clean up

    return 0;
}