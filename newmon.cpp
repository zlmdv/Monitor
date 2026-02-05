#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <atomic>

// Global mutex to ensure threads don't print at the same time
std::mutex console_mtx;

// Shared values using atomics or simple doubles (since we'll lock for display)
double g_cpuUsage = 0.0;
double g_memTotal = 0.0;
double g_memUsed = 0.0;
double g_memPerc = 0.0;


void refreshUI() {
    std::lock_guard<std::mutex> lock(console_mtx);
    // clear screen
    std::cout << "\033[H\033[J"; 
    std::cout << "========= SYSTEM MONITOR (SYSMON) =========\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "CPU USAGE:    [" << g_cpuUsage << "%]\n";
    std::cout << "---------------------------------------\n";
    std::cout << "MEM TOTAL:    " << g_memTotal << " MB\n";
    std::cout << "MEM USED:     " << g_memUsed << " MB\n";
    std::cout << "MEM PERCENT:  " << std::setprecision(3) << g_memPerc << "%\n";
    std::cout << "=======================================" << std::endl;
}

void calcCpuUsage() {
    auto read_cpu = []() {
        std::ifstream file("/proc/stat");
        unsigned long user, nice, system, idle, iowait, iorq, softirq, steal;
        std::string label;
        file >> label >> user >> nice >> system >> idle >> iowait >> iorq >> softirq >> steal;
        return std::make_pair(user + nice + system + idle + iowait + iorq + softirq + steal, idle + iowait);
    };

    while (true) {
        auto [total1, idle1] = read_cpu();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto [total2, idle2] = read_cpu();

        double totalDelta = static_cast<double>(total2 - total1);
        double idleDelta  = static_cast<double>(idle2 - idle1);
        
        if (totalDelta > 0) {
            g_cpuUsage = 100.0 * (totalDelta - idleDelta) / totalDelta;
        }
        refreshUI();
    }
}

void calcMemUsage() {
    while (true) {
        std::ifstream file("/proc/meminfo");
        std::string label;
        unsigned long value, total = 0, free = 0, buf = 0, cach = 0;

        while (file >> label >> value) {
            if (label == "MemTotal:")      total = value;
            else if (label == "MemFree:")  free = value;
            else if (label == "Buffers:")  buf = value;
            else if (label == "Cached:")   { cach = value; break; }
            file.ignore(256, '\n'); 
        }

        g_memTotal = total / 1024.0;
        g_memUsed  = (total - free - buf - cach) / 1024.0;
        g_memPerc  = (g_memTotal > 0) ? (g_memUsed / g_memTotal) * 100.0 : 0.0;

        refreshUI();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main() {
    // Launching independent threads
    std::thread cpuThread(calcCpuUsage);
    std::thread memThread(calcMemUsage);

    // Join threads (standard practice, though these loops are infinite)
    cpuThread.join();
    memThread.join();

    return 0;
}