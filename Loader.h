class Loader
{
   private:
      bool loaded;   //set to true if a file is successfully loaded into memory
      ifstream inFile;  //input file handle
      bool checkYo (char* arr);
      void loadline(std::string line);
      int32_t convert (std:: string line, int start, int end);
      bool checkErrors(std::string line);
      bool checkComment(std::string line);
      bool checkColon(std::string line);
      bool checkPipe(std::string line);
      //bool commentLine(string line);
      bool checkAddress(string line);
      bool dataSpace(string line);
      
      //int getDataEnd (string line);
   public:
      Loader(int argc, char * argv[]);
      bool isLoaded();
};
