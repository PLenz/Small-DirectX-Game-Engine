#pragma once

class Keyboard {
public:
  static void Init();
  static void KeyDown(int);
  static void KeyUp(int);

  static bool IsPressed(int);

private:
  Keyboard() {};


  ~Keyboard() {};
};
