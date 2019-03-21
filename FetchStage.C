#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "E.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "FetchStage.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"

using namespace std;

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   //Initialize registers and values
   F * freg = (F *) pregs[FREG];
   D * dreg = (D *) pregs[DREG];
   E * ereg = (E *) pregs[EREG];
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];

   Memory * mem = Memory::getInstance();

   uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
   uint64_t rA = RNONE, rB = RNONE, stat = SAOK;
   bool needReg, needVal;
   bool mem_error = false;
   
   //Calculate values needed for F stage
   
   f_pc = selectPC(freg, mreg, wreg);
   uint8_t instructionByte = mem->getByte(f_pc, mem_error);
   icode = Tools::getBits(instructionByte, 4, 7);
   ifun = Tools::getBits(instructionByte, 0, 3);
   needReg = needRegIds(icode);
   needVal = needValC(icode);
   //stat = f_stat(mem_error, icode);
   valP = PCincrement(f_pc, needReg, needVal);
   
   if(needReg)
   {
       uint64_t regByte = getRegIds(f_pc + 1);
       rA = Tools::getBits(regByte, 4, 7);        
       rB = Tools::getBits(regByte, 0, 3);
   }
   if(needVal)
   {
       valC = buildValC(f_pc, needReg);
   }
   
   uint64_t pc = predictPC(icode, valC, valP);
   //freg->getpredPC()->setInput(pc);
   
   if(mem_error)
   {
       icode = INOP;
       ifun = FNONE;
   }
   stat = f_stat(mem_error, icode);
   calculateControlSignals(dreg, ereg, mreg, stages); 
   freg->getpredPC()->setInput(pc); 
   //provide the input values for the D register
   setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
   return false;
   
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
   F * freg = (F *) pregs[FREG];

   if (!F_stall)
       freg->getpredPC()->normal();
   else
       freg->getpredPC()->stall();
   
   if(D_bubble)
       bubbleD(pregs);
   else
       normalD(pregs);
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
*/
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t rA, uint64_t rB,
                           uint64_t valC, uint64_t valP)
{
   
   dreg->getstat()->setInput(stat);
   dreg->geticode()->setInput(icode);
   dreg->getifun()->setInput(ifun);
   dreg->getrA()->setInput(rA);
   dreg->getrB()->setInput(rB);
   dreg->getvalC()->setInput(valC);
   dreg->getvalP()->setInput(valP);
   
}

/* selectPC
 * uses the icode from the F, M, and W registers to calculate the PC
 *
 * @param: freg - pointer to the F register instance
 * @param: mreg - pointer to the M register instance
 * @param: wreg - pointer to the W register instance
*/
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg)
{
    uint64_t f_pc;
    if(mreg->geticode()->getOutput() == IJXX && !mreg->getCnd()->getOutput())
        f_pc = mreg->getvalA()->getOutput();
    else if (wreg->geticode()->getOutput() == IRET)
        f_pc = wreg->getvalM()->getOutput();
    else
        f_pc = freg->getpredPC()->getOutput();
    return f_pc;
         
}

/* needRegIds
 * uses the icode to determine if register IDs are needed
 *
 * @param: icode - the icode value from the F stage
*/
bool FetchStage::needRegIds(uint64_t icode)
{
    if (icode == IRRMOVQ || icode == IOPQ || icode == IPUSHQ || icode == IPOPQ || icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ)
        return true;
    else 
        return false;
}

/* needValC
 * uses the icode to determine if ValC is needed
 *
 * @param: icode - the icode value from the F stage
*/
bool FetchStage::needValC(uint64_t icode)
{
    if(icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ || icode == IJXX || icode == ICALL)
        return true;
    else 
        return false;
}

/* predictPC
 * uses the icode to predict what PC will be
 *
 * @param: icode - the icode value from the F stage
 * @param: valC - the valC value from the F stage
 * @param: valP - the valP value from the F stage
*/
uint64_t FetchStage::predictPC(uint64_t icode, uint64_t valC, uint64_t valP)
{
    if(icode == IJXX || icode == ICALL)
        return valC;
    else 
        return valP;
}

/* PCincrement
 * uses f_pc, regIds, and valC to determine the increment needed fro the PC
 *
 * @param: f_pc - the f_pc value from the F stage
 * @param: regIds - the value returned from needRegIds
 * @param: valC - the value returned from needValC
*/
uint64_t FetchStage::PCincrement(uint64_t f_pc, bool regIds, bool valC)
{
    
    int pcInc;
    if(regIds && valC)
    {
        pcInc = 10 + f_pc;
    }
    else if(!regIds && !valC)
    {
        pcInc = 1 + f_pc; 
    }
    else if(regIds && !valC)
    {
        pcInc = 2 + f_pc;
    }
    else
    {
        pcInc = 9 + f_pc;
    }
    return pcInc;
    
}

/* getRegIds
 * uses f_pc to get register IDs
 *
 * @param: f_pc - the f_pc value from the F stage
*/
uint64_t FetchStage::getRegIds(uint64_t f_pc)
{
    Memory * mem = Memory::getInstance();
    uint64_t regByte;
    bool check = true;
    regByte = mem -> getByte(f_pc, check);
    return regByte;      
}

/* buildValC
 * uses f_pc and regIds to build the valC
 *
 * @param: f_pc - the f_pc value from the F stage
 * @param: regIds - the value returned from needRegIds
*/
uint64_t FetchStage::buildValC(uint64_t f_pc, bool regIds)
{
    //If register ID is true, add 2, else add 1
    if(regIds)
        f_pc = f_pc + 2;
    else
        f_pc ++;
    
    //Uses f_pc to create an array of bytes, then uses this array to build a long using Tools
    Memory * mem = Memory::getInstance();
    bool check = true;
    uint8_t theLong[8];
    for(int i = 0; i < 8; i++)
    {
        theLong[i] = mem -> getByte(f_pc, check);
        f_pc++;
    }
    uint64_t valC = Tools::buildLong(theLong);
    return valC;
}

/* instr_invalid
 * return true if icode is valid 
 *
 * @param: icode - icode of current instruction
 */ 
bool FetchStage::instr_valid(uint64_t icode)
{
    return (icode == INOP || icode == IHALT || icode == IRRMOVQ ||
        icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ || 
        icode == IOPQ || icode == IJXX || icode == ICALL ||
        icode == IRET || icode == IPUSHQ || icode == IPOPQ);
}

/* f_stat 
 * returns value to be stored in stat field
 *
 *
 */
uint64_t FetchStage::f_stat(bool mem_error, uint64_t icode)
{
   // uint64_t icode = freg->geticode()->getOutput();
    if (mem_error)
        return SADR;
    else if (!instr_valid(icode))
        return SINS;
    else if (icode == IHALT)
        return SHLT;
    else
        return SAOK;
}
uint64_t FetchStage::f_icode(bool mem_error, M * mreg)
{
    uint64_t icode = mreg->geticode()->getOutput();
    if(mem_error)
        return INOP;
    else 
        return icode;
}
uint64_t FetchStage::f_ifun(bool mem_error, uint64_t ifun)
{
   // uint64_t ifun = mreg->getifun()->getOutput();
    if (mem_error)
        return FNONE;
    else 
        return ifun;
}
bool FetchStage::getF_stall(D * dreg, E * ereg, M * mreg, Stage ** stages)
{
    DecodeStage * decStage = (DecodeStage *) stages[DSTAGE];
    uint64_t srcA = decStage->getd_srcA();
    uint64_t srcB = decStage->getd_srcB();
    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t D_icode = dreg->geticode()->getOutput();
    uint64_t M_icode = mreg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput();
    return ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == srcA || E_dstM == srcB)) ||
        (D_icode == IRET || E_icode == IRET || M_icode == IRET);

}
bool FetchStage::getD_stall(E * ereg, Stage ** stages)
{
    DecodeStage * decStage = (DecodeStage *) stages[DSTAGE];
    uint64_t srcA = decStage->getd_srcA();
    uint64_t srcB = decStage->getd_srcB();
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput();
    return ((icode == IMRMOVQ || icode == IPOPQ) && (E_dstM == srcA || E_dstM == srcB));
}

void FetchStage::calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages)
{
    //FetchStage * fetStage = (FetchStage *) stages[FSTAGE];
    F_stall = getF_stall(dreg, ereg, mreg, stages);
    D_stall = getD_stall(ereg, stages);
    D_bubble = calculateD_bubble(dreg, ereg, mreg, stages);
}

bool FetchStage::calculateD_bubble(D * dreg, E * ereg, M * mreg, Stage ** stages) 
{
    ExecuteStage * exStage = (ExecuteStage *) stages[ESTAGE];
    DecodeStage * decStage = (DecodeStage *) stages[DSTAGE];
    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t Cnd = exStage->getCnd();
    uint64_t D_icode = dreg->geticode()->getOutput();
    uint64_t M_icode = mreg->geticode()->getOutput();
    uint64_t srcA = decStage->getd_srcA();
    uint64_t srcB = decStage->getd_srcB();
    uint64_t dstM = ereg->getdstM()->getOutput();

    return ((E_icode == IJXX && !Cnd ) ||
           (!((E_icode == IMRMOVQ || E_icode == IPOPQ) && (dstM == srcA || dstM == srcB)) &&
           (D_icode == IRET || E_icode == IRET || M_icode == IRET)));
}
void FetchStage::normalD(PipeReg ** pregs) {
   //Initializes registers
   D * dreg = (D *) pregs[DREG];
   //Normalizes values in the F and D register
   if(!D_stall)
   {
        dreg->getstat()->normal();
        dreg->geticode()->normal();
        dreg->getifun()->normal();
        dreg->getrA()->normal();
        dreg->getrB()->normal();
        dreg->getvalC()->normal();
        dreg->getvalP()->normal();
   }
   else {
       dreg->getstat()->stall();
       dreg->geticode()->stall();
       dreg->getifun()->stall();
       dreg->getrA()->stall();
       dreg->getrB()->stall();
       dreg->getvalC()->stall();
       dreg->getvalP()->stall();
   }
}


void FetchStage::bubbleD(PipeReg ** pregs) {
   //Initializes registers
   D * dreg = (D *) pregs[DREG];

   //Normalizes values in the F and D registers
   dreg->getstat()->bubble(SAOK);
   dreg->geticode()->bubble(INOP);
   dreg->getifun()->bubble();
   dreg->getrA()->bubble(RNONE);
   dreg->getrB()->bubble(RNONE);
   dreg->getvalC()->bubble();
   dreg->getvalP()->bubble();
}
