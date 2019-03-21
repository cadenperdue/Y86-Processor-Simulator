//class to perform the combinational logic of
//the Fetch stage
class FetchStage: public Stage
{
   private:
      void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t rA, uint64_t rB,
                     uint64_t valC, uint64_t valP);
      bool F_stall;
      bool D_stall;
      bool D_bubble;
   public:      
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      uint64_t selectPC(F * freg, M * mreg, W * wreg);
      bool needRegIds(uint64_t icode);
      bool needValC(uint64_t icode);
      uint64_t predictPC(uint64_t icode, uint64_t valC, uint64_t valP);
      uint64_t PCincrement(uint64_t f_pc, bool regIds, bool valC);
      uint64_t getRegIds(uint64_t f_pc);
      uint64_t buildValC(uint64_t f_pc, bool needReg);
      bool instr_valid(uint64_t icode);
      uint64_t f_stat(bool mem_error, uint64_t icode);
      uint64_t f_icode(bool mem_error, M * mreg);
      uint64_t f_ifun(bool mem_error, uint64_t ifun);
      bool getF_stall (D * dreg, E * ereg, M * mreg, Stage ** stages);
      bool getD_stall (E * ereg, Stage ** stages);
      void calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages);
      bool calculateD_bubble(D * dreg, E * ereg, M * mreg, Stage ** stages);
      void bubbleD(PipeReg ** pregs);
      void normalD(PipeReg ** pregs);
};
