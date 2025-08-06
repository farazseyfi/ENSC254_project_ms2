/* Name : Faraz Seyfi , Niyousha Amin Afshari
 * Student ID: 301543610, 301465214
 * Course name: ENSC 254 - Summer 2025
 * Institution: Simon Fraser University
 * This file contains solutions for the Data Lab assignment.
 * Use is permitted only for educational and non-commercial purposes.
 */


#include <stdio.h> // for stderr
#include <stdlib.h> // for exit()
#include "types.h"
#include "utils.h"
#include "riscv.h"

void execute_rtype(Instruction, Processor *);
void execute_itype_except_load(Instruction, Processor *);
void execute_branch(Instruction, Processor *);
void execute_jal(Instruction, Processor *);
void execute_load(Instruction, Processor *, Byte *);
void execute_store(Instruction, Processor *, Byte *);
void execute_ecall(Processor *, Byte *);
void execute_lui(Instruction, Processor *);

void execute_instruction(uint32_t instruction_bits, Processor *processor,Byte *memory) {    
    Instruction instruction = parse_instruction(instruction_bits);
    switch(instruction.opcode) {
        case 0x33:
            execute_rtype(instruction, processor);
            break;
        case 0x13:
            execute_itype_except_load(instruction, processor);
            break;
        case 0x73:
            execute_ecall(processor, memory);
            break;
        case 0x63:
            execute_branch(instruction, processor);
            break;
        case 0x6F:
            execute_jal(instruction, processor);
            break;
        case 0x23:
            execute_store(instruction, processor, memory);
            break;
        case 0x03:
            execute_load(instruction, processor, memory);
            break;
        case 0x37:
            execute_lui(instruction, processor);
            break;
        default: // undefined opcode
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_rtype(Instruction instruction, Processor *processor) {
    switch (instruction.rtype.funct3){
        case 0x0:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // Add
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) +
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x1:
                    // Mul
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) *
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x20:
                    // Sub
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) -
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x1:
            switch(instruction.rtype.funct7){
                case 0x00: // Shift left logical (Sll) - Shift the value in rs1 to the left by number of bits in rs2 (lower 5 bits)
                    processor->R[instruction.rtype.rd] =
                        processor->R[instruction.rtype.rs1] << (processor->R[instruction.rtype.rs2] & 0x1f);
                    break;
                
                case 0x01: // Multiply High (mulh) - Multiplying two signed integers from rs1 and rs2 and storing the upper 32-bit
                    processor->R[instruction.rtype.rd] =
                        ((int64_t)(sWord)processor->R[instruction.rtype.rs1] *
                        (int64_t)(sWord)processor->R[instruction.rtype.rs2]) >> 32;
                    break;

                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;


        case 0x2:
            switch(instruction.rtype.funct7) {
                case 0x00: // Set less than (SLT) - rd = 1 if rs1 < rs2 , otherwise rd = 0 : weuse a ternary operator to take care of the logic
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1] < (sWord)processor->R[instruction.rtype.rs2]) ? 1 : 0;
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;

        case 0x4:
            switch(instruction.rtype.funct7) {
                case 0x00: // XOR
                     processor->R[instruction.rtype.rd] = 
                        processor->R[instruction.rtype.rs1] ^ processor->R[instruction.rtype.rs2];
                    break;
                case 0x01: // Div
                    processor->R[instruction.rtype.rd] = 
                        ((sWord)processor->R[instruction.rtype.rs1]) /
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;

                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;

        case 0x5:
            switch(instruction.rtype.funct7) {
                case 0x00: // Shift Right Logical (SRL) - shift the value in rs1 by number of bits in rs2 and store it in rd
                     processor->R[instruction.rtype.rd] =
                        processor->R[instruction.rtype.rs1] >> (processor->R[instruction.rtype.rs2] & 0x1F);
                    break;

                case 0x20: // Shift Right arithmetic (SRA) - Same as SRL but sign matters
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) >> (processor->R[instruction.rtype.rs2] & 0x1F);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;

            }
            break;

        case 0x6:
            switch(instruction.rtype.funct7){
                case 0x00: // Bitwise OR operation
                    processor->R[instruction.rtype.rd] =
                        processor->R[instruction.rtype.rs1] | processor->R[instruction.rtype.rs2];
                    break;

                case 0x01: // Remainder (REM) - calculates the remainder of rs1 / rs2
                    processor->R[instruction.rtype.rd] =
                         ((sWord)processor->R[instruction.rtype.rs1]) % ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                    
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;


        case 0x7:
            switch(instruction.rtype.funct7){
                case 0x00: // Bitwise AND operation
                    processor->R[instruction.rtype.rd] =
                        processor->R[instruction.rtype.rs1] & processor->R[instruction.rtype.rs2];
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;

        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
    // update PC
    processor->PC += 4;
}

void execute_itype_except_load(Instruction instruction, Processor *processor) {
    
    // Declearing some variables so I don't have to repeat
    Register rd = instruction.itype.rd;
    Register rs1 = instruction.itype.rs1;
    sWord imm = sign_extend_number(instruction.itype.imm, 12);
    uint32_t shamt = instruction.itype.imm & 0x1F; // extracting the lower 5 bits of imm to use as a shift amount

    
    switch (instruction.itype.funct3) {
        case 0x0: // Add immediate (ADDI) - adds a signed immediate value to the value in rs1
            processor->R[rd] = (sWord)processor->R[rs1] + imm;
            break;

        case 0x1: // Shift Left Logical immediate (SLLI) - performs a logical left shift on rs1 by a specific immediate value
            if ((instruction.itype.imm >> 5) == 0x00) { // upper 7 bits of immediate encodes funct7
                processor->R[rd] = processor->R[rs1] << shamt; // The lower 5 bits of immediate gives the shift amount
            }
            else {
                handle_invalid_instruction(instruction);
                exit(-1);
                break;
            }
            break;
        
        case 0x2: // Set Less Than Immediate (SLTI) - if rs1 < imm : rd = 1, otherwise rd = 0
            processor->R[rd] = ((sWord)processor->R[rs1] < imm ) ? 1 : 0;
            break;

        
        case 0x4: // Excelusive OR immediate (XORI) - performs a bitwise XOR between rs1 and a signed imm value
            processor->R[rd] = processor->R[rs1] ^ imm;
            break;


        case 0x5: // Funct7 = upper 7 bits of imm
            switch((instruction.itype.imm >> 5) & 0x7F) {
                case 0x00: // Shift Right Logical Immediate (SRLI) - Shifts right logically the rs1 value by a specific imm value
                   processor->R[rd] = processor->R[rs1] >> shamt; // The lower 5 bits of immediate gives the shift amount
                   break;
                case 0x20: // Shift Right arithmetic immediate (SRAI) - Shifts right arithmatic the rs1 value by imm value (the sign matters)
                   processor->R[rd] = ((sWord)processor->R[rs1]) >> shamt; // The lower 5 bits of immediate gives the shift amount
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;

        case 0x6: //OR immediate (ORI) - a bitwise OR operation between source register and a imm value
            processor->R[rd] = processor->R[rs1] | imm;
            break;

        case 0x7: // ANd immediate (ANDI) - a bitwise AND operation between source register and a imm value
            processor->R[rd] = processor->R[rs1] & imm;
            break;


        
        default:
            handle_invalid_instruction(instruction);
            break;
    }
    // update PC
    processor->PC += 4;
}

void execute_ecall(Processor *p, Byte *memory) {
    Register i;
    
    // syscall number is given by a0 (x10)
    // argument is given by a1
    switch(p->R[10]) {
        case 1: // print an integer
            printf("%d",p->R[11]);
            p->PC += 4;
            break;
        case 4: // print a string
            for(i=p->R[11];i<MEMORY_SPACE && load(memory,i,LENGTH_BYTE);i++) {
                printf("%c",load(memory,i,LENGTH_BYTE));
            }
            p->PC += 4;
            break;
        case 10: // exit
            printf("exiting the simulator\n");
            exit(0);
            break;
        case 11: // print a character
            printf("%c",p->R[11]);
            p->PC += 4;
            break;
        default: // undefined ecall
            printf("Illegal ecall number %d\n", p->R[10]);
            exit(-1);
            break;
    }
}

void execute_branch(Instruction instruction, Processor *processor) {
    // Declearing some variables so I don't have to repeat
    Register rs1 = instruction.sbtype.rs1;
    Register rs2 = instruction.sbtype.rs2;
    uint32_t imm_sum = ((instruction.sbtype.imm7 & 0x40) << 6)     // imm[12]
            | ((instruction.sbtype.imm7 & 0x3F) << 5)     // imm[10:5]
            | ((instruction.sbtype.imm5 & 0x1E) >> 1)     // imm[4:1]
            | ((instruction.sbtype.imm5 & 0x01) << 11);   // imm[11]


    sWord imm = sign_extend_number(imm_sum, 13); // offset = 13 bits
    imm = imm << 1;
    
    switch (instruction.sbtype.funct3) {
        case 0x0: // Branch If equal (BEQ) - if rs1 == rs2, branches to PC + (offset << 1), otherwise keeps executing from pc + 4
            if (processor->R[rs1] == processor->R[rs2]) {
                processor->PC += imm;
            }
            else {
                processor->PC += 4;
            }
            break;

        case 0x1: // Branch IF Not Equal (BNE) - if rs1 != rs2, branches to PC + (offset << 1), otherwise keeps executing from pc + 4
            if (processor->R[rs1] != processor->R[rs2]) {
                processor->PC += imm;
            }
            else {
                processor->PC += 4;
            }
            break;
        
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_load(Instruction instruction, Processor *processor, Byte *memory) {
    // Declearing variables so I don't have to repeat
    Register rd = instruction.itype.rd; // declearing destination register
    Register rs1 = instruction.itype.rs1; // declearing Source Register 1
    sWord imm = sign_extend_number(instruction.itype.imm, 12); // Signed offset value
    Address addr = processor->R[rs1] + imm; // Memory Location to store the data
    switch (instruction.itype.funct3) {

        case 0x0: // Load Byte (LB) = Loads a Byte (8 bits) from memory adress in rs1 + offset into a destination Register - Signed
            processor->R[rd] = (sByte)load(memory, addr, LENGTH_BYTE); // sByte: Signed 8 bit, and Using an already decleared function load()
            break;

        case 0x1: // Load Halfword (LH) = Loads a 16 bit halfword from memory adress in rs1 + offset into a destination Register - Signed
            processor->R[rd] = (sHalf)load(memory, addr, LENGTH_HALF_WORD); // sHalf: Signed 16 bit, and Using an already decleared function load()
            break;

        case 0x2: // Load Word (LW) = Loads a 32 bit memory adress in rs1 + offset into a destination Register
            processor->R[rd] = load(memory, addr, LENGTH_WORD);
            break;

        case 0x4: // Load Byte Unsigned (LBU) = Loads a Byte (8 bits) from memory adress in rs1 + offset into a destination Register - UnSigned
            processor->R[rd] = load(memory, addr, LENGTH_BYTE);
            break;

        case 0x5: // Load Halfword Unsigned (LHU) = Loads a 16 bit halfword from memory adress in rs1 + offset into a destination Register - UnSigned
            processor->R[rd] = load(memory, addr, LENGTH_HALF_WORD);
            break;
        
        
        default:
            handle_invalid_instruction(instruction);
            break;
    }
    // Advance PC
    processor->PC += 4;
}

void execute_store(Instruction instruction, Processor *processor, Byte *memory) {
    // Declearing variables so I don't have to repeat
    Register rs1 = instruction.stype.rs1;
    Register rs2 = instruction.stype.rs2;
    uint32_t imm_sum = instruction.stype.imm7 << 5 | instruction.stype.imm5;
    sWord imm = sign_extend_number(imm_sum, 12); // The offset
    Address addr = processor->R[rs1] + imm; // Final memory location to store the data

    switch (instruction.stype.funct3) {
        case 0x0: // Store Byte (SB) - Stores the least 8 sig bits of rs2 into memory address of rs1 + offset
            store(memory, addr, LENGTH_BYTE, processor->R[rs2]); // Using the store() decleared in riscv.h
            break;
        case 0x1: // Store HalfWord (SH) - Stores the least 16 sig of a source register (rs2) into memory adress of of rs1 + offset
            store(memory, addr, LENGTH_HALF_WORD, processor->R[rs2]);
            break;
        case 0x2: // Store the entire 32-bit value of source register into memory adress of base register + offset
            store(memory, addr, LENGTH_WORD, processor->R[rs2] );
            break;


        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
    // Advanc PC
    processor->PC += 4;
}



void execute_jal(Instruction instruction, Processor *processor) {
    Address return_adress = processor->PC + 4; // Adress to return to after the jump
    int offset = get_jump_offset(instruction); // Extract the 21-bit immediate offset from instruction                           
    processor->R[instruction.ujtype.rd] = return_adress; // Store the return adress in rd                     
    processor->PC += sign_extend_number(offset, 21); // Update the program counter by signed offset
}


void execute_lui(Instruction instruction, Processor *processor) {
    uint32_t up_imm = instruction.utype.imm << 12; // shiftig this 20 bit value 12 bits ot the left 
    if (instruction.utype.rd != 0){ // checks if register is 0, b/c we cannot directly load to a 0 register
        processor -> R[instruction.utype.rd] = up_imm; // utype.rd is the destination register 
    }
    processor->PC += 4;                                              
}

void store(Byte *memory, Address address, Alignment alignment, Word value) {
    switch (alignment) { 
        case 1: // Sb - store byte (1 byte)
        memory[address] = (Byte)(value & 0xFF); // takes LSB of value and stores it in 0xFF 
        break; 

        case 2: // sh - store halfword (2 bytes)
        memory[address] = (Byte)(value & 0xFF); // takes LSB of value and stores it in 0xFF 
        memory[address + 1] = (Byte)((value >> 8) & 0xFF); // shifting to the next byte, by shifting 8 bits to the left
        break; 

        case 4: // sw - store word (4 bytes)
        memory[address] = (Byte)(value & 0xFF); // takes LSB of value and stores it in 0xFF 
        memory[address + 1] = (Byte)((value >> 8) & 0xFF); // shifting to the next byte, by shifting 8 bits to the right
        memory[address + 2] = (Byte)((value >> 16) & 0xFF); // shifting to third byte, by shifting 16 bits to the right 
        memory[address + 3] = (Byte)((value >> 24) & 0XFF); // shifting to fourth byte, by shifting 24 buts to the right 
        break; 

        default: 
        handle_invalid_write(address); 
        break; 
    }
}

Word load(Byte *memory, Address address, Alignment alignment) {
    if(alignment == LENGTH_BYTE) {
        return memory[address];
    } else if(alignment == LENGTH_HALF_WORD) {
        return (memory[address+1] << 8) + memory[address];
    } else if(alignment == LENGTH_WORD) {
        return (memory[address+3] << 24) + (memory[address+2] << 16)
               + (memory[address+1] << 8) + memory[address];
    } else {
        printf("Error: Unrecognized alignment %d\n", alignment);
        exit(-1);
    }
}