/**
 * Copyright 2013 Disa Mhembere
 *
 * This file is part of FlashGraph.
 *
 * FlashGraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FlashGraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FlashGraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <signal.h>
#include <google/profiler.h>

#include <vector>
#include <algorithm>

#include "thread.h"
#include "io_interface.h"
#include "container.h"
#include "concurrency.h"

#include "vertex_index.h"
#include "graph_engine.h"
#include "graph_config.h"

vsize_t K; // Min degree necessary to be part of the k-core graph

class kcore_vertex: public compute_directed_vertex
{
  bool deleted;
  vsize_t degree; 

public:
	kcore_vertex() {
	}

  kcore_vertex(vertex_id_t id, const vertex_index *index): compute_directed_vertex(id, index) {
    this->deleted = false;
    this->degree = get_num_in_edges() + get_num_out_edges();
  }

  bool is_deleted() const {
    return deleted;
  }

  void _delete() {
    this->deleted = true;
  }

  vsize_t get_degree() {
    return degree;
  }

  void run(graph_engine &graph) {
    if (degree > K) { return; }

    if (!is_deleted()) {
			vertex_id_t id = get_id();
			request_vertices(&id, 1); // put my edgelist in page cache
    }
  }

	void run(graph_engine &graph, const page_vertex &vertex);

	virtual void run_on_messages(graph_engine &,
			const vertex_message *msgs[], int num); 
};

// If I am to be deleted, multicast this message to all my neighbors
// and activate them
class deleted_message: public vertex_message
{
  int dummy;
  public:
  deleted_message(): vertex_message(sizeof(deleted_message), true) {
  }
};

void multicast_delete_msg(graph_engine &graph, const page_vertex &vertex, edge_type E)
{
    page_byte_array::const_iterator<vertex_id_t> end_it
      = vertex.get_neigh_end(E);
    stack_array<vertex_id_t, 1024> dest_buf(vertex.get_num_edges(E));
    int num_dests = 0;
    for (page_byte_array::const_iterator<vertex_id_t> it
        = vertex.get_neigh_begin(E); it != end_it; ++it) {
      vertex_id_t id = *it;
      dest_buf[num_dests++] = id;
    } 

    // Doesn't matter who sent it, just --degree on reception 
    if (num_dests > 0) {
      graph.multicast_msg(dest_buf.data(), num_dests, deleted_message());
    }
}

// This is only run by 1st iteration active vertices
void kcore_vertex::run(graph_engine &graph, const page_vertex &vertex) {
  if(is_deleted()) {
    return; // Nothing to be done here
  }

  if ( get_degree() < K ) {
    _delete();
   
    // Send two multicast messages - [IN_EDGE, OUT_EDGE] 
    multicast_delete_msg(graph, vertex, IN_EDGE);
    multicast_delete_msg(graph, vertex, OUT_EDGE);
    
  }
}

void kcore_vertex::run_on_messages(graph_engine &,
    const vertex_message *msgs[], int num) {
  if (is_deleted()) {
    return; // nothing to be done here
  }

  for (int i = 0; i < num; i++) {
    degree--;
  }
}

void int_handler(int sig_num)
{
	if (!graph_conf.get_prof_file().empty())
		ProfilerStop();
	exit(0);
}

void print_usage()
{
	fprintf(stderr,
			"k-core [options] conf_file graph_file index_file K\n");
	fprintf(stderr, "-c confs: add more configurations to the system\n");
	graph_conf.print_help();
	params.print_help();
}

int main(int argc, char *argv[])
{
	int opt;
	std::string confs;
	int num_opts = 0;
	while ((opt = getopt(argc, argv, "c:")) != -1) {
		num_opts++;
		switch (opt) {
			case 'c':
				confs = optarg;
				num_opts++;
				break;
			default:
				print_usage();
		}
	}
	argv += 1 + num_opts;
	argc -= 1 + num_opts;

	if (argc < 4) {
		print_usage();
		exit(-1);
	}

	std::string conf_file = argv[0];
	std::string graph_file = argv[1];
	std::string index_file = argv[2];
	K = atoi(argv[3]); // set K

  //printf("Here!\n");
	config_map configs(conf_file);
	configs.add_options(confs);
	graph_conf.init(configs);
	graph_conf.print();

	signal(SIGINT, int_handler);
	init_io_system(configs);

	graph_index *index = NUMA_graph_index<kcore_vertex>::create(index_file,
			graph_conf.get_num_threads(), params.get_num_nodes());
	graph_engine *graph = graph_engine::create(graph_conf.get_num_threads(),
			params.get_num_nodes(), graph_file, index);
	printf("K-core starting\n");
	printf("prof_file: %s\n", graph_conf.get_prof_file().c_str());
	if (!graph_conf.get_prof_file().empty())
		ProfilerStart(graph_conf.get_prof_file().c_str());

  // Filter for activation first time around
  class activate_k_filter: public vertex_filter {
    vsize_t min;
    public:
    activate_k_filter (vsize_t min) {
      this->min = min;
    }
    bool keep(compute_vertex &v) {
      kcore_vertex &kcore_v = (kcore_vertex &) v;
      return kcore_v.get_num_in_edges() + kcore_v.get_num_out_edges() < min;
    }
  };

	std::shared_ptr<vertex_filter> filter
		= std::shared_ptr<vertex_filter>(new activate_k_filter(K));

	struct timeval start, end;
	gettimeofday(&start, NULL);
	graph->start(filter); 
  graph->wait4complete();
	gettimeofday(&end, NULL);

	NUMA_graph_index<kcore_vertex>::const_iterator it
		= ((NUMA_graph_index<kcore_vertex> *) index)->begin();
	NUMA_graph_index<kcore_vertex>::const_iterator end_it
		= ((NUMA_graph_index<kcore_vertex> *) index)->end();

	vsize_t in_k_core = 0;

  // Check number of visited vertices
	for (; it != end_it; ++it) {
		const kcore_vertex &v = (const kcore_vertex &) *it;
		if ( !(v.is_deleted()) ) {
			in_k_core++;
    }
	}

	if (!graph_conf.get_prof_file().empty())
		ProfilerStop();
	if (graph_conf.get_print_io_stat())
		print_io_thread_stat();
	graph_engine::destroy(graph);
	destroy_io_system();
	printf("\n%d-core shows %d vertices > %d degree in %f seconds\n",
			K, in_k_core, K, time_diff(start, end));
}
