#include <iostream>
#include <Windows.h>

int main() {
    DWORD processId;
    std::cout << "Enter the process ID: ";
    std::cin >> processId;

    std::string dllPath;
    std::cout << "Enter the DLL file path: ";
    std::cin >> dllPath;


    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process: " << GetLastError() << std::endl;
        return 1;
    }

    void* remoteBuffer = VirtualAllocEx(hProcess, NULL, dllPath.length() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (remoteBuffer == NULL) {
        std::cerr << "Failed to allocate memory in the target process: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(hProcess, remoteBuffer, dllPath.c_str(), dllPath.length() + 1, &bytesWritten)) {
        std::cerr << "Failed to write to the target process: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HMODULE hModule = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE loadLibraryAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddress, remoteBuffer, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Failed to create remote thread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    std::cout << "DLL injection successful!" << std::endl;
    return 0;
}
