#include "FileDialog.hpp"

#include <cstdio>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <cstdlib>
#endif

namespace OxygenCrate::Platform
{
#if defined(_WIN32)
    std::string OpenFileDialog()
    {
        char fileBuffer[MAX_PATH] = {};
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = fileBuffer;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = "All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameA(&ofn))
            return std::string(fileBuffer);
        return {};
    }
#elif defined(__APPLE__)
    namespace
    {
        std::string ExecuteCommandForPath(const char* command)
        {
            std::string result;
            FILE* pipe = popen(command, "r");
            if (!pipe)
                return result;

            char buffer[512];
            while (std::fgets(buffer, sizeof(buffer), pipe))
                result.append(buffer);

            pclose(pipe);

            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            return result;
        }
    }

    std::string OpenFileDialog()
    {
        return ExecuteCommandForPath("osascript -e 'set theFile to POSIX path of (choose file)'");
    }
#elif defined(__linux__)
    namespace
    {
        std::string ExecuteCommandForPath(const char* command)
        {
            std::string result;
            FILE* pipe = popen(command, "r");
            if (!pipe)
                return result;

            char buffer[512];
            while (std::fgets(buffer, sizeof(buffer), pipe))
                result.append(buffer);

            pclose(pipe);

            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            return result;
        }
    }

    std::string OpenFileDialog()
    {
        return ExecuteCommandForPath("zenity --file-selection 2>/dev/null");
    }
#else
    std::string OpenFileDialog()
    {
        return {};
    }
#endif
}
