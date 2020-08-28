#pragma once
#include <interfaces/lifecycle.h>

class Application {
  public:
    std::vector<LifecycleBase> getDependencies();
};