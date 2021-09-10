#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include "main.h"

const std::string usageString = "\nError!\nUsage: TypoColorChange.exe input_file aX|color_modifier[ color_modifier[ color_modifier[ color_modifier]]]\n\ncolor_modifier: at least one cX|mX|yX|bX where X - positive integer\n";
const std::string inkPrefix = "/HDMInkCode /HDMInk";
const std::string inkCodePrefix = "/HDMInk";
const std::string targetLinePrefix = "/HDMZoneCoverageValue";
const std::string arrPrefix = "[0.0";
const std::string zero = "0.0";
const std::string arrSuffix = "]";
const std::string lineFinish = "def";

/*
// Usage: TypoColorChange.exe input_file aX|color_modifier[ color_modifier[ color_modifier[ color_modifier]]]

          color_modifier: at least one cX|mX|yX|bX where X - positive integer
*/
int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 6)
    {
        std::cerr << usageString;
        return 1;
    }

    ColorValueShift colorShift;
    for (int modNo = 2; modNo < argc; ++modNo)
    {
        if (strlen(argv[modNo]) < 2)
        {
            std::cerr << usageString;
            return 1;
        }

        int color = tolower(argv[modNo][0]);
        int shift = atoi(argv[modNo] + 1);
        if (color == 'a')
        {
            colorShift.black = colorShift.cyan = colorShift.magenta = colorShift.yellow = shift;
            break; // ignore other modifiers if we have 'a' modifier
        }
        else if (color == 'c')
        {
            colorShift.cyan = shift;
        }
        else if (color == 'm')
        {
            colorShift.magenta = shift;
        }
        else if (color == 'y')
        {
            colorShift.yellow = shift;
        }
        else if (color == 'b')
        {
            colorShift.black = shift;
        }
        else
        {
            std::cerr << usageString;
            return 1;
        }
    }

    std::cout << "Cyan: " << colorShift.cyan << "  Magenta: " << colorShift.magenta << "  Yellow: " << colorShift.yellow << "  Black: " << colorShift.black << '\n';

    std::string inFilename{ argv[1] };

    std::ifstream inFile{ inFilename, std::ios::binary };
    if (!inFile)
    {
        // Print an error and exit
        std::cerr << "Uh oh, '" << inFilename << "' could not be opened for reading!" << std::endl;
        return 1;
    }

    std::string outFilename = inFilename + "tmp";
    std::ofstream outFile{ outFilename, std::ios::binary };

    size_t savedLineLen{ 0 };

    while (!inFile.eof())
    {
        unsigned int shift{ colorShift.cyan }; // if shifts are not the same this shift will be replaced by processInkLine()

        if (!colorShift.isEqual())
        { // search ink code
            if (!searchForPrefix(inkPrefix, savedLineLen, inFile, outFile))
            {
                break;
            }

            std::string line;

            shift = processInkLine(inFilename, inFile.tellg().operator-(1), line, savedLineLen, colorShift);
            
            outFile.write(line.c_str(), line.length());
        }
        
        if (!searchForPrefix(targetLinePrefix, savedLineLen, inFile, outFile))
        {
            break;
        }

        std::stringstream outLine;
        outLine << targetLinePrefix << ' ' << arrPrefix << ' ' << zero;

        std::vector<std::string> numbers = processValuesLine(inFilename, inFile.tellg().operator-(1), savedLineLen);
        std::vector<std::string> increased = increaseIntPartsInVector(numbers, shift);

        for (auto& number : increased)
        {
            outLine << ' ' << number;
        }
        outLine << ' ' << zero << ' ' << arrSuffix << ' ' << lineFinish;
        outFile.write(outLine.str().c_str(), outLine.str().length());
        std::cout << outLine.str() << "\n\n";
    }

    inFile.close();
    outFile.close();
    remove(inFilename.c_str());
    rename(outFilename.c_str(), inFilename.c_str());

    return 0;
}

std::ifstream reopenFileForTextRead(std::string filename, long long pos, std::string& readLine, size_t& savedLineLen)
{
    std::ifstream file{ filename };
    file.seekg(pos);

    if (file)
    {
        std::getline(file, readLine);
        file.seekg(pos);
        savedLineLen = readLine.length() - 1;
    }

    return file;
}

int processInkLine(std::string filename, long long pos, std::string& readLine, size_t& savedLineLen, const ColorValueShift& colorsShift)
{
    int colorShift{ 0 };

    std::ifstream file = reopenFileForTextRead(filename, pos, readLine, savedLineLen);

    std::string strInput;
    file >> strInput; // read ink prefix
    file >> strInput; // read ink code

    if (strInput.substr(0, inkCodePrefix.length()).compare(inkCodePrefix) == 0)
    {
        int colorCode = tolower(strInput[strInput.length() - 1]); // get last symbol
        if (colorCode == 'c')
        {
            colorShift = colorsShift.cyan;
        }
        else if (colorCode == 'm')
        {
            colorShift = colorsShift.magenta;
        }
        else if (colorCode == 'y')
        {
            colorShift = colorsShift.yellow;
        }
        else if (colorCode == 'b')
        {
            colorShift = colorsShift.black;
        }
    }

    return colorShift;
}

std::vector<std::string> processValuesLine(std::string filename, long long pos, size_t& savedLineLen)
{
    std::string strInput;

    std::ifstream file = reopenFileForTextRead(filename, pos, strInput, savedLineLen);

    std::vector<std::string> strNumbers;
    LinePositions linePos{ LinePositions::begin };
    LinePositions nextLinePos;

    while (file)
    {
        file >> strInput;
        std::string searchWord = "";
        switch (linePos)
        {
        case LinePositions::begin:
        {
            searchWord = targetLinePrefix;
            nextLinePos = LinePositions::firstValue;
            break;
        }
        case LinePositions::firstValue:
        {
            searchWord = arrPrefix;
            nextLinePos = LinePositions::secondValue;
            break;
        }
        case LinePositions::secondValue:
        {
            searchWord = zero;
            nextLinePos = LinePositions::lastValue;
            break;
        }
        case LinePositions::lastValue:
        {
            searchWord = zero;
            nextLinePos = LinePositions::end;
            break;
        }
        case LinePositions::end:
        {
            searchWord = arrSuffix;
            break;
        }
        default:
            break;
        }

        if (linePos < LinePositions::lastValue)
        {
            if (strInput != searchWord)
            {
                // incorrect line
                break;
            }
            linePos = nextLinePos;
        }
        else if (linePos == LinePositions::lastValue)
        {
            if (strInput != searchWord)
            {
                // process number
                strNumbers.push_back(strInput);
            }
            else
            {
                linePos = nextLinePos;
            }
        }
        else
        {
            if (strInput != searchWord)
            { // not last zero in the array
                // process number
                strNumbers.push_back(zero);
                strNumbers.push_back(strInput);
                linePos = LinePositions::lastValue;
            }
            else
            {
                // end of line
                //strNumbers.pop_back(); // remove second last value
                break;
            }
        }
    }

    return strNumbers;
}

std::vector<std::string> increaseIntPartsInVector(const std::vector<std::string>& numbers, int shift)
{
    std::vector<std::string> increased;
    
    if (!numbers.empty())
    {
        std::string lastNumber;

        for (auto& number : numbers)
        {
            lastNumber = number; // we won't increase second last value
            std::stringstream str{ number };
            std::string part;
            std::getline(str, part, '.');
            int intPart = atoi(part.c_str());
            std::getline(str, part);
            std::stringstream inc;
            inc << intPart + shift << '.' << part;
            increased.push_back(inc.str());
        }
        increased.pop_back(); // remove second last value
        increased.push_back(lastNumber); // push not increased
    }

    return increased;
}

bool searchForPrefix(std::string searchPrefix, size_t& savedLineLen, std::ifstream& inFile, std::ofstream& outFile)
{
    bool retVal = false;

    char* prefix = new char[searchPrefix.length() + 1];

    // While there's still stuff left to read
    while (!retVal && !inFile.eof())
    {
        //save current positon
        std::streampos curPos{ inFile.tellg() };

        if (!inFile.read(prefix, searchPrefix.length()))
        {
            // save tail by seekg and save byte-by-byte
            size_t bytesRead = inFile.gcount();
            for (size_t byteNo = 0; byteNo < bytesRead; ++byteNo)
            {
                if (savedLineLen == 0)
                {
                    outFile.write(&(prefix[byteNo]), 1);
                }
                else
                { // skip already saved line
                    --savedLineLen;
                }
            }
            break;
        }
        prefix[searchPrefix.length()] = 0;

        retVal = searchPrefix.compare(prefix) == 0;
        if (!retVal)
        { // prefix not found. Copy one byte to outFile and move one byte forward
            if (savedLineLen <= 0)
            {
                outFile.write(prefix, 1);
            }
            else
            { // skip already saved line
                --savedLineLen;
            }
        }
        inFile.seekg(curPos.operator+(1), std::ios::beg);
    } // while

    delete[] prefix;

    return retVal;
}