#include <renderers/pattern.h>

void PatternRenderer::process() {
  // if (_pattern.compare("theater-chase-rainbow") == 0)
  //   theaterChaseRainbow(1, 50);
  // else if (_pattern.compare("theater-chase") == 0)
  //   theaterChase(_lights->randomColorRGB(), 10, 50);
  // else if (_pattern.compare("rainbow") == 0)
  //   rainbow(1, 5);
  // else if (_pattern.compare("lightning") == 0)
  //   lightning(3, 50, 50);
}

void PatternRenderer::allColorRGB(ColorRGB color) {
  // for (auto channel : channels) {
  //   _lights->colorLEDs(channel.first, 0, channel.second.leds, color);
  // }
  // _lights->render();
}

void PatternRenderer::allRandom() {
  // for (auto channel : channels) {
  //   _lights->colorLEDs(channel.first, 0, channel.second.leds, _lights->randomColorRGB());
  // }
  // _lights->render();
}

void PatternRenderer::dissolve(int simultaneous, int cycles, int speed) {
  // for(int i = 0; i < cycles; i++){
  //   for(int j = 0; j < simultaneous; j++) {
  //     for (auto channel : channels) {
  //       int idx = rand() % channel.second.leds;
  //       _lights->colorLEDs(channel.first, idx, idx + 1, lightOff);
  //     }
  //   }
  //   _lights->render();
  //   delay(speed);
  // }

  // allColorRGB(lightOff);
}

void PatternRenderer::rainbow(int cycles, int speed) {
  // if(cycles == 0){
  //   for (auto channel : channels) {
  //     auto controller = _lights->channelMap[channel.first];

  //     for(int i =  0; i < channel.second.leds; i++)
  //       (*controller)[i] = _lights->Wheel(((i * 256 / channel.second.leds)) & 255);
  //   }
  //   _lights->render();
  // }
  // else{
  //   for(int j = 0; j < 256*cycles; j++) {
  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];

  //       for(int i = 0; i < channel.second.leds; i++)
  //         (*controller)[i] = _lights->Wheel(((i * 256 / channel.second.leds) + j) & 255);
  //     }
  //     _lights->render();
  //     delay(speed);
  //   }
  // }
}

void PatternRenderer::theaterChase(ColorRGB c, int cycles, int speed) { // TODO direction
  // for (int j = 0; j < cycles; j++) {  
  //   for (int q = 0; q < 3; q++) {
  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];
  //       for (int i = 0; i < channel.second.leds; i = i + 3) {
  //         int pos = i + q;
  //         (*controller)[pos] = c;    //turn every third pixel on
  //       }
  //     }
  //     _lights->render();

  //     delay(speed);

  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];
  //       for (int i=0; i < channel.second.leds; i=i+3) {
  //         (*controller)[i+q] = lightOff;        //turn every third pixel off
  //       }
  //     }
  //   }
  // }
}

void PatternRenderer::theaterChaseRainbow(int cycles, int speed) { // TODO direction, duration
  // for (int j = 0; j < 256 * cycles; j++) {     // cycle all 256 colors in the wheel
  //   for (int q = 0; q < 3; q++) {
  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];
  //       for (int i = 0; i < channel.second.leds; i = i + 3) {
  //         int pos = i + q;
  //         (*controller)[pos] = _lights->Wheel((i + j) % 255);    //turn every third pixel on
  //       }
  //     }
  //     _lights->render();

  //     delay(speed);

  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];
  //       for (int i=0; i < channel.second.leds; i=i+3) {
  //         (*controller)[i+q] = lightOff;  //turn every third pixel off
  //       }
  //     }
  //   }
  // }
}

void PatternRenderer::lightning(int simultaneous, int cycles, int speed) {
  // int flashes[simultaneous][4];

  // for(int i = 0; i < cycles; i++){
  //   for(int j = 0; j < simultaneous; j++){
  //     for (auto channel : channels) {
  //       auto controller = _lights->channelMap[channel.first];

  //       int idx = rand() % channel.second.leds;
  //       flashes[j][channel.first] = idx;
  //       (*controller)[idx] = _lights->randomColorRGB();
  //     }
  //   }
  //   _lights->render();
  //   delay(speed);

  //   for (auto channel : channels) {
  //     auto controller = _lights->channelMap[channel.first];

  //     for(int s = 0; s < simultaneous; s++){
  //       (*controller)[flashes[s][channel.first]] = lightOff;
  //     }
  //   }
  //   delay(speed);
  // }
}