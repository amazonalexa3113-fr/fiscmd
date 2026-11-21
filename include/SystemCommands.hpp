#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace fis {

void ShowUser();
void ShowInfo();
void ShowHelp();
void RunRng();
void RunCalc();
void ShowExample();
void ShowTime();
void ShowFullTime();
void DoEcho(std::string_view input);
void RunBeep(const std::vector<std::string>& parts);
void DetectOS();
void WatchTime();
void FakeCrash();
void ShowHistory();
void ClearHistory();
void ShowVersion();
void SetTitle(const std::vector<std::string>& args);
void PrintFis();
void FlipCoin();
void SysInfo();
void MemInfo();
void Benchmark();
void ShowUpdate();
void NetWatch();
void DiskParty();
void Stars();
void DownCommand(const std::vector<std::string>& args);

} // namespace fis
