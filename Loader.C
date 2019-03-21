/**
 * Names: Caden Perdue and Thomas Lawing
 * Team: Team 21
*/
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>

using namespace std;

#include "Loader.h"
#include "Memory.h"

#define ADDRBEGIN 2   //starting column of 3 digit hex address 
#define ADDREND 4     //ending column of 3 digit hext address
#define DATABEGIN 7   //starting column of data bytes
#define PIPE 28   //location of the '|' character 

/**
 * Loader constructor
 * Opens the .yo file named in the command line arguments, reads the contents of the file
 * line by line and loads the program into memory.  If no file is given or the file doesn't
 * exist or the file doesn't end with a .yo extension or the .yo file contains errors then
 * loaded is set to false.  Otherwise loaded is set to true.
 *
 * @param argc is the number of command line arguments passed to the main; should
 *        be 2
 * @param argv[0] is the name of the executable
 *        argv[1] is the name of the .yo file
 */
Loader::Loader(int argc, char * argv[])
{  
    //Initialize values
    ifstream inFile;
    string s;
    loaded = false;
    int count = 0;
    
    //Check to see if file can open
    if (checkYo(argv[1])) 
    {
        inFile.open(argv[1]);
    }
    else 
    {
        return;
    }
    //read the file line by line
    while(getline(inFile, s)) 
    {
        count++;
        if (checkErrors(s)) 
        {
            if(s[0] == '0' && s[DATABEGIN] != ' ')
            {
                loadline(s);
            }
            
        }
        else 
        {
                cout << "Error on line " << dec << count << ": " << s << endl;
                return;
        }
    }
    //If everything checks out, the file is loaded
    loaded = true;
}

/**
 * isLoaded
 * returns the value of the loaded data member; loaded is set by the constructor
 *
 * @return value of loaded (true or false)
 */
bool Loader::isLoaded()
{
   return loaded;
}

/**
 * checkYo
 * uses the title of the file to determine if it can be opened
 *
 * @param: arr - the title of the file
 */
bool Loader::checkYo(char* arr) {
    //initializes file and values
    ifstream inFile;
    int x = 0;
    bool fl = 0;
    
    //checks the file name for errors
    if (arr !=  NULL)                   //Makes sure array isn't null
    {
        x = sizeof(arr);
       
        if (x <= 2)                     //Makes sure array is > 2 characters
	    {
            fl = false;   
        }
        
        else 
	    {
            if (arr[x - 3] == '.' && arr[x - 2] == 'y' && arr[x - 1] == 'o') 
	            fl = true;
            else                        //Checks that file ends in .yo
                fl = false;
        }

        inFile.open(arr);
        if(!inFile)                     //makes sure the file can open
            fl = false;
        else 
            fl = true;
    }
    else 
        fl = false;
    
    return fl;
}

/**
 * loadline
 * loads lines in from the file
 *
 * @param: line - the line being currently loaded
 */
void Loader::loadline(string line) 
{
    int32_t addr = convert(line, ADDRBEGIN, ADDREND);
    int data = DATABEGIN;
    bool error = false;
    Memory * memo = Memory::getInstance();
    int32_t byte = 0;
    
    while(line[data] != ' ') 
    {
        byte = convert(line, data, data + 1);
        data += 2;
        
        memo -> putByte(byte, addr, error);
        addr++;
    }
}

/**
 * convert
 * converts the substring into hexidecimal
 * 
 * @param: line - the line of the file currently being converted
 * @param: start - the starting point of the hex
 * @param: end - the ending point of the hex
 */
int32_t Loader::convert(string line, int start, int end) 
{    
    string s = line.substr(start, end - (start - 1));
    return stoul(s, NULL, 16);
}

/**
 * checkErrors
 * checks the line for format errors
 *
 * @param: line - the line being tested
 */
bool Loader::checkErrors(string line) 
{
    //initialize the boolean
    bool c = false;

    if(checkPipe(line))                     //Checks pipe location
    {
        if(line[0] == ' ')                  //Checks for comment if first char is ' '
        {
            c = checkComment(line);
        }
        if(checkColon(line))                //Checks colon location
        {
            if(line[DATABEGIN] != ' ')
            {
                c = dataSpace(line);        //Checks space loactions
            }
            else
            {
            
                c = checkAddress(line);     //Checks address formatting
            }
        }
    }
    return c;
}

/**
 * checkComment
 * checks comment formatting
 *
 * @param: line - the line being checked
 */
bool Loader::checkComment(string line)
{
    int i = 0;
    while(i < PIPE)
    {
        if(line.at(i) != ' ')
            return false;
        i++;
    }
    return true;
}

/**
 * checkColon
 * checks colon location
 *
 * @param: line - the line being checked
 */
bool Loader::checkColon(string line) 
{
    if(line[5] == ':' && line[6] == ' ')
        return true;
    else
        return false;
}

/**
 * checkPipe
 * checks pipe location
 *
 * @param: line - the line being checked
 */
bool Loader::checkPipe(string line) 
{
    if(line[PIPE] != '|')
        return false;
    else
        return true;
}

/**
 * checkAddress
 * checks address
 *
 * @param: line - the line being checked
 */
bool Loader::checkAddress(string line) 
{
    //initializes boolean and counter
    bool check = false;
    int begin = DATABEGIN;

    //checks address
    if (line[0] == '0' && line[1] == 'x')           //makes sure the line begins with '0x'
    {
        if (isxdigit(line[ADDRBEGIN]) && isxdigit(line[ADDRBEGIN + 1]) && isxdigit(line[ADDREND]))  
        {
            check = true;                           //makes sure the address numbers are hex
        }
        if(line[DATABEGIN] == ' ')
        {
            while(begin < PIPE)                     //increments the counter until it hits the pipe
            {
                if (line[begin] != ' ')             //makes sure there are spaces before the pipe
                    return false;
                begin ++;
            }
            check = true;
        }
    }
    
    return check;
}

/**
 * dataSpace
 * checks data
 *
 * @param: line - the line being checked
 */
bool Loader::dataSpace(string line) 
{
    //initializes location integers and the counter
    int data = DATABEGIN;
    uint32_t byte = 0;
    static int lastAddress = -1;

    //checks data
    if(!checkAddress(line))                                 //checks for valid address first
    {
         return false;
    }
    int currAddress = convert(line, ADDRBEGIN, ADDREND);    //uses convert to create the current address

    if(currAddress <= lastAddress)                          //makes sure the address is incrementing correctly
    {
        return false;
    }

    while(line[data] != ' ')                                //increments the counter until there is a space
    {
        if ((!isxdigit(line[data])) || (!isxdigit(line[data + 1])))
        {
             return false;                                  //makes sure the input here is valid hexidecimal
        }
        byte++;
        data += 2;
    }
    lastAddress = currAddress + byte - 1;                   //incrememnts lastAddress
    if(lastAddress >= MEMSIZE)
    {
        return false;                                       //makes sure lastAddress is < maximum memory size
    }

    while(data < 27)                                        
    {
        if(line[data] != ' ')                               //makes sure there are properly located spaces after char 27
            return false;

        data++;
    }

    return true;
    
}
