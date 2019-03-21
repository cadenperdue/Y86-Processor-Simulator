#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Tools.h"
#include "ConditionCodes.h"
#include "MemoryStage.h"

/*
 * doClockLow:
 * Performs the Execute stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   //Initializes registers and values
   M * mreg = (M *) pregs[MREG];
   E * ereg = (E *) pregs[EREG];
   W * wreg = (W *) pregs[WREG];
   uint64_t icode = 0, valA = 0;
   uint64_t dstM = RNONE, stat = SAOK;
   
   //pushes values through the E register
   icode = ereg->geticode()->getOutput();
   stat = ereg->getstat()->getOutput();
   valE = ereg->getvalC()->getOutput();
   dstM = ereg->getdstM()->getOutput();
   valA = ereg->getvalA()->getOutput();
   
   //sets values based on y86 logic
   uint64_t ifun = ereg->getifun()->getOutput();
   valE = alu(ereg);
   ccCircuit(ereg, wreg, mreg, stages);
   Cnd = getCond(icode, ifun);
   dstE = e_dstE(ereg, Cnd);
   M_bubble = calculateControlSignals(stages, mreg, wreg);


   setMInput(mreg, stat, icode, Cnd, valE, valA, dstE, dstM);
   return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
   //initializes the M object
   M * mreg = (M *) pregs[MREG];
    
   if (!M_bubble)
   {
        //normalizes mreg values
        mreg->getstat()->normal();
        mreg->geticode()->normal();
        mreg->getCnd()->normal();
        mreg->getvalA()->normal();
        mreg->getvalE()->normal();
        mreg->getdstE()->normal();
        mreg->getdstM()->normal();
   }
   else 
   {
        mreg->getstat()->bubble(SAOK);
	    mreg->geticode()->bubble(INOP);
	    mreg->getCnd()->bubble();
	    mreg->getvalE()->bubble();
	    mreg->getvalA()->bubble();
	    mreg->getdstE()->bubble(RNONE);
	    mreg->getdstM()->bubble(RNONE);
   }

}

/* setMInput
 * uses paramters to set the input variables of the M register
 *
 * @param: mreg - pointer to the M register instance
 * @param: stat - value passed to stat in the M register
 * @param: icode - value passed to icode in the M register
 * @param: Cnd - value passed to Cnd in the M register
 * @param: valE - value passed to valE in the M register
 * @param: valA - value passed to valA in the M register
 * @param: dstE - value passed to dstE in the M register
 * @param: dstM - value passed to dstM in the M register
*/
void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode, 
                           uint64_t Cnd, uint64_t valE,
                           uint64_t valA, uint64_t dstE,
                           uint64_t dstM)
{  
   mreg->getstat()->setInput(stat);
   mreg->geticode()->setInput(icode);
   mreg->getCnd()->setInput(Cnd);
   mreg->getvalA()->setInput(valA);
   mreg->getvalE()->setInput(valE);
   mreg->getdstE()->setInput(dstE);
   mreg->getdstM()->setInput(dstM);   
}

/* aluA
 * uses the icode from the E register to set aluA
 *
 * @param: ereg - pointer to the E register instance
*/
uint64_t ExecuteStage::aluA(E * ereg)
{
    uint64_t icode = ereg->geticode()->getOutput();
    if(icode == IRRMOVQ || icode == IOPQ)
        return ereg->getvalA()->getOutput();
    else if(icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ)
        return ereg->getvalC()->getOutput();
    else if(icode == ICALL || icode == IPUSHQ)
        return -8;
    else if(icode == IRET || icode == IPOPQ)
        return 8;    
    else 
        return 0;
}

/* aluB
 * uses the icode from the E register to set aluB
 *
 * @param: ereg - pointer to the E register instance
*/
uint64_t ExecuteStage::aluB(E * ereg)
{
    uint64_t icode = ereg->geticode()->getOutput();
    if(icode == IRMMOVQ || icode == IMRMOVQ || icode == IOPQ || icode == ICALL 
            || icode == IPUSHQ || icode == IRET || icode == IPOPQ)
        return ereg->getvalB()->getOutput();
    else if (icode == IRRMOVQ || icode == IIRMOVQ)
        return 0;
    else
        return 0;
}

/* alufun
 * uses the icode from the E register to set alufun
 *
 * @param: ereg - pointer to the E register instance
*/
uint64_t ExecuteStage::alufun(E * ereg)
{
    uint64_t icode = ereg -> geticode()->getOutput();
    if(icode == IOPQ)
        return ereg->getifun()->getOutput();
    else
        return ADDQ;
}

/* set_cc
 * uses the icode from the E register to set_cc
 *
 * @param: ereg - pointer to the E register instance
 * @param: wreg - pointer to W register instance 
*/
bool ExecuteStage::set_cc(E * ereg, W * wreg, M * mreg, Stage ** stages)
{
    MemoryStage * memStage = (MemoryStage *) stages[MSTAGE];
    uint64_t icode = ereg->geticode()->getOutput();
    //uint64_t m_stat = mreg->getstat()->getOutput();
    uint64_t m_stat = memStage->getm_stat();
    uint64_t w_stat = wreg->getstat()->getOutput();
    return ((icode == IOPQ) && !(m_stat == SADR || m_stat == SINS ||
            m_stat == SHLT) && !(w_stat == SADR || w_stat == SINS ||
            w_stat == SHLT));
}

/* e_dstE
 * uses the icode from the E register and cnd to set e_dstE
 *
 * @param: ereg - pointer to the E register instance
 * @param: cnd - cnd value passed in from the E stage
*/
uint64_t ExecuteStage::e_dstE(E * ereg, uint64_t cnd)
{
    uint64_t icode = ereg->geticode()->getOutput();
 
    if(icode == IRRMOVQ && !cnd)
        return RNONE;
    else
        return ereg->getdstE()->getOutput();
}

/* ccCircuit
 * uses values from the E register to set condition codes
 *
 * @param: ereg - pointer to the E register instance
*/
void ExecuteStage::ccCircuit(E * ereg, W * wreg, M * mreg, Stage ** stages)
{
    uint64_t fun = alufun(ereg);
    uint64_t num = alu(ereg);
    uint64_t val1 = aluA(ereg);
    uint64_t val2 = aluB(ereg);
    bool check = false;
    ConditionCodes * cc = ConditionCodes::getInstance();
    if(set_cc(ereg, wreg, mreg, stages))
    {
        if(fun == ADDQ)
        {
            if(Tools::addOverflow(val1, val2))
                cc -> setConditionCode(1, OF, check);
	        else
		        cc -> setConditionCode(0, OF, check);
            
        }
        else if(fun == SUBQ)
        {
           if(Tools::subOverflow(val1, val2))
                cc -> setConditionCode(1, OF, check);
	       else
		        cc -> setConditionCode(0, OF, check);
	    }
	    if(num == 0)
            cc -> setConditionCode(1, ZF, check);
	    else
		    cc -> setConditionCode(0, ZF, check);
        if(Tools::getBits(num, 63, 63))
            cc -> setConditionCode(1, SF, check);
	    else
		    cc -> setConditionCode(0, SF, check);
    }
    else 
        return;
}

/* alu
 * uses values from the E register to return the alu
 *
 * @param: ereg - pointer to the E register instance
*/
uint64_t ExecuteStage::alu(E * ereg)
{
    uint64_t val1 = aluA(ereg);
    uint64_t val2 = aluB(ereg);
    uint64_t fun = alufun(ereg);
    if(fun == ADDQ)
    {
        return val1 + val2;
    }
    else if(fun == SUBQ)
    {
        return val2 - val1;
    }
    else if(fun == ANDQ)
    {
        return val1 & val2;
    }
    else 
        return val1 ^ val2;
}

/* gete_dstE
 * retrieves the dstE value
*/
uint64_t ExecuteStage::gete_dstE() {
    return dstE;
}

/* gete_valE
 * retrieves the valE value
*/
uint64_t ExecuteStage::gete_valE() {
    return valE;
}

/* getCond
 * uses the icode and ifun to get condition codes
 *
 * @param: icode - the icode value from the E register
 * @param: ifun - the ifun value from the E register
*/
uint64_t ExecuteStage::getCond(uint64_t icode, uint64_t ifun)
{
    ConditionCodes * cc = ConditionCodes::getInstance();
    bool error = false;
    bool overflow =  cc -> getConditionCode(OF, error);
    bool zero = cc -> getConditionCode(ZF, error);
    bool sign = cc -> getConditionCode(SF, error);

    //return 0 if icode isn't ijxx or icmovxx
    if(icode != IJXX && icode != ICMOVXX)
        return 0;
    if (ifun == UNCOND)
        return 1;
    else if (ifun == LESSEQ)
        return ((sign ^ overflow) || zero);
    else if (ifun == LESS)
        return sign ^ overflow;
    else if (ifun == EQUAL)
        return zero;
    else if (ifun == NOTEQUAL)
        return !zero;
    else if (ifun == GREATEREQ)
        return (!(sign ^ overflow));
    else if (ifun == GREATER)
        return (!(sign ^ overflow) && !zero);
    else return 0;
}

bool ExecuteStage::calculateControlSignals(Stage ** stages, M * mreg, 
                                                            W * wreg)
{
    uint64_t wstat = wreg->getstat()->getOutput();
    MemoryStage * memStage = (MemoryStage *) stages[MSTAGE]; 
    uint64_t mstat = memStage -> getm_stat();
    return ((mstat == SADR || mstat == SINS || mstat == SHLT) ||
                (wstat == SADR || wstat == SINS || wstat == SHLT));
}

uint64_t ExecuteStage::getCnd() {
    return Cnd;
}
