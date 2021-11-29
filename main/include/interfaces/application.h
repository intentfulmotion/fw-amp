#pragma once
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>

class Application : public LifecycleBase, public PowerListener
{
};