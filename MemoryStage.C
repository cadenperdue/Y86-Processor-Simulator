#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"

/*
 * doClockLow:
 * Performs the Execute stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   //Initializes registers and values
   Memory * mem = Memory::getInstance();
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];
   uint64_t icode = 0, valE = 0;
   uint64_t dstE = RNONE, dstM = RNONE;
   valM = 0;
   bool mem_error = false; 
   //Pushes proper values through
   icode = mreg->geticode()->getOutput();
   stat = mreg->getstat()->getOutput();
   valE = mreg->getvalE()->getOutput();
   dstE = mreg->getdstE()->getOutput();
   dstM = mreg->getdstM()->getOutput();
   
   //Calculates remaining values
   uint64_t valA = mreg -> getvalA() -> getOutput();
   uint32_t addr = Addr(mreg);
   if(mem_read(mreg))
   {
        valM = mem->getLong(addr, mem_error);
   }
   if(mem_write(mreg))
   {
        mem->putLong(valA, addr, mem_error);
   }
   if (mem_error)
   {
       stat = SADR;
   }
   else
       stat = mreg->getstat()->getOutput();
   setWInput(wreg, stat, icode, valE, valM, dstE, dstM);
   return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void MemoryStage::doClockHigh(PipeReg ** pregs)
{
   //Initializes the W register instance
   W * wreg = (W *) pregs[WREG];

   //Normalizes values in the W register
   wreg->getstat()->normal();
   wreg->geticode()->normal();
   wreg->getvalM()->normal();
   wreg->getvalE()->normal();
   wreg->getdstE()->normal();
   wreg->getdstM()->normal();

}

/**
 * setWInput
 * uses parameters to set the W register values
 *
 * @param: wreg - pointer to the W register instance
 * @param: stat - stat value from the M stage
 * @param: icode - icode value from the M stage
 * @param: valE - valE value from the M stage
 * @param: valM - valM value from the M stage
 * @param: dstE - the dstE value from the M stage
 * @param: dstM - the dstM value from the M stage
 */
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode, 
                           uint64_t valE, uint64_t valM, uint64_t dstE,
                           uint64_t dstM)
{   
   wreg->getstat()->setInput(stat);
   wreg->geticode()->setInput(icode);
   wreg->getvalE()->setInput(valE);
   wreg->getvalM()->setInput(valM);
   wreg->getdstE()->setInput(dstE);
   wreg->getdstM()->setInput(dstM);   
}

/**
 * mem_read
 * uses the icode from the M register to determine if the memory needs to be read
 *
 * @param: mreg - pointer to the M register instance
 */
bool MemoryStage::mem_read(M * mreg)
{
    uint64_t icode = mreg->geticode()->getOutput();
    if(icode ==  IMRMOVQ || icode ==  IPOPQ || icode == IRET)
        return true;
    else
        return false;
}

/**
 * mem_write
 * uses the icode from the M register to determine if the memory needs to be written to
 *
 * @param: mreg - pointer to the M register instance
 */
bool MemoryStage::mem_write(M * mreg)
{
    uint64_t icode = mreg->geticode()->getOutput();
    if(icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL)
        return true;
    else
        return false;
}

/**
 * Addr
 * uses the icode to determine the address
 *
 * @param: mreg - pointer to the M register instance
 */
uint64_t MemoryStage::Addr(M * mreg)
{
    uint64_t icode = mreg->geticode()->getOutput();
    if(icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL || icode == IMRMOVQ)
        return mreg->getvalE()->getOutput();
    else if(icode == IPOPQ || icode == IRET)
        return mreg->getvalA()->getOutput();
    else return 0;
}

/**
 * getm_valM
 * returns m_valM
 */
uint64_t MemoryStage::getm_valM() {
    return valM;
}
/*
 * getm_stat
 * returns mstat
 */
uint64_t MemoryStage::getm_stat()
{
    return stat;
}

