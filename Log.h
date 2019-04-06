#pragma once
class Log {
public:
  static void Info(std::string);
  static void Info(std::wstring);
  static void InfoVector(XMVECTOR&, std::string);
  static void Error(std::string);
  static void Error(std::wstring);

private:
  Log() {}


  ~Log() {}
};
