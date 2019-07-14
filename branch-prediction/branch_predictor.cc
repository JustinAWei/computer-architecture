#include "ooo_cpu.h"
#include<unordered_map>
const int history_length = 20;
const int threshold = 14 + 1.93*history_length;
int global_history, y;
unordered_map<uint64_t, vector<int> > perceptrons;
bool prediction;

void O3_CPU::initialize_branch_predictor() {
}

uint8_t O3_CPU::predict_branch(uint64_t pc) {
  if(!perceptrons.count(pc)) {
    perceptrons[pc].resize(history_length+1, 0);
  }
  // account for bias
  y = perceptrons[pc][0];
  for(int i = 1; i < history_length; i++) {
    y += perceptrons[pc][i] * (global_history & (1<<(history_length-1-i)));
  }
  prediction = y >= 0;
  return prediction;
}

void O3_CPU::last_branch_result(uint64_t pc, uint8_t taken) {
  if(!perceptrons.count(pc)) {
    perceptrons[pc].resize(history_length+1);
  }
  if(taken != prediction || abs(y) <= threshold) {
    // account for bias bit
    perceptrons[pc][0] += ((taken) ? 1 : -1);
    for(int i = 1; i < history_length; i++) {
      perceptrons[pc][i]+=((taken) ? 1 : -1) * (global_history & (1<<(history_length-1-i)));
    }
  }
  global_history = (global_history << 1) | taken;
}
