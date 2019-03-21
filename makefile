CC = g++
CFLAGS = -g -Wall -std=c++0x 
OBJ = yess.o Tools.o RegisterFile.o Loader.o ConditionCodes.o Memory.o Simulate.o\
	PipeRegField.o PipeReg.o D.o E.o F.o M.o W.o FetchStage.o DecodeStage.o ExecuteStage.o\
	MemoryStage.o WritebackStage.o\

.C.o:
	$(CC) $(CFLAGS) -c  $< -o $@

yess: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o yess

yess.o: Debug.h Memory.h Loader.h RegisterFile.h ConditionCodes.h PipeReg.h Stage.h Simulate.h 

Loader.o: Loader.C Loader.h Memory.h

Memory.o: Memory.h Tools.h

RegisterFile.o: RegisterFile.h Tools.h

Tools.o: Tools.h

ConditionCodes.o: ConditionCodes.h Tools.h

Simulate.o: PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h ExecuteStage.h\
			 MemoryStage.h DecodeStage.h FetchStage.h WritebackStage.h\
			 Simulate.h RegisterFile.h ConditionCodes.h

PipeRegField.o: PipeRegField.h

PipeReg.o: PipeReg.h

D.o: Instructions.h RegisterFile.h PipeReg.h PipeRegField.h D.h Status.h 

E.o: Instructions.h RegisterFile.h PipeReg.h PipeRegField.h E.h Status.h 

F.o: PipeRegField.h PipeReg.h F.h

M.o: Instructions.h RegisterFile.h PipeReg.h PipeRegField.h M.h Status.h

W.o: Instructions.h RegisterFile.h PipeReg.h PipeRegField.h W.h Status.h

FetchStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h\
				Stage.h FetchStage.h DecodeStage.h ExecuteStage.h Status.h\
				Debug.h Instructions.h Memory.h Tools.h

DecodeStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h W.h\
				 Stage.h DecodeStage.h Status.h Debug.h

ExecuteStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h W.h\
				 Stage.h ExecuteStage.h Status.h Debug.h Instructions.h\
				 Tools.h ConditionCodes.h

MemoryStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h W.h\
				Stage.h MemoryStage.h Status.h Debug.h Instructions.h\
				Memory.h

WritebackStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h W.h\
				 Stage.h WritebackStage.h Status.h Debug.h

clean:
	rm -f $(OBJ)

run:
	make clean
	make yess

submit:
	make clean
	submit can 3481-yess *.C *.h makefile 
