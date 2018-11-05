#include "Game.h"

int main() {
  StateForChouFleur state;
  while (state.GetStatus() < 2) {
    std::cout << state.GetStatus() << std::endl;
    state.DoGoodAction();
  }
}
