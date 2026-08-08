#include <stdio.h>
#include <stdint.h>

void disassemble(uint32_t inst) {
    // Extract common fields using bitwise shifting and masking
    // Standard 32-bit alignment based on standard ISA lengths
    uint32_t opcode = (inst >> 26) & 0x3F; // Top 6 bits [cite: 18]
    uint32_t rs     = (inst >> 21) & 0x1F; // 5 bits [cite: 20]
    uint32_t rt     = (inst >> 16) & 0x1F; // 5 bits [cite: 21]
    uint32_t rd     = (inst >> 11) & 0x1F; // 5 bits [cite: 22]
    uint32_t funct  = inst & 0x7FF;        // Bottom 11 bits for R-Type [cite: 23]
    
    // Immediate and Address extraction
    uint16_t imm        = inst & 0xFFFF;         // 16-bit immediate
    int16_t  signed_imm = (int16_t)imm;          // Sign-extended 16-bit immediate
    uint32_t address    = inst & 0x3FFFFFF;      // 26-bit jump address [cite: 110]

    switch (opcode) {
        case 0: // R-Type Instructions [cite: 19]
            switch (funct) {
                case 0: printf("ADD X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 1: printf("SUB X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 2: printf("AND X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 3: printf("OR X%u, X%u, X%u\n", rd, rs, rt); break;  
                case 4: printf("XOR X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 5: printf("SLL X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 6: printf("SRL X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 7: printf("SRA X%u, X%u, X%u\n", rd, rs, rt); break; 
                case 8: printf("SLT X%u, X%u, X%u\n", rd, rs, rt); break;
                case 9: printf("MUL X%u, X%u, X%u\n", rd, rs, rt); break;
                default: printf("UNKNOWN R-TYPE (Funct: %u)\n", funct); break;
            }
            break;

        // I-Type Instructions
        case 1: // ADDI uses sign-extended immediate [cite: 11, 73]
            // Note: I-Type registers are parsed as Rs and Rd in your table 
            printf("ADDI X%u, X%u, %d\n", rt, rs, signed_imm); 
            break;
        case 2: // ANDI uses zero-extended immediate [cite: 12, 77]
            printf("ANDI X%u, X%u, 0x%04X\n", rt, rs, imm); 
            break;
        case 3: // ORI uses zero-extended immediate [cite: 13, 82]
            printf("ORI X%u, X%u, 0x%04X\n", rt, rs, imm); 
            break;
        case 4: // LUI [cite: 86]
            // Target register is held in the 'Rs' bits (25-21) according to the table [cite: 87]
            printf("LUI X%u, 0x%04X\n", rs, imm); 
            break;
            
        // Memory Instructions
        case 5: // LW [cite: 91]
            printf("LW X%u, %d(X%u)\n", rt, signed_imm, rs); 
            break;
        case 6: // SW [cite: 95]
            printf("SW X%u, %d(X%u)\n", rt, signed_imm, rs); 
            break;

        // Control Flow Instructions
        case 7: // BEQ [cite: 100]
            printf("BEQ X%u, X%u, %d\n", rs, rt, signed_imm); 
            break;
        case 8: // BNE [cite: 105]
            printf("BNE X%u, X%u, %d\n", rs, rt, signed_imm); 
            break;

        // J-Type Instructions
        case 9: // J [cite: 110]
            printf("J 0x%07X\n", address); 
            break;
        case 10: // JAL [cite: 112]
            printf("JAL 0x%07X\n", address); 
            break;
        case 11: // JR [cite: 114]
            // Register is stored in the Rs bits (25-21) [cite: 115]
            printf("JR X%u\n", rs); 
            break;

        default:
            printf("UNKNOWN INSTRUCTION (Opcode: %u)\n", opcode);
            break;
    }
}

// Example usage and testing
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <hex_instruction>\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    uint8_t buffer[4];
    uint32_t pc = 0;

    while (fread(buffer, 1, 4, f) == 4) {
        uint32_t inst = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
        printf("0x%04X (0x%08X): ", pc, inst);

        disassemble(inst);

        pc += 4;
    }

    fclose(f);


    return 0;
}