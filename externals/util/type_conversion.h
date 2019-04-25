#ifndef EXTERNALS_UTILE_TYPE_COVERSION_H
#define EXTERNALS_UTILE_TYPE_COVERSION_H

// #include <numeric>

std::string ValToStr(const double value) {
  if (value >= std::numeric_limits<float>::max()) {
    return "Inf";
  }
  std::string out = std::to_string(value);
  std::size_t found = out.find(".");
  if (found != std::string::npos) {
    out.erase(found + 4, out.length());
  }
  return out;
}
#endif
