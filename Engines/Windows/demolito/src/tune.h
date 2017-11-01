#pragma once
#include <vector>
#include <string>

namespace tune {

void load(const std::string& fileName);
void search(int depth, int threads, int hash);
void logistic();

}    // namespace tune
