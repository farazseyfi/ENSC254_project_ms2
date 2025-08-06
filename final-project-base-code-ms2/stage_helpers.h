#ifndef __STAGE_HELPERS_H__
#define __STAGE_HELPERS_H__

#include <stdio.h>
#include "utils.h"
#include "pipeline.h"

/// EXECUTE STAGE HELPERS ///

/**
 * input  : idex_reg_t
 * output : uint32_t alu_control signal
 **/
uint32_t gen_alu_control(idex_reg_t idex_reg)
{
  uint32_t alu_control = 0;
  
  switch(idex_reg.instr.opcode) {
    case 0x33: // R-type instructions
      switch(idex_reg.instr.rtype.funct3) {
        case 0x0: // add/sub
          if (idex_reg.instr.rtype.funct7 == 0x00) alu_control = 0x0; // add
          else if (idex_reg.instr.rtype.funct7 == 0x20) alu_control = 0x1; // sub
          break;
        case 0x1: // sll
          alu_control = 0x2;
          break;
        case 0x2: // slt
          alu_control = 0x3;
          break;
        case 0x3: // sltu
          alu_control = 0x4;
          break;
        case 0x4: // xor
          alu_control = 0x5;
          break;
        case 0x5: // srl/sra
          if (idex_reg.instr.rtype.funct7 == 0x00) alu_control = 0x6; // srl
          else if (idex_reg.instr.rtype.funct7 == 0x20) alu_control = 0x7; // sra
          break;
        case 0x6: // or
          alu_control = 0x8;
          break;
        case 0x7: // and
          alu_control = 0x9;
          break;
      }
      break;
      
    case 0x13: // I-type immediate instructions
      switch(idex_reg.instr.itype.funct3) {
        case 0x0: // addi
          alu_control = 0x0;
          break;
        case 0x1: // slli
          alu_control = 0x2;
          break;
        case 0x2: // slti
          alu_control = 0x3;
          break;
        case 0x3: // sltiu
          alu_control = 0x4;
          break;
        case 0x4: // xori
          alu_control = 0x5;
          break;
        case 0x5: // srli/srai
          if ((idex_reg.instr.itype.imm >> 10) == 0) alu_control = 0x6; // srli
          else alu_control = 0x7; // srai
          break;
        case 0x6: // ori
          alu_control = 0x8;
          break;
        case 0x7: // andi
          alu_control = 0x9;
          break;
      }
      break;
      
    case 0x03: // Load instructions
      alu_control = 0x0; // add for address calculation
      break;
      
    case 0x23: // Store instructions
      alu_control = 0x0; // add for address calculation
      break;
      
    case 0x63: // Branch instructions
      alu_control = 0x1; // sub for comparison
      break;
      
    case 0x6F: // J-type (jal) - calculate return address
    case 0x67: // I-type (jalr) - calculate return address
      alu_control = 0x0; // add for return address calculation
      break;
      
    case 0x37: // U-type (lui) - just pass immediate through
      alu_control = 0x0; // add with rs1=0, so result = 0 + immediate
      break;
  }
  
  return alu_control;
}

/**
 * input  : alu_inp1, alu_inp2, alu_control
 * output : uint32_t alu_result
 **/
uint32_t execute_alu(uint32_t alu_inp1, uint32_t alu_inp2, uint32_t alu_control)
{
  uint32_t result;
  switch(alu_control){
    case 0x0: //add
      result = alu_inp1 + alu_inp2;
      break;
    case 0x1: //sub
      result = alu_inp1 - alu_inp2;
      break;
    case 0x2: //sll
      result = alu_inp1 << (alu_inp2 & 0x1F);
      break;
    case 0x3: //slt
      result = ((int32_t)alu_inp1 < (int32_t)alu_inp2) ? 1 : 0;
      break;
    case 0x4: //sltu
      result = (alu_inp1 < alu_inp2) ? 1 : 0;
      break;
    case 0x5: //xor
      result = alu_inp1 ^ alu_inp2;
      break;
    case 0x6: //srl
      result = alu_inp1 >> (alu_inp2 & 0x1F);
      break;
    case 0x7: //sra
      result = (int32_t)alu_inp1 >> (alu_inp2 & 0x1F);
      break;
    case 0x8: //or
      result = alu_inp1 | alu_inp2;
      break;
    case 0x9: //and
      result = alu_inp1 & alu_inp2;
      break;
    default:
      result = 0xBADCAFFE;
      break;
  };
  return result;
}

/// DECODE STAGE HELPERS ///

/**
 * input  : Instruction
 * output : idex_reg_t
 **/
uint32_t gen_imm(Instruction instruction)
{
  int imm_val = 0;
  
  switch(instruction.opcode) {
    case 0x03: // I-type (load)
    case 0x13: // I-type (immediate)
    case 0x67: // I-type (jalr)
    case 0x73: // I-type (system)
      imm_val = sign_extend_number(instruction.itype.imm, 12);
      break;
    case 0x23: // S-type (store)
      imm_val = sign_extend_number(instruction.stype.imm5 | (instruction.stype.imm7 << 5), 12);
      break;
    case 0x63: // B-type (branch)
      imm_val = get_branch_offset(instruction);
      break;
    case 0x17: // U-type (auipc)
      imm_val = sign_extend_number(instruction.utype.imm, 20);
      break;
    case 0x37: // U-type (lui)
      imm_val = instruction.utype.imm << 12; // lui loads immediate << 12 (no sign extension)
      break;
    case 0x6F: // J-type (jal)
      imm_val = get_jump_offset(instruction);
      break;
    default: // R and undefined opcode
      break;
  };
  return imm_val;
}

/**
 * generates all the control logic that flows around in the pipeline
 * input  : Instruction
 * output : idex_reg_t
 **/
idex_reg_t gen_control(Instruction instruction)
{
  idex_reg_t idex_reg = {0};
  
  switch(instruction.opcode) {
    case 0x33:  //R-type
      idex_reg.rs1 = instruction.rtype.rs1;
      idex_reg.rs2 = instruction.rtype.rs2;
      idex_reg.rd = instruction.rtype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = false;
      idex_reg.use_imm = false;
      break;
      
    case 0x13:  //I-type (immediate)
      idex_reg.rs1 = instruction.itype.rs1;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.itype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = false;
      idex_reg.use_imm = true;
      break;
      
    case 0x03:  //I-type (load)
      idex_reg.rs1 = instruction.itype.rs1;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.itype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = true;
      idex_reg.memWrite = false;
      idex_reg.branch = false;
      idex_reg.use_imm = true;
      break;
      
    case 0x23:  //S-type (store)
      idex_reg.rs1 = instruction.stype.rs1;
      idex_reg.rs2 = instruction.stype.rs2;
      idex_reg.rd = 0;
      idex_reg.regWrite = false;
      idex_reg.memRead = false;
      idex_reg.memWrite = true;
      idex_reg.branch = false;
      idex_reg.use_imm = true;
      break;
      
    case 0x63:  //B-type (branch)
      idex_reg.rs1 = instruction.sbtype.rs1;
      idex_reg.rs2 = instruction.sbtype.rs2;
      idex_reg.rd = 0;
      idex_reg.regWrite = false;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = true;
      idex_reg.use_imm = true;
      break;
      
    case 0x37:  //U-type (lui)
      idex_reg.rs1 = 0;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.utype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = false;
      idex_reg.use_imm = true;
      break;
      
    case 0x17:  //U-type (auipc)
      idex_reg.rs1 = 0;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.utype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = false;
      idex_reg.use_imm = true;
      break;
      
    case 0x6F:  //J-type (jal)
      idex_reg.rs1 = 0;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.ujtype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = true;
      idex_reg.use_imm = true;
      break;
      
    case 0x67:  //I-type (jalr)
      idex_reg.rs1 = instruction.itype.rs1;
      idex_reg.rs2 = 0;
      idex_reg.rd = instruction.itype.rd;
      idex_reg.regWrite = true;
      idex_reg.memRead = false;
      idex_reg.memWrite = false;
      idex_reg.branch = true;
      idex_reg.use_imm = true;
      break;
      
    default:  // Remaining opcodes
      break;
  }
  return idex_reg;
}

/// MEMORY STAGE HELPERS ///

/**
 * evaluates whether a branch must be taken
 * input  : <open to implementation>
 * output : bool
 **/
bool gen_branch(uint32_t reg_val1, uint32_t reg_val2, Instruction instruction)
{
  bool branch_taken = false;
  
  switch(instruction.opcode) {
    case 0x63: // B-type (conditional branches)
      switch(instruction.sbtype.funct3) {
        case 0x0: // beq
          branch_taken = (reg_val1 == reg_val2);
          break;
        case 0x1: // bne
          branch_taken = (reg_val1 != reg_val2);
          break;
        case 0x4: // blt
          branch_taken = ((int32_t)reg_val1 < (int32_t)reg_val2);
          break;
        case 0x5: // bge
          branch_taken = ((int32_t)reg_val1 >= (int32_t)reg_val2);
          break;
        case 0x6: // bltu
          branch_taken = (reg_val1 < reg_val2);
          break;
        case 0x7: // bgeu
          branch_taken = (reg_val1 >= reg_val2);
          break;
        default:
          branch_taken = false;
          break;
      }
      break;
      
    case 0x6F: // J-type (jal) - unconditional jump
    case 0x67: // I-type (jalr) - unconditional jump
      branch_taken = true;
      break;
      
    default:
      branch_taken = false;
      break;
  }
  
  return branch_taken;
}


/// PIPELINE FEATURES ///

/**
 * Task   : Sets the pipeline wires for the forwarding unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void gen_forward(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p)
{
  // Get current instruction in EX stage (using output registers for current state)
  idex_reg_t idex_reg = pregs_p->idex_preg.out;
  exmem_reg_t exmem_reg = pregs_p->exmem_preg.out;
  memwb_reg_t memwb_reg = pregs_p->memwb_preg.out;
  
  // Initialize forwarding signals
  pwires_p->forward_rs1_ex = false;
  pwires_p->forward_rs2_ex = false;
  pwires_p->forward_rs1_mem = false;
  pwires_p->forward_rs2_mem = false;
  
  // Skip forwarding if current instruction is NOP or invalid
  if (idex_reg.instr.bits == 0x00000013 || idex_reg.instr.bits == 0) {
    return;
  }
  
  // Check for EX hazard (forwarding from EXMEM to EX) - HIGHER PRIORITY
  // Note: Cannot forward from load instructions in EXMEM stage (data not available yet)
  if (exmem_reg.regWrite && exmem_reg.rd != 0 && exmem_reg.instr.bits != 0x00000013 && !exmem_reg.memRead) {
    // Check if rs1 needs forwarding
    if (idex_reg.rs1 != 0 && idex_reg.rs1 == exmem_reg.rd && 
        (idex_reg.instr.opcode == 0x33 || idex_reg.instr.opcode == 0x13 || 
         idex_reg.instr.opcode == 0x03 || idex_reg.instr.opcode == 0x23 || 
         idex_reg.instr.opcode == 0x63 || idex_reg.instr.opcode == 0x67)) {
      pwires_p->forward_rs1_ex = true;
      pwires_p->forward_rs1_data = exmem_reg.alu_result;
      fwd_exex_counter++;
    }
    // Check if rs2 needs forwarding
    if (idex_reg.rs2 != 0 && idex_reg.rs2 == exmem_reg.rd && 
        (idex_reg.instr.opcode == 0x33 || idex_reg.instr.opcode == 0x23 || 
         idex_reg.instr.opcode == 0x63)) {
      pwires_p->forward_rs2_ex = true;
      pwires_p->forward_rs2_data = exmem_reg.alu_result;
      fwd_exex_counter++;
    }
  }
  
  // Check for MEM hazard (forwarding from MEMWB to EX) - LOWER PRIORITY, only if not already forwarded from EX
  if (memwb_reg.regWrite && memwb_reg.rd != 0 && memwb_reg.instr.bits != 0x00000013) {
    // Check if rs1 needs forwarding (and not already forwarded from EX)
    if (idex_reg.rs1 != 0 && idex_reg.rs1 == memwb_reg.rd && 
        !pwires_p->forward_rs1_ex && // Not already forwarded from EX
        (idex_reg.instr.opcode == 0x33 || idex_reg.instr.opcode == 0x13 || 
         idex_reg.instr.opcode == 0x03 || idex_reg.instr.opcode == 0x23 || 
         idex_reg.instr.opcode == 0x63 || idex_reg.instr.opcode == 0x67)) {
      pwires_p->forward_rs1_mem = true;
      pwires_p->forward_rs1_data = memwb_reg.mem_to_reg ? memwb_reg.mem_data : memwb_reg.alu_result;
      fwd_exmem_counter++;
    }
    // Check if rs2 needs forwarding (and not already forwarded from EX)
    if (idex_reg.rs2 != 0 && idex_reg.rs2 == memwb_reg.rd && 
        !pwires_p->forward_rs2_ex && // Not already forwarded from EX
        (idex_reg.instr.opcode == 0x33 || idex_reg.instr.opcode == 0x23 || 
         idex_reg.instr.opcode == 0x63)) {
      pwires_p->forward_rs2_mem = true;
      pwires_p->forward_rs2_data = memwb_reg.mem_to_reg ? memwb_reg.mem_data : memwb_reg.alu_result;
      fwd_exmem_counter++;
    }
  }
}

/**
 * Task   : Sets the pipeline wires for the hazard unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void detect_hazard(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  // Get current instruction in ID stage
  idex_reg_t idex_reg = pregs_p->idex_preg.out;
  exmem_reg_t exmem_reg = pregs_p->exmem_preg.out;
  
  // Initialize stall signal
  pwires_p->stall = false;
  
  // Check for load-use hazard
  // A load-use hazard occurs when:
  // 1. Previous instruction (in EXMEM) is a load instruction
  // 2. Current instruction (in IDEX) uses the register that the load writes to
  if (exmem_reg.memRead && exmem_reg.regWrite && exmem_reg.rd != 0) {
    // Check if current instruction uses the register that the load writes to
    if ((idex_reg.rs1 != 0 && idex_reg.rs1 == exmem_reg.rd) || 
        (idex_reg.rs2 != 0 && idex_reg.rs2 == exmem_reg.rd)) {
      
      // Set stall signal
      pwires_p->stall = true;
      
      // Stall and re-fetch the same instruction
      printf("[HZD]: Stalling and rewriting PC: 0x%08x\n", pregs_p->ifid_preg.out.instr_addr);
      
      // Don't update PC, so the same instruction will be fetched again
      // This is handled by not updating the PC in the cycle_pipeline function
      stall_counter++;
    }
  }
}


///////////////////////////////////////////////////////////////////////////////


/// RESERVED FOR PRINTING REGISTER TRACE AFTER EACH CLOCK CYCLE ///
void print_register_trace(regfile_t* regfile_p)
{
  // print
  for (uint8_t i = 0; i < 8; i++)       // 8 columns
  {
    for (uint8_t j = 0; j < 4; j++)     // of 4 registers each
    {
      uint8_t reg_num = i * 4 + j;
      if (reg_num < 10) {
        printf("r %d=%08x ", reg_num, regfile_p->R[reg_num]);
      } else {
        printf("r%d=%08x ", reg_num, regfile_p->R[reg_num]);
      }
    }
    printf("\n");
  }
  printf("\n");
}

#endif // __STAGE_HELPERS_H__
