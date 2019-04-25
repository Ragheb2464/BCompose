#ifndef CONTRIB_CONTROL_DATA_H
#define CONTRIB_CONTROL_DATA_H

#include <unordered_map>
#include <unordered_set>

struct NodeInfo {
  std::unordered_set<int> exiting_edges;   // head node ids
  std::unordered_set<int> entering_edges;  // tail node ids
  bool is_origin = false;
  bool is_destination = false;
};
struct ArcInfo {
  double fixed_cost, flow_cost, capacity;
  int arc_id;
};
struct Data {
  int num_nodes, num_arcs, num_dummy_arcs, num_od, num_scenario;
  // for a node, it gives those neighbors which are head/tail of an arc
  // exiting/entering from the given node
  std::unordered_map<int, std::unordered_set<int>> node_id_to_head_neighbors;
  std::unordered_map<int, std::unordered_set<int>> node_id_to_tail_neighbors;
  //
  std::unordered_map<std::pair<int, int>, ArcInfo, Hasher>
      arcs_info;                              // tail-head->ArcInfo
  std::unordered_set<int> origin_nodes;       // origin node id
  std::unordered_set<int> destination_nodes;  // destination node id
  std::unordered_map<int, std::pair<int, int>>
      od_pair;  // k->origin-destination

  std::vector<double> scenario_probability;  // scenario prob
  std::vector<std::vector<double>> demands;  // demand for each scenario:s->k

  uint64_t total_num_constraints_in_subproblem;  // in the determinstic version
};

/*!
  This function reads the input graph files for the R instances
*/
void GraphFileReader(const std::string &file_path, Data &data,
                     const std::shared_ptr<spdlog::logger> console) {
  //! Read the graph data
  std::fstream instance_file;
  instance_file.open(file_path, std::ios::in);
  assert(instance_file.is_open());
  assert(instance_file.good());
  std::string value;
  int t = 0;
  int k = 0;
  int arc_id = 0;  // to number arcs in the order they are read
  while (true) {
    instance_file >> value;
    if (instance_file.eof()) {
      break;
    }
    if (t == 0) {
      ++t;
      continue;
    } else if (t == 1) {
      assert(data.num_nodes == std::atoi(value.c_str()));
    } else if (t == 2) {
      assert(data.num_arcs == std::atoi(value.c_str()));
    } else if (t == 3) {
      assert(data.num_od == std::atoi(value.c_str()));
      data.num_dummy_arcs = data.num_od;  // for the complete recourse
    } else if (t >= 4 && t < 4 + 7 * data.num_arcs) {
      ArcInfo arc_info;
      const int tail = std::atoi(value.c_str()) - 1;
      instance_file >> value;
      ++t;
      const int head = std::atoi(value.c_str()) - 1;
      assert(head != tail);
      const auto &res =
          data.arcs_info.emplace(std::make_pair(tail, head), arc_info);
      assert(res.second);
      auto &arc = res.first->second;
      arc.arc_id = arc_id;
      ++arc_id;

      instance_file >> value;
      ++t;
      arc.flow_cost = std::atoi(value.c_str());
      instance_file >> value;
      ++t;
      arc.capacity = std::atoi(value.c_str());
      instance_file >> value;
      ++t;
      arc.fixed_cost = std::atoi(value.c_str());

      instance_file >> value;
      ++t;
      instance_file >> value;
      ++t;
    } else if (t >= 4 + 7 * data.num_arcs) {
      const int origin = std::atoi(value.c_str());

      instance_file >> value;
      const int destination = std::atoi(value.c_str());
      ++t;

      data.origin_nodes.insert(origin);
      data.destination_nodes.insert(destination);
      const auto &res =
          data.od_pair.emplace(k, std::make_pair(origin, destination));
      ++k;
      if (!res.second) {
        console->error("   ++Something wrong with data");
        exit(0);
      }
      instance_file >> value;
      ++t;
    }
    ++t;
  }
  instance_file.close();

  //! Keeping node info map...having it facilitates things later on
  for (const auto &it : data.arcs_info) {
    const int tail = it.first.first;
    const int head = it.first.second;
    data.node_id_to_head_neighbors[tail].insert(head);
    data.node_id_to_tail_neighbors[head].insert(tail);
  }

  data.total_num_constraints_in_subproblem =
      data.num_nodes * data.num_od + data.num_arcs;
  //! Extra sanity checks
  assert(data.arcs_info.size() == data.num_arcs);
  assert(data.od_pair.size() == data.num_od);
  assert(data.node_id_to_head_neighbors.size() ==
         data.node_id_to_tail_neighbors.size());
  assert(data.node_id_to_head_neighbors.size() == data.num_nodes);
  console->info("   +Done reading the graph file.");
}

/*!
  This function reads the scenario data file, here demand is only scenario based
*/
void ScenarioFileReader(const std::string scenario_path, Data &data,
                        const std::shared_ptr<spdlog::logger> console) {
  std::fstream scenario_file;
  scenario_file.open(scenario_path, std::ios::in);
  assert(scenario_file.is_open());
  assert(scenario_file.good());
  std::string value;
  int t = 0;
  while (true) {
    scenario_file >> value;
    if (scenario_file.eof()) {
      break;
    }
    if (t == 0) {
      assert(data.num_scenario == std::stoi(value));
    } else {
      data.scenario_probability.push_back(std::stod(value));

      std::vector<double> vec;
      for (int i = 0; i < data.num_od; i++) {
        scenario_file >> value;
        ++t;
        vec.push_back(std::stod(value));
      }
      data.demands.push_back(vec);
    }
    ++t;
  }

  // sanity check
  assert(data.demands.size() == data.num_scenario);
  for (int s = 0; s < data.num_scenario; s++) {
    assert(data.demands[s].size() == data.num_od);
  }
  assert(data.scenario_probability.size() == data.num_scenario);
  console->info("   +Done reading the scenario file.");
}

#endif
