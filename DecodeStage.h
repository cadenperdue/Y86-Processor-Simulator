class DecodeStage: public Stage
{
   private:
      void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t valC, uint64_t valA, uint64_t valB,
                     uint64_t dstE, uint64_t dstM, uint64_t srcA, uint64_t srcB);
      uint64_t srcA;
      uint64_t scrB;
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      uint64_t getSrcA(D * dreg);
      uint64_t getSrcB(D * dreg);
      uint64_t getDstE(D * dreg);
      uint64_t getDstM(D * dreg);
      uint64_t getValA(uint64_t srcA, Stage ** stages, M * mreg, W * wreg, D * dreg);
      uint64_t getValB(uint64_t srcB, Stage ** stages, M * mreg, W * wreg);
      uint64_t getd_srcA();
      uint64_t getd_srcB();
      bool calculateControlSignals(E * ereg, Stage ** stages);
};
