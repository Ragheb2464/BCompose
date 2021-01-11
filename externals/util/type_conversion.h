#ifndef EXTERNALS_UTILE_TYPE_COVERSION_H
#define EXTERNALS_UTILE_TYPE_COVERSION_H

// #include <numeric>

std::string ValToStr(const double value) {
    if (value >= std::numeric_limits<float>::max()) {
        return "Inf";
    }
    std::string out = std::to_string(round(1000 * value) / 1000.0);
    std::size_t found = out.find(".");
    if (found != std::string::npos) {
        out.erase(found + 4, out.length());
    }
    return out;
}


float GetIloMin(const IloNumArray &iloArr) {
    float ilomin = 1e74;
    for (int i = 0; i < iloArr.getSize(); ++i) {
        if (iloArr[i] < ilomin) {
            ilomin = iloArr[i];
        }
    }
    return ilomin;
}

float GetIloMax(const IloNumArray &iloArr) {
    float ilomax = -1e74;
    for (int i = 0; i < iloArr.getSize(); ++i) {
        if (iloArr[i] > ilomax) {
            ilomax = iloArr[i];
        }
    }
    return ilomax;
}

#endif
