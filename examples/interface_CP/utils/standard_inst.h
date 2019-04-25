#ifndef INTERFACE_CP_UTILS_STANDARD_INST
#define INTERFACE_CP_UTILS_STANDARD_INST

#include <fstream>
#include <iostream>
// #include "../../externals/util/rand.h"

uint64_t J = 0;  // customer
uint64_t I = 0;  // facility
std::unordered_map<uint64_t, float> fixed_cost;
std::unordered_map<uint64_t, float> demand;

std::unordered_map<uint64_t, std::pair<float, float>> customer_x_y;
std::unordered_map<uint64_t, std::pair<float, float>> facility_x_y;

// double D_bar = 0.0, D;
// const float percentage = 0.6;
const int B = 20;
const float R = 3.25;

std::unordered_map<uint64_t, std::unordered_set<uint64_t>> set_J_i;
// facility_id to demand it can cover; used in symmetry breaking
std::unordered_map<uint64_t, double> facility_to_covered_demand;
std::unordered_map<uint64_t, std::unordered_set<uint64_t>>
    facility_to_covered_customers;

double Euclidean(uint64_t i, uint64_t j) {
  return std::sqrt(
      std::pow(facility_x_y.at(i).first - customer_x_y.at(j).first, 2.0) +
      std::pow(facility_x_y.at(i).second - customer_x_y.at(j).second, 2.0));
}

void SetCoordinates(const std::string file_path) {
  std::ifstream file;
  file.open(file_path);
  int data = 0;
  file >> I;
  file >> J;
  std::string tag;
  uint64_t id;
  double x, y;
  uint64_t f, d;
  for (uint64_t line = 0; line < I; ++line) {
    file >> tag;
    assert(tag == "F");
    file >> id;
    assert(id == line);
    file >> x;
    file >> y;
    file >> f;
    assert(f == 1);

    {
      const auto& res = fixed_cost.emplace(id, f);
      assert(res.second);
    }
    {
      const auto& res = facility_x_y.emplace(id, std::make_pair(x, y));
      assert(res.second);
    }
  }

  for (uint64_t line = 0; line < J; ++line) {
    file >> tag;
    assert(tag == "C");
    file >> id;
    assert(id == line);
    file >> x;
    file >> y;
    file >> d;

    {
      const auto& res = demand.emplace(id, d);
      assert(res.second);
    }
    {
      const auto& res = customer_x_y.emplace(id, std::make_pair(x, y));
      assert(res.second);
    }
  }

  file.close();
  // exit(1);
}

void SetData(const std::string& file_path) {
  SetCoordinates(file_path);

  // D = percentage * D_bar;

  for (uint64_t i = 0; i < I; i++) {    // facility
    for (uint64_t j = 0; j < J; j++) {  // customer
      if (Euclidean(i, j) <= R) {
        const auto& res = set_J_i[j].insert(i);
        assert(res.second);
        {
          facility_to_covered_demand[i] =
              (facility_to_covered_demand.count(i))
                  ? facility_to_covered_demand[i] + demand.at(j)
                  : demand.at(j);

          facility_to_covered_customers[i].insert(j);
        }
      }
    }
  }

  // for (const auto& it : facility_to_covered_customers) {
  //   std::cout << "\n" << it.first << "=>";
  //   for (const auto j : it.second) {
  //     std::cout << j << ", ";
  //   }
  // }
}

#endif
