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
#include "WritebackStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"


/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   //Initializes W register and values
   W * wreg = (W *) pregs[WREG];
   //uint64_t icode = wreg->geticode()->getOutput();
   uint64_t stat = wreg->getstat()->getOutput();
   RegisterFile * reg = RegisterFile::getInstance();
   bool check = true;

   //Pushes values through to the W register
   uint64_t dstM = wreg->getdstM()->getOutput();
   uint64_t valM = wreg->getvalM()->getOutput();
   uint64_t valE = wreg->getvalE()->getOutput();
   uint64_t dstE = wreg->getdstE()->getOutput();
   
   //Calculates the values needed to write into the register
   reg -> writeRegister(valE, dstE, check);
   reg -> writeRegister(valM, dstM, check);
   
   //Determines whether to halt or not
   if(stat != SAOK)
       return true;
   else
       return false;
    
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void WritebackStage::doClockHigh(PipeReg ** pregs)
{

}
