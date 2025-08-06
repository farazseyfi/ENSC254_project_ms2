/* Name : Faraz Seyfi , Niyousha Amin Afshari
 * Student ID: 301543610, 301465214
 * Course name: ENSC 254 - Summer 2025
 * Institution: Simon Fraser University
 * This file contains solutions for the Data Lab assignment.
 * Use is permitted only for educational and non-commercial purposes.
 */


#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "config.h"
#include "types.h"
#include "cache.h"
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
/// Functionality
///////////////////////////////////////////////////////////////////////////////

extern simulator_config_t sim_config; // Simulation Configuration setting
extern uint64_t miss_count; // Cache miss count
extern uint64_t hit_count;  // Cache hit count
extern uint64_t total_cycle_counter; // Total number of Cycles executed
extern uint64_t stall_counter;  // Number of pipeline stalls
extern uint64_t branch_counter; // Number of branch instructions executed
extern uint64_t fwd_exex_counter; // Forwarding EX → EX counter
extern uint64_t fwd_exmem_counter; // Forwarding EX → MEM counter
extern uint64_t mem_access_counter; // Memory access counter

///////////////////////////////////////////////////////////////////////////////
/// RISC-V Pipeline Register Types
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  Instruction instr;
  uint32_t instr_addr; // Address of the fetched instruction
  uint32_t next_pc; // PC + 4 - Next instruction address
 
}ifid_reg_t;

typedef struct
{
  Instruction instr;
  uint32_t instr_addr; // Instruction address

  uint8_t rs1; // Source Register 1
  uint8_t rs2; // Source Register 2
  uint8_t rd; // Destination Register

  int32_t reg_val1; // Read the value from rs1
  int32_t reg_val2; // Read the value from rs2
  int32_t imm; // Immediate value

  // Boolean
  bool memRead; // True if it's a load instruction (Since we need to read from memory)
  bool memWrite; // True if it's store instruction (Since we need to write in memory)
  bool regWrite; // True if the instruction writes back into the register
  bool branch; // True if it's a branch instruction
  bool use_imm; // True if ALU operand 2 is imm

  
}idex_reg_t;

typedef struct
{
  Instruction instr;
  uint32_t instr_addr;

  int32_t alu_result; // ALU operation result
  int32_t store_val; // The value to be stored (for store instructions)
  uint8_t rd; // Destination Register

  bool memRead; // True if it's a load instruction (Since we need to read from memory)
  bool memWrite; // True if it's store instruction (Since we need to write in memory)
  bool regWrite; // True if the instruction writes back into the register
  bool branch_taken; // True if branch is taken

  // Branch-related fields
  uint32_t branch_target; // Branch target address
  bool is_jalr; // True if this is a JALR instruction
  uint32_t jalr_base; // Base register value for JALR


}exmem_reg_t;

typedef struct
{
  Instruction instr;
  uint32_t    instr_addr;

  int32_t mem_data; // Read the data from memory
  int32_t alu_result; // ALU operation result
  uint8_t rd; // Destination Register

  bool regWrite; // True if the instruction writes back into the register
  bool mem_to_reg; // Selects memory data or ALU result for WB
  
}memwb_reg_t;


///////////////////////////////////////////////////////////////////////////////
/// Register types with input and output variants for simulator
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  ifid_reg_t inp;
  ifid_reg_t out;
}ifid_reg_pair_t;

typedef struct
{
  idex_reg_t inp;
  idex_reg_t out;
}idex_reg_pair_t;

typedef struct
{
  exmem_reg_t inp;
  exmem_reg_t out;
}exmem_reg_pair_t;

typedef struct
{
  memwb_reg_t inp;
  memwb_reg_t out;
}memwb_reg_pair_t;

///////////////////////////////////////////////////////////////////////////////
/// Functional pipeline requirements
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  ifid_reg_pair_t  ifid_preg;
  idex_reg_pair_t  idex_preg;
  exmem_reg_pair_t exmem_preg;
  memwb_reg_pair_t memwb_preg;
}pipeline_regs_t;

typedef struct
{
  bool      pcsrc; // Select the next program counter source
  uint32_t  pc_src0; // PC + 4 (default)
  uint32_t  pc_src1; // Branch or jump target address
  /**
   * Add other fields here
   */
}pipeline_wires_t;


///////////////////////////////////////////////////////////////////////////////
/// Function definitions for different stages
///////////////////////////////////////////////////////////////////////////////

/**
 * output : ifid_reg_t
 **/ 
ifid_reg_t stage_fetch(pipeline_wires_t* pwires_p, regfile_t* regfile_p, Byte* memory_p);

/**
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p);

/**
 * output : exmem_reg_t
 **/ 
exmem_reg_t stage_execute(idex_reg_t idex_reg, pipeline_wires_t* pwires_p);

/**
 * output : memwb_reg_t
 **/ 
memwb_reg_t stage_mem(exmem_reg_t exmem_reg, pipeline_wires_t* pwires_p, Byte* memory, Cache* cache_p);

/**
 * output : write_data
 **/ 
void stage_writeback(memwb_reg_t memwb_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p);

void cycle_pipeline(regfile_t* regfile_p, Byte* memory_p, Cache* cache_p, pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, bool* ecall_exit);

void bootstrap(pipeline_wires_t* pwires_p, pipeline_regs_t* pregs_p, regfile_t* regfile_p);

#endif  // __PIPELINE_H__
