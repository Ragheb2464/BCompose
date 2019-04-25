#ifndef INTERFACE_CP_UTILS_INST_GEN
#define INTERFACE_CP_UTILS_INST_GEN
#include <iostream>
#include "../../externals/util/rand.h"

const auto J = 10000000;  // customer
const auto I = 100;       // facility
double D_bar = 0.0, D;
const int B = 15;
const float percentage = 0.6;
const auto R = 4.5;

std::vector<int> f(I, 1);
std::vector<int> d(J);
std::vector<std::pair<float, float>> customer_x_y(J);
std::vector<std::pair<float, float>> facility_x_y(I);
std::vector<std::vector<std::size_t>> set_J_i(J);

double Euclidean(std::size_t i, std::size_t j) {
  return std::sqrt(
      std::pow(facility_x_y[i].first - customer_x_y[j].first, 2.0) +
      std::pow(facility_x_y[i].second - customer_x_y[j].second, 2.0));
}

void SetData() {
  for (std::size_t j = 0; j < J; j++) {
    d[j] = RandInt(1, 100);
    D_bar += d[j];
    customer_x_y[j] =
        std::make_pair(RandRealBetween(0, 30), RandRealBetween(0, 30));
  }
  for (std::size_t i = 0; i < I; i++) {
    facility_x_y[i] =
        std::make_pair(RandRealBetween(0, 30), RandRealBetween(0, 30));
  }

  D = percentage * D_bar;

  for (std::size_t j = 0; j < J; j++) {
    for (std::size_t i = 0; i < I; i++) {
      if (Euclidean(i, j) <= R) {
        set_J_i[j].push_back(i);
      }
    }
  }
}

#endif
