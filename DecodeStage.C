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
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"

using namespace std;
uint64_t d_srcA;
uint64_t d_srcB;
bool E_bubble;
/*
 * doClockLow:
 * Performs the Decode stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   //Initializing registers and values
   E * ereg = (E *) pregs[EREG];
   D * dreg = (D *) pregs[DREG];
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];
   uint64_t valA = 0, valB = 0,icode = 0, ifun = 0, valC = 0;
   uint64_t dstE = RNONE, dstM = RNONE, stat = SAOK;
   
   //Passing through values
   valC = dreg->getvalC()->getOutput(); 
   icode = dreg->geticode()->getOutput();
   ifun = dreg->getifun()->getOutput();
   stat = dreg->getstat()->getOutput();

   //Providing the input values for the E register
   d_srcA = getSrcA(dreg);
   d_srcB = getSrcB(dreg);
   dstE = getDstE(dreg);
   dstM = getDstM(dreg);
   valA = getValA(d_srcA, stages, mreg, wreg, dreg);
   valB = getValB(d_srcB, stages, mreg, wreg);
   E_bubble = calculateControlSignals(ereg, stages); 
   DecodeStage::setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM,
                 d_srcA, d_srcB);
   
   return false;

}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register fintances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
   //Initializing register
   E * ereg = (E *) pregs[EREG];

   //Normalizes the values in the E register
   if (!E_bubble) {
       ereg->getstat()->normal();
       ereg->geticode()->normal();
       ereg->getifun()->normal();
       ereg->getsrcA()->normal();
       ereg->getsrcB()->normal();
       ereg->getvalC()->normal();
       ereg->getvalA()->normal();
       ereg->getvalB()->normal();
       ereg->getdstE()->normal();
       ereg->getdstM()->normal();   
   }
   else {
       ereg->getstat()->bubble(SAOK);
       ereg->geticode()->bubble(INOP);
       ereg->getifun()->bubble();
       ereg->getsrcA()->bubble(RNONE);
       ereg->getsrcB()->bubble(RNONE);
       ereg->getvalC()->bubble();
       ereg->getvalA()->bubble();
       ereg->getvalB()->bubble();
       ereg->getdstE()->bubble(RNONE);
       ereg->getdstM()->bubble(RNONE);
   }
}
/* setEInput
 * provides the input to potentially be stored in the E register
 * during doClockHigh
 *
 * @param: ereg - pointer to the E register instance
 * @param: stat - value to be stored in the stat pipeline register within E
 * @param: icode - value to be stored in the icode pipeline register within E
 * @param: ifun - value to be stored in the ifun pipeline register within E
 * @param: rA - value to be stored in the rA pipeline register within E
 * @param: rB - value to be stored in the rB pipeline register within E
 * @param: valC - value to be stored in the valC pipeline register within E
 * @param: valP - value to be stored in the valP pipeline register within E
*/
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t valC, uint64_t valA,
                           uint64_t valB, uint64_t dstE, uint64_t dstM,
                           uint64_t srcA, uint64_t srcB)
{
   //Sets the values in the E register based on input in DoClockLow   
   ereg->getstat()->setInput(stat);
   ereg->geticode()->setInput(icode);
   ereg->getifun()->setInput(ifun);
   ereg->getvalC()->setInput(valC);
   ereg->getvalA()->setInput(valA);
   ereg->getvalB()->setInput(valB);
   ereg->getdstE()->setInput(dstE);
   ereg->getdstM()->setInput(dstM);
   ereg->getsrcA()->setInput(srcA);
   ereg->getsrcB()->setInput(srcB);   
}

/* getSrcA
 * uses the icode from the D register to calculate the value of SrcA
 *
 * @param: dreg - pointer to the D register instance
*/
uint64_t DecodeStage::getSrcA(D * dreg) {
    uint64_t icode = dreg->geticode()->getOutput();
    if (icode == IRRMOVQ || icode == IRMMOVQ || icode == IOPQ || icode == IPUSHQ)
        return dreg->getrA()->getOutput();
    else if (icode == IPOPQ || icode == IRET)
        return RSP;
    else
        return RNONE;
}

/* getSrcB
 * uses the icode from the D register to calculate the value of SrcB
 *
 * @param: dreg - pointer to the D register instance
*/
uint64_t DecodeStage::getSrcB(D * dreg) {
    uint64_t icode = dreg->geticode()->getOutput();
    if (icode == IOPQ || icode == IRMMOVQ || icode == IMRMOVQ)
        return dreg->getrB()->getOutput();
    else if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET)
        return RSP;
    else
        return RNONE;
}

/* getDstE
 * uses the icode from the D register to calculate the value of DstE
 *
 * @param: dreg - pointer to the D register instance
*/
uint64_t DecodeStage::getDstE(D * dreg) {
    uint64_t icode = dreg->geticode()->getOutput();
    if (icode == IRRMOVQ || icode == IIRMOVQ || icode == IOPQ)
        return dreg->getrB()->getOutput();
    else if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET)
        return RSP;
    else
        return RNONE;
}

/* getDstM
 * uses the icode from the D register to calculate the value of DstM
 *
 * @param: dreg - pointer to the D register instance
*/
uint64_t DecodeStage::getDstM(D * dreg) {
    uint64_t icode = dreg->geticode()->getOutput();
    if (icode == IMRMOVQ || icode == IPOPQ)
        return dreg->getrA()->getOutput();
    else
        return RNONE;
}

/* getValA
 * uses the icode from the D register and srcA to calculate the value of ValA
 *
 * @param: srcA - value passed in from the D stage
 * @param: stages - a pointer to the stages object
 * @param: mreg - pointer to the M register instance
 * @param: wreg - pointer to the W register instance
 * @param: dreg - pointer to the D register instance
*/
uint64_t DecodeStage::getValA(uint64_t srcA, Stage ** stages, M * mreg, W * wreg, D * dreg)
{
    RegisterFile * reg = RegisterFile::getInstance();
    bool error = false;
    ExecuteStage * exStage = (ExecuteStage *) stages[ESTAGE];
    MemoryStage * memStage = (MemoryStage *) stages[MSTAGE];
    uint64_t icode = dreg->geticode()->getOutput();
    if (icode == ICALL || icode == IJXX)
        return dreg->getvalP()->getOutput();
    else if(srcA == RNONE)
        return 0;
    else if (srcA == exStage->gete_dstE())
        return exStage->gete_valE(); 
    else if (srcA == mreg->getdstM()->getOutput())
        return memStage->getm_valM();
    else if (srcA == mreg->getdstE()->getOutput())
        return mreg->getvalE()->getOutput();
    else if (srcA == wreg->getdstM()->getOutput())
        return wreg->getvalM()->getOutput();
    else if (srcA == wreg->getdstE()->getOutput())
        return wreg->getvalE()->getOutput();
    else
        return reg->readRegister(srcA, error);
}

/* getValB
 * uses the icode from the D register and srcB to calculate the value of ValB
 * 
 * @param: srcB - value passed in from the D stage
 * @param: stages - a pointer to the stages object
 * @param: mreg - pointer to the M register instance
 * @param: wreg - pointer to the W register instance
 * @param: dreg - pointer to the D register instance
*/ 
uint64_t DecodeStage::getValB(uint64_t srcB, Stage ** stages, M * mreg, W * wreg)
{
    RegisterFile * reg = RegisterFile::getInstance();
    bool error = false;
    ExecuteStage * exStage = (ExecuteStage *) stages[ESTAGE];
    MemoryStage * memStage = (MemoryStage *) stages[MSTAGE];
    if(srcB == RNONE)
        return 0;
    if (srcB == exStage->gete_dstE())
        return exStage->gete_valE();
    else if (srcB == mreg->getdstE()->getOutput())
        return mreg->getvalE()->getOutput();
    else if (srcB == mreg->getdstM()->getOutput())
        return memStage->getm_valM();
    else if (srcB == mreg->getdstE()->getOutput())
        return mreg->getvalE()->getOutput();
    else if (srcB == wreg->getdstM()->getOutput())
        return wreg->getvalM()->getOutput();
    else if (srcB == wreg->getdstE()->getOutput())
        return wreg->getvalE()->getOutput();
    else
        return reg -> readRegister(srcB, error);
}

uint64_t DecodeStage::getd_srcA()
{
    return d_srcA;
}
uint64_t DecodeStage::getd_srcB()
{
    return d_srcB;
}
bool DecodeStage::calculateControlSignals(E * ereg, Stage ** stages) {
    ExecuteStage * exStage = (ExecuteStage *) stages[ESTAGE];
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t dstM = ereg->getdstM()->getOutput();
    uint64_t Cnd = exStage->getCnd();
    return (icode == IJXX && !Cnd) || 
        ((icode == IMRMOVQ || icode == IPOPQ) && (dstM == d_srcA || dstM == d_srcB));
}
