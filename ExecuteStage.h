class ExecuteStage: public Stage
{
   private:
      void setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, uint64_t valE, 
	uint64_t valA, uint64_t dstE, uint64_t dstM);
      uint64_t aluA(E * ereg);
      uint64_t aluB(E * ereg);
      uint64_t alufun(E * ereg);
      bool set_cc(E * ereg, W * wreg, M * mreg, Stage ** stages);
      uint64_t e_dstE(E * ereg, uint64_t cnd);
      void ccCircuit(E * ereg, W * wreg, M * mreg, Stage ** stages);
      uint64_t alu(E * ereg);
      uint64_t getCond(uint64_t icode, uint64_t ifun);
      uint64_t valE;
      uint64_t dstE;
      uint64_t Cnd;
      bool M_bubble;
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      uint64_t gete_dstE();
      uint64_t gete_valE();
      bool calculateControlSignals(Stage ** stages, M * mreg, W * wreg);
      uint64_t getCnd();
};
