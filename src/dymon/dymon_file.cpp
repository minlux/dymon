#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "dymon.h"

namespace fs = std::filesystem;



DymonFile::DymonFile(const char * const outDirectory) : Dymon(1, false)
{
    counter = 0;
    this->outDirectory = std::string(outDirectory);
    // Check if directory exists, create if not
    if (!fs::exists(this->outDirectory)) {
        if (!fs::create_directories(this->outDirectory)) {
            throw std::runtime_error("Failed to create directory: " + this->outDirectory);
        }
    }
}


bool DymonFile::connect(void * arg)
{
    (void)arg;
    return true;
}


int DymonFile::send(const uint8_t * data, const size_t dataLen, bool more)
{
    (void)more;
    if (dataLen == 16) //this condition gives us the command sequence "_labelIndexHeightWidth"
    {
        //if a file is already open; close it now
        close();
        //open a new file
        {
            std::ostringstream filename;
            filename << "img_" << std::setw(4) << std::setfill('0') << ++counter << ".pbm";
            fs::path filePath = fs::path(outDirectory) / filename.str();
            outFile.open(filePath, std::ios::binary);
            if (!outFile) {
                return 0;
            }
        }

        //get label geometry and write it as header into the file
        #define LABEL_HEIGHT_OFFSET    (8) //label height must be patched int bytes [8..11]
        #define LABEL_WIDTH_OFFSET     (12) //label height must be patched int bytes [12..15]

        uint32_t height = data[LABEL_HEIGHT_OFFSET + 3];
        height = (height << 8) | data[LABEL_HEIGHT_OFFSET + 2];
        height = (height << 8) | data[LABEL_HEIGHT_OFFSET + 1];
        height = (height << 8) | data[LABEL_HEIGHT_OFFSET];

        uint32_t width = data[LABEL_WIDTH_OFFSET + 3];
        width = (width << 8) | data[LABEL_WIDTH_OFFSET + 2];
        width = (width << 8) | data[LABEL_WIDTH_OFFSET + 1];
        width = (width << 8) | data[LABEL_WIDTH_OFFSET];

        // write header
        outFile << "P4\n" << width << " " << height << "\n";
    }
    else if (dataLen >= 22) //this condition filters all command sequences leaf use with the bitmap data
    {
        // write file body
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(data), dataLen);
            outFile.close();
        }
    }
    return dataLen;
}


int DymonFile::receive(uint8_t * buffer, const size_t bufferLen)
{
    buffer[0] = 0;
    return 1;
}


void DymonFile::close()
{
    if (outFile.is_open()) {
        outFile.close();
    }
}



