/* Name : Faraz Seyfi , Niyousha Amin Afshari
 * Student ID: 301543610, 301465214
 * Course name: ENSC 254 - Summer 2025
 * Institution: Simon Fraser University
 * This file contains solutions for the Data Lab assignment.
 * Use is permitted only for educational and non-commercial purposes.
 */



#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/* Unpacks the 32-bit machine code instruction given into the correct
 * type within the instruction struct */
Instruction parse_instruction(uint32_t instruction_bits) {
  
  Instruction instruction;
  instruction.opcode = instruction_bits & ((1U << 7) - 1);

  // Store the original instruction bits before we start parsing
  uint32_t original_bits = instruction_bits;
  
  // Might need it later
  uint32_t raw= instruction_bits;

  // Shift right to move to pointer to interpret next fields in instruction.
  instruction_bits >>= 7;

  switch (instruction.opcode) {
  // Handle no-op instruction (0x00000000) - used during pipeline flushing
  case 0x00:
    // For no-op, we can treat it as a special case or as an I-type with all zeros
    instruction.itype.rd = 0;
    instruction.itype.funct3 = 0;
    instruction.itype.rs1 = 0;
    instruction.itype.imm = 0;
    break;
    
  // R-Type
  case 0x33:
    // instruction: 0000 0001 0101 1010 0000 0100 1, destination : 01001
    instruction.rtype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101 1010 0000, func3 : 000
    instruction.rtype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3;

    // instruction: 0000 0001 0101 1010 0, src1: 10100
    instruction.rtype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101, src2: 10101
    instruction.rtype.rs2 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // funct7: 0000 000
    instruction.rtype.funct7 = instruction_bits & ((1U << 7) - 1);
    break;
  

  // I-type
  case 0x03:

  // Destination register (rd): next 5 bits
  instruction.itype.rd = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>= 5;

  // Function code (funct3): next 3 bits
  instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);
  instruction_bits >>= 3;
  
  // Source register 1 (rs1): next 5 bits
  instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>=5;

  // Immediate (imm) : remaining 12 bits
  instruction.itype.imm = instruction_bits & ((1U << 12) - 1);
  break;

  // I-Type
  case 0x13:

  // Destination register (rd): next 5 bits
  instruction.itype.rd = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>= 5;

  // Function code (funct3): next 3 bits
  instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);
  instruction_bits >>= 3;
  
  // Source register 1 (rs1): next 5 bits
  instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>=5;

  // Immediate (imm) : remaining 12 bits
  instruction.itype.imm = instruction_bits & ((1U << 12) - 1);
  break;
  


  // ECALL instruction (special case)
  case 0x73:
    // For ecall, we don't need to parse any additional fields
    // The instruction is fully defined by the opcode
    // Set all fields to 0 to indicate this is an ecall
    instruction.itype.rd = 0;
    instruction.itype.funct3 = 0;
    instruction.itype.rs1 = 0;
    instruction.itype.imm = 0;
    break;
  
  


  // S-type
  case 0x23:


  // imm[4:0]: next 5 bits
  instruction.stype.imm5 = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>=5;


  // Function code (funct3): next 3 bits
  instruction.stype.funct3 = instruction_bits & ((1U << 3) - 1);
  instruction_bits >>=3;


  // Source Register 1 (rs1): next 5 bits
  instruction.stype.rs1 = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>=5;


  // Source Register 2 (rs2): next 5 bits
  instruction.stype.rs2 = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>=5;


  // imm[11:5] : remaining 7 bits 
  instruction.stype.imm7 = instruction_bits & ((1U << 7) - 1);
  break;



  // SB Type
  case 0x63:
{
    uint32_t full_instruction = (instruction_bits << 7) | instruction.opcode;  

    instruction.sbtype.imm5   = (full_instruction >> 7)  & 0x1F;
    instruction.sbtype.funct3 = (full_instruction >> 12) & 0x7;
    instruction.sbtype.rs1    = (full_instruction >> 15) & 0x1F;
    instruction.sbtype.rs2    = (full_instruction >> 20) & 0x1F;
    instruction.sbtype.imm7   = (full_instruction >> 25) & 0x7F;

    break;
}



  
  // U-type (LUI and AUIPC)
  // U-type (LUI and AUIPC)
  case 0x37:  // LUI
  case 0x17:  // AUIPC
  // rd is bits [11:7] of the original word
  instruction.utype.rd  = (raw >> 7) & 0x1F;
  // imm is bits [31:12] of the original word
  instruction.utype.imm =  raw >> 12;
  break;




  


  // UJ type instruction
  case 0x6f: {
  
  // Extract destination register, located in bits [11:7]
  instruction.ujtype.rd = instruction_bits & ((1U << 5) - 1);
  instruction_bits >>= 5; // Shift right 

  // Extract raw 20-bit immediate (bits 31:12)
  instruction.ujtype.imm = instruction_bits & ((1U << 20) - 1);
  break;
}



  #ifndef TESTING
  default:
    exit(EXIT_FAILURE);
  #endif
  }
  
  // Set the bits field after all individual fields have been parsed
  instruction.bits = original_bits;
  
  return instruction;
}


/************************Helper functions************************/
/* Here, you will need to implement a few common helper functions, 
 * which you will call in other functions when parsing, printing, 
 * or executing the instructions. */

/* Sign extends the given field to a 32-bit integer where field is
 * interpreted an n-bit integer. */
int sign_extend_number(unsigned int field, unsigned int n) {
  int shift = 32 - n;
  return (int)(field << shift) >> shift;

}






/* Return the number of bytes (from the current PC) to the branch label using
 * the given branch instruction */

int get_branch_offset(Instruction instruction) {
    // imm[12]  = full_bit31 = sbtype.imm7 bit6
    unsigned int imm12   = (instruction.sbtype.imm7 >> 6) & 0x1;
    // imm[11]  = full_bit7  = sbtype.imm5 bit0
    unsigned int imm11   =  instruction.sbtype.imm5        & 0x1;
    // imm[10:5]= full_bits30:25 = sbtype.imm7 bits0–5
    unsigned int imm10_5 =  instruction.sbtype.imm7        & 0x3F;
    // imm[4:1] = full_bits11:8  = sbtype.imm5 bits1–4
    unsigned int imm4_1  = (instruction.sbtype.imm5 >> 1)  & 0xF;

    unsigned int combined_imm =
        (imm12   << 12) |
        (imm11   << 11) |
        (imm10_5 <<  5) |
        (imm4_1  <<  1);

    // sign-extend the 13-bit immediate — no extra <<1 here
    return sign_extend_number(combined_imm, 13);
}






/* Returns the number of bytes (from the current PC) to the jump label using the
 * given jump instruction */
/* Returns the number of bytes (from the current PC) to the jump label */
int get_jump_offset(Instruction instruction) {
    
  unsigned int imm = instruction.ujtype.imm; // Raw 20-bit immediate stored during parsing

     // Extract scattered immediate fields from the raw bits
    unsigned int imm_20 = (imm >> 19) & 0x1;      // bit 20 (MSB)     
    unsigned int imm_10_1 = (imm >> 9) & 0x3FF;   // bits 10:1
    unsigned int imm_11 = (imm >> 8) & 0x1;       // bit 11
    unsigned int imm_19_12 = imm & 0xFF;          // bits 19:12

    
    // Shift each part to its proper location in the 21-bit signed immediate
    unsigned int part_imm20 = imm_20 << 20; 
    unsigned int part_imm10_1 = imm_10_1 << 1;
    unsigned int part_imm11 = imm_11 << 11;
    unsigned int part_imm19_12 = imm_19_12 << 12;

    // Combine all parts into the full 21-bit immediate
    unsigned int full_imm = part_imm20 + part_imm19_12 + part_imm11 + part_imm10_1;

    // Sign-extend the 21-bit value to 32-bit signed int and return it
    return sign_extend_number(full_imm, 21);
}




/* Returns the number of bytes (from the current PC) to the base address using the
 * given store instruction */
int get_store_offset(Instruction instruction) {

  unsigned int combined_immediate = (instruction.stype.imm7 << 5) | (instruction.stype.imm5);

  int store_offset = sign_extend_number(combined_immediate, 12);

  return store_offset;
}

/************************Helper functions************************/

void handle_invalid_instruction(Instruction instruction) {
  printf("Invalid Instruction: 0x%08x\n", instruction.bits);
}

void handle_invalid_read(Address address) {
  printf("Bad Read. Address: 0x%08x\n", address);
  exit(-1);
}

void handle_invalid_write(Address address) {
  printf("Bad Write. Address: 0x%08x\n", address);
  exit(-1);
}