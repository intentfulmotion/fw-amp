struct BalanceTiltbackConfig
{
  float highVoltage;
  float lowVoltage;
  float dutyCycle;
};

struct BalanceFootpadsConfig
{
  float adc1;
  float adc2;
};

struct BalanceConfig
{
  BalanceTiltbackConfig tiltback;
  BalanceFootpadsConfig footpads;
};