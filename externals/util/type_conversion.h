#ifndef EXTERNALS_UTILE_TYPE_COVERSION_H
#define EXTERNALS_UTILE_TYPE_COVERSION_H

std::string ValToStr(const float value) {
  std::string out = std::to_string(value);
  std::size_t found = out.find(".");
  if (found != std::string::npos) {
    out.erase(found + 4, out.length());
  }
  return out;
}
#endif
