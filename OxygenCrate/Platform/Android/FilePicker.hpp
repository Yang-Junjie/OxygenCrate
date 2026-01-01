#pragma once

#include <string>

namespace OxygenCrate::Platform
{
    void RequestFilePicker();
    std::string PollSelectedFile();
}
