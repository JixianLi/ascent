//
// Created by Li, Jixian on 2019-06-04.
//

#include "ascent_runtime_babelflow_filters.h"

#include <conduit.hpp>
#include <conduit_relay.hpp>
#include <conduit_blueprint.hpp>


void ascent::runtime::filter::BabelFlow::declare_interface(conduit::Node &i) {
  i["type_name"] = "babelflow";
  i["port_names"].append() = "in";
  i["output_port"] = "false";
}


void ascent::runtime::filter::BabelFlow::execute() {
  if (op == PMT) {
    // connect to the input port and get the parameters
    conduit::Node p = params();
    auto *in = input<conduit::Node>("in");
    auto &data_node = in->children().next();

    // get the data handle
    conduit::DataArray<float> array = data_node[p["data_path"].as_string()].as_float32_array();

    // get the parameters
    MPI_Comm comm = MPI_Comm_f2c(p["mpi_comm"].as_int());
    int rank;
    MPI_Comm_rank(comm, &rank);
    int32_t *data_size = p["data_size"].as_int32_ptr();
    int32_t *low = p["low"].as_int32_ptr();
    int32_t *high = p["high"].as_int32_ptr();
    int32_t *n_blocks = p["n_blocks"].as_int32_ptr();
    int32_t task_id;
    if (!p.has_child("task_id") || p["task_id"].as_int32() == -1) {
      MPI_Comm_rank(comm, &task_id);
    } else {
      task_id = p["task_id"].as_int32();
    }
    int32_t fanin = p["fanin"].as_int32();
    FunctionType threshold = p["threshold"].as_float();

    // create ParallelMergeTree instance and run
    ParallelMergeTree pmt(reinterpret_cast<FunctionType *>(array.data_ptr()),
                          task_id,
                          data_size,
                          n_blocks,
                          low, high,
                          fanin, threshold, comm);

    ParallelMergeTree::s_data_size[0] = data_size[0];
    ParallelMergeTree::s_data_size[1] = data_size[1];
    ParallelMergeTree::s_data_size[2] = data_size[2];



    pmt.Initialize();
    pmt.Execute();
  } else {
    return;
  }
}

bool ascent::runtime::filter::BabelFlow::verify_params(const conduit::Node &params, conduit::Node &info) {
  if (params.has_child("task")) {
    std::string task_str(params["task"].as_string());
    if (task_str == "pmt") {
      this->op = PMT;
    } else {
      std::cerr << "[Error] ascent::BabelFlow\nUnknown task \"" << task_str << "\"" << std::endl;
      return false;
    }
  } else {
    std::cerr << "[Error] ascent::BabelFlow\ntask need to be specified" << std::endl;
    return false;
  }
  return true;
}