#include <stdbool.h>
#include "cache.h"
#include "riscv.h"
#include "types.h"
#include "utils.h"
#include "pipeline.h"
#include "stage_helpers.h"

uint64_t total_cycle_counter = 0;
uint64_t miss_count = 0;
uint64_t hit_count = 0;
uint64_t stall_counter = 0;
uint64_t branch_counter = 0;
uint64_t fwd_exex_counter = 0;
uint64_t fwd_exmem_counter = 0;
uint64_t mem_access_counter = 0;

simulator_config_t sim_config = {0};

///////////////////////////////////////////////////////////////////////////////

void bootstrap(pipeline_wires_t* pwires_p, pipeline_regs_t* pregs_p, regfile_t* regfile_p)
{
  // PC src must get the same value as the default PC value
  pwires_p->pc_src0 = regfile_p->PC;
}

///////////////////////////
/// STAGE FUNCTIONALITY ///
///////////////////////////

/**
 * STAGE  : stage_fetch
 * output : ifid_reg_t
 **/ 
ifid_reg_t stage_fetch(pipeline_wires_t* pwires_p, regfile_t* regfile_p, Byte* memory_p)
{
  ifid_reg_t ifid_reg = {0};
  
  // Fetch instruction from memory at current PC
  uint32_t instruction_bits = *(uint32_t*)(memory_p + regfile_p->PC);
  
  ifid_reg.instr = parse_instruction(instruction_bits);
  
  ifid_reg.instr_addr = regfile_p->PC;
  ifid_reg.next_pc = regfile_p->PC + 4;
  
  return ifid_reg;
}

/**
 * STAGE  : stage_decode
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  idex_reg_t idex_reg = {0};
  
  // Copy instruction and address
  idex_reg.instr = ifid_reg.instr;
  idex_reg.instr_addr = ifid_reg.instr_addr;
  
  // Generate control signals
  idex_reg_t control = gen_control(ifid_reg.instr);
  idex_reg.memRead = control.memRead;
  idex_reg.memWrite = control.memWrite;
  idex_reg.regWrite = control.regWrite;
  idex_reg.branch = control.branch;
  idex_reg.use_imm = control.use_imm;
  
  // Extract register numbers
  idex_reg.rs1 = control.rs1;
  idex_reg.rs2 = control.rs2;
  idex_reg.rd = control.rd;
  
  // Read register values
  idex_reg.reg_val1 = regfile_p->R[idex_reg.rs1];
  idex_reg.reg_val2 = regfile_p->R[idex_reg.rs2];
  
  // Generate immediate value
  idex_reg.imm = gen_imm(ifid_reg.instr);
  
  return idex_reg;
}

/**
 * STAGE  : stage_execute
 * output : exmem_reg_t
 **/ 
exmem_reg_t stage_execute(idex_reg_t idex_reg, pipeline_wires_t* pwires_p)
{
  exmem_reg_t exmem_reg = {0};
  
  // Copy instruction and address
  exmem_reg.instr = idex_reg.instr;
  exmem_reg.instr_addr = idex_reg.instr_addr;
  
  // Copy control signals
  exmem_reg.memRead = idex_reg.memRead;
  exmem_reg.memWrite = idex_reg.memWrite;
  exmem_reg.regWrite = idex_reg.regWrite;
  exmem_reg.rd = idex_reg.rd;
  
  // Prepare ALU inputs with forwarding
  uint32_t alu_inp1 = idex_reg.reg_val1;
  uint32_t alu_inp2 = idex_reg.use_imm ? idex_reg.imm : idex_reg.reg_val2;
  
  // Apply forwarding for rs1
  if (pwires_p->forward_rs1_ex) {
    alu_inp1 = pwires_p->forward_rs1_data;
  } else if (pwires_p->forward_rs1_mem) {
    alu_inp1 = pwires_p->forward_rs1_data;
  }
  
  // Apply forwarding for rs2
  if (pwires_p->forward_rs2_ex) {
    alu_inp2 = pwires_p->forward_rs2_data;
  } else if (pwires_p->forward_rs2_mem) {
    alu_inp2 = pwires_p->forward_rs2_data;
  }
  
  // For JAL and JALR, calculate return address (PC + 4)
  if (idex_reg.instr.opcode == 0x6F || idex_reg.instr.opcode == 0x67) {
    alu_inp1 = idex_reg.instr_addr;
    alu_inp2 = 4;
  }
  
  // Generate ALU control signal
  uint32_t alu_control = gen_alu_control(idex_reg);
  
  // Execute ALU operation
  exmem_reg.alu_result = execute_alu(alu_inp1, alu_inp2, alu_control);
  
  // Store value for store instructions (use forwarded value if available)
  if (pwires_p->forward_rs2_ex) {
    exmem_reg.store_val = pwires_p->forward_rs2_data;
  } else if (pwires_p->forward_rs2_mem) {
    exmem_reg.store_val = pwires_p->forward_rs2_data;
  } else {
    exmem_reg.store_val = idex_reg.reg_val2;
  }
  
  // Handle branch logic - just evaluate the condition, don't take the branch yet
  if (idex_reg.branch) {
    exmem_reg.branch_taken = gen_branch(alu_inp1, alu_inp2, idex_reg.instr);
    exmem_reg.branch_target = idex_reg.instr_addr + idex_reg.imm;
    exmem_reg.is_jalr = (idex_reg.instr.opcode == 0x67);
    exmem_reg.jalr_base = alu_inp1; // Use forwarded value if available
  } else {
    exmem_reg.branch_taken = false;
  }
  
  return exmem_reg;
}

/**
 * STAGE  : stage_mem
 * output : memwb_reg_t
 **/ 
memwb_reg_t stage_mem(exmem_reg_t exmem_reg, pipeline_wires_t* pwires_p, Byte* memory_p, Cache* cache_p)
{
  memwb_reg_t memwb_reg = {0};
  
  // Copy instruction and address
  memwb_reg.instr = exmem_reg.instr;
  memwb_reg.instr_addr = exmem_reg.instr_addr;
  
  // Copy control signals and data
  memwb_reg.regWrite = exmem_reg.regWrite;
  memwb_reg.rd = exmem_reg.rd;
  memwb_reg.alu_result = exmem_reg.alu_result;
  
  // Handle memory operations
  if (exmem_reg.memRead) {
    // Load instruction - read from memory
    // Check instruction type for proper loading
    if (exmem_reg.instr.opcode == 0x03) { // Load instructions
      switch (exmem_reg.instr.itype.funct3) {
        case 0x0: // lb (load byte)
          memwb_reg.mem_data = (int8_t)(*(uint8_t*)(memory_p + exmem_reg.alu_result));
          break;
        case 0x1: // lh (load halfword)
          memwb_reg.mem_data = (int16_t)(*(uint16_t*)(memory_p + exmem_reg.alu_result));
          break;
        case 0x2: // lw (load word)
          memwb_reg.mem_data = *(int32_t*)(memory_p + exmem_reg.alu_result);
          break;
        case 0x4: // lbu (load byte unsigned)
          memwb_reg.mem_data = (uint8_t)(*(uint8_t*)(memory_p + exmem_reg.alu_result));
          break;
        case 0x5: // lhu (load halfword unsigned)
          memwb_reg.mem_data = (uint16_t)(*(uint16_t*)(memory_p + exmem_reg.alu_result));
          break;
        default:
          memwb_reg.mem_data = *(int32_t*)(memory_p + exmem_reg.alu_result);
          break;
      }
    } else {
      // Default to 32-bit load
      memwb_reg.mem_data = *(int32_t*)(memory_p + exmem_reg.alu_result);
    }
    memwb_reg.mem_to_reg = true;
  } else if (exmem_reg.memWrite) {
    // Store instruction - write to memory
    *(int32_t*)(memory_p + exmem_reg.alu_result) = exmem_reg.store_val;
    memwb_reg.mem_to_reg = false;
  } else {
    // Non-memory instruction
    memwb_reg.mem_to_reg = false;
  }
  
  // Handle branch logic in MEM stage
  if (exmem_reg.branch_taken) {
    branch_counter++;
    // Set branch target address for PC update
    pwires_p->pcsrc = true;
    
    // Calculate target address based on instruction type
    if (exmem_reg.is_jalr) { // JALR
      pwires_p->pc_src1 = exmem_reg.jalr_base + exmem_reg.branch_target - exmem_reg.instr_addr;
    } else { // JAL or conditional branches
      pwires_p->pc_src1 = exmem_reg.branch_target;
    }
  } else {
    pwires_p->pcsrc = false;
  }
  
  return memwb_reg;
}

/**
 * STAGE  : stage_writeback
 * output : nothing - The state of the register file may be changed
 **/ 
void stage_writeback(memwb_reg_t memwb_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  // Write back to register file if instruction writes to registers
  if (memwb_reg.regWrite && memwb_reg.rd != 0) {
    // Select between memory data and ALU result
    int32_t write_data = memwb_reg.mem_to_reg ? memwb_reg.mem_data : memwb_reg.alu_result;
    regfile_p->R[memwb_reg.rd] = write_data;
  }
}

///////////////////////////////////////////////////////////////////////////////

/** 
 * excite the pipeline with one clock cycle
 **/
void cycle_pipeline(regfile_t* regfile_p, Byte* memory_p, Cache* cache_p, pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, bool* ecall_exit)
{
  // Initialize hazard detection and forwarding signals
  pwires_p->stall = false;
  pwires_p->flush = false;
  pwires_p->forward_rs1_ex = false;
  pwires_p->forward_rs2_ex = false;
  pwires_p->forward_rs1_mem = false;
  pwires_p->forward_rs2_mem = false;
  
  // Detect hazards and generate forwarding signals BEFORE processing stages
  detect_hazard(pregs_p, pwires_p, regfile_p);
  gen_forward(pregs_p, pwires_p);
  
  // Update PC based on branch decisions from previous cycle
  if (pwires_p->pcsrc) {
    regfile_p->PC = pwires_p->pc_src1;
    // Clear pipeline registers for control hazard
    pregs_p->ifid_preg.inp = (ifid_reg_t){0};
    pregs_p->idex_preg.inp = (idex_reg_t){0};
    pregs_p->exmem_preg.inp = (exmem_reg_t){0};
    
    // Set NOP instructions for cleared registers
    pregs_p->ifid_preg.inp.instr.bits = 0x00000013;
    pregs_p->idex_preg.inp.instr.bits = 0x00000013;
    pregs_p->exmem_preg.inp.instr.bits = 0x00000013;
    
    printf("[CPL]: Pipeline Flushed\n");
  } else if (!pwires_p->stall) {
    // Normal PC increment - use next_pc from previous fetch
    if (pregs_p->ifid_preg.out.next_pc != 0) {
      regfile_p->PC = pregs_p->ifid_preg.out.next_pc;
    } else if (pregs_p->ifid_preg.out.instr_addr != 0) {
      // If we have a valid instruction address but no next_pc, advance by 4
      regfile_p->PC = pregs_p->ifid_preg.out.instr_addr + 4;
    }
  }
  // If stall is true, don't update PC - same instruction will be fetched again

  // process each stage

  /* Output               |    Stage      |       Inputs  */
  if (!pwires_p->stall) {
    pregs_p->ifid_preg.inp  = stage_fetch     (pwires_p, regfile_p, memory_p);
  } else {
    // Keep the same instruction in IFID when stalling
    pregs_p->ifid_preg.inp = pregs_p->ifid_preg.out;
  }
  
  if (!pwires_p->stall) {
    pregs_p->idex_preg.inp  = stage_decode    (pregs_p->ifid_preg.out, pwires_p, regfile_p);
  } else {
    // Insert bubble in IDEX stage when stalling
    pregs_p->idex_preg.inp = (idex_reg_t){0};
    pregs_p->idex_preg.inp.instr.bits = 0x00000013; // NOP instruction
  }

  pregs_p->exmem_preg.inp = stage_execute   (pregs_p->idex_preg.out, pwires_p);

  pregs_p->memwb_preg.inp = stage_mem       (pregs_p->exmem_preg.out, pwires_p, memory_p, cache_p);

  // Writeback should use the old memwb register values (from previous cycle)
  stage_writeback (pregs_p->memwb_preg.out, pwires_p, regfile_p);
  
  // Print debug information for current cycle after processing stages but before updating registers
  #ifdef DEBUG_CYCLE
  printf("v==============Cycle Counter = %5ld==============v\n\n", total_cycle_counter);
  
  // Print debug information for each pipeline stage with safe access
  printf("[IF ]: Instruction [%08x]@[%08x]: ", 
         pregs_p->ifid_preg.inp.instr.bits, 
         pregs_p->ifid_preg.inp.instr_addr);
  if (pregs_p->ifid_preg.inp.instr.bits != 0) {
    decode_instruction(pregs_p->ifid_preg.inp.instr.bits);
  } else {
    printf("\n");
  }
  
  printf("[ID ]: Instruction [%08x]@[%08x]: ", 
         pregs_p->idex_preg.inp.instr.bits, 
         pregs_p->idex_preg.inp.instr_addr);
  if (pregs_p->idex_preg.inp.instr.bits != 0) {
    decode_instruction(pregs_p->idex_preg.inp.instr.bits);
  } else {
    printf("\n");
  }
  
  printf("[EX ]: Instruction [%08x]@[%08x]: ", 
         pregs_p->exmem_preg.inp.instr.bits, 
         pregs_p->exmem_preg.inp.instr_addr);
  if (pregs_p->exmem_preg.inp.instr.bits != 0) {
    decode_instruction(pregs_p->exmem_preg.inp.instr.bits);
  } else {
    printf("\n");
  }
  
  printf("[MEM]: Instruction [%08x]@[%08x]: ", 
         pregs_p->memwb_preg.inp.instr.bits, 
         pregs_p->memwb_preg.inp.instr_addr);
  if (pregs_p->memwb_preg.inp.instr.bits != 0) {
    decode_instruction(pregs_p->memwb_preg.inp.instr.bits);
  } else {
    printf("\n");
  }
  
  printf("[WB ]: Instruction [%08x]@[%08x]: ", 
         pregs_p->memwb_preg.out.instr.bits, 
         pregs_p->memwb_preg.out.instr_addr);
  if (pregs_p->memwb_preg.out.instr.bits != 0) {
    decode_instruction(pregs_p->memwb_preg.out.instr.bits);
  } else {
    printf("\n");
  }
  #endif

  // increment the cycle
  total_cycle_counter++;

  // update all the output registers for the next cycle from the input registers in the current cycle
  pregs_p->ifid_preg.out  = pregs_p->ifid_preg.inp;
  pregs_p->idex_preg.out  = pregs_p->idex_preg.inp;
  pregs_p->exmem_preg.out = pregs_p->exmem_preg.inp;
  pregs_p->memwb_preg.out = pregs_p->memwb_preg.inp;

  /////////////////// NO CHANGES BELOW THIS ARE REQUIRED //////////////////////

  #ifdef DEBUG_REG_TRACE
  print_register_trace(regfile_p);
  #endif

  /**
   * check ecall condition
   * To do this, the value stored in R[10] (a0 or x10) should be 10.
   * Hence, the ecall condition is checked by the existence of following
   * two instructions in sequence:
   * 1. <instr>  x10, <val1>, <val2> 
   * 2. ecall
   * 
   * The first instruction must write the value 10 to x10.
   * The second instruction is the ecall (opcode: 0x73)
   * 
   * The condition checks whether the R[10] value is 10 when the
   * `memwb_reg.instr.opcode` == 0x73 (to propagate the ecall)
   * 
   * If more functionality on ecall needs to be added, it can be done
   * by adding more conditions on the value of R[10]
   */
  // Check ecall condition
  if( (pregs_p->memwb_preg.out.instr.bits == 0x00000073) &&
      (regfile_p->R[10] == 10) )
  {
    *(ecall_exit) = true;
  }
}

