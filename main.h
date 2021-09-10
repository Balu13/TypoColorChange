#pragma once

struct ColorValueShift
{
    unsigned int cyan = 0;
    unsigned int magenta = 0;
    unsigned int yellow = 0;
    unsigned int black = 0;

    bool isEqual()
    {
        return cyan == magenta && cyan == yellow && cyan == black;
    }
};

std::ifstream reopenFileForTextRead(std::string filename, long long pos, std::string& readLine, size_t& savedLineLen);
int processInkLine(std::string filename, long long pos, std::string& readLine, size_t& savedLineLen, const ColorValueShift& colorsShift);
std::vector<std::string> processValuesLine(std::string filename, long long pos, size_t& savedLineLen);
std::vector<std::string> increaseIntPartsInVector(const std::vector<std::string>& numbers, int shift);
bool searchForPrefix(std::string searchPrefix, size_t& savedLineLen, std::ifstream& inFile, std::ofstream& outFile);
