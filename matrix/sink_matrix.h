/*
 * Copyright 2016 Open Connectome Project (http://openconnecto.me)
 * Written by Da Zheng (zhengda1936@gmail.com)
 *
 * This file is part of FlashMatrix.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FM_SINK_MATRIX_H__
#define __FM_SINK_MATRIX_H__

#include "virtual_matrix_store.h"

namespace fm
{

namespace detail
{

class sink_store: public virtual_matrix_store
{
	matrix_store::const_ptr res;

	// When we access the materialized result, we should also save it.
	// It's especially important when we access a portion of
	// the materialized matrix.
	matrix_store::const_ptr get_save_result() const {
		materialize_self();
		const_cast<sink_store *>(this)->res = get_result();
		return res;
	}
public:
	typedef std::shared_ptr<sink_store> ptr;
	typedef std::shared_ptr<const sink_store> const_ptr;

	sink_store(size_t nrow, size_t ncol, bool in_mem,
			const scalar_type &type): virtual_matrix_store(nrow, ncol,
				in_mem, type) {
	}

	// The class has been materialized.
	virtual bool has_materialized() const = 0;
	// This returns the materialized result of the sink matrix.
	virtual matrix_store::const_ptr get_result() const = 0;
	// This returns a set of computation matrix that can be used for
	// materialization with other matrices.
	virtual std::vector<virtual_matrix_store::const_ptr> get_compute_matrices() const = 0;

	virtual matrix_store::const_ptr get_cols(const std::vector<off_t> &idxs) const {
		return get_save_result()->get_cols(idxs);
	}
	virtual matrix_store::const_ptr get_rows(const std::vector<off_t> &idxs) const {
		return get_save_result()->get_rows(idxs);
	}

	using virtual_matrix_store::get_portion;
	virtual std::shared_ptr<const local_matrix_store> get_portion(
			size_t start_row, size_t start_col, size_t num_rows,
			size_t num_cols) const {
		return get_save_result()->get_portion(start_row, start_col, num_rows,
				num_cols);
	}
	virtual std::shared_ptr<const local_matrix_store> get_portion(
			size_t id) const {
		return get_save_result()->get_portion(id);
	}
	using virtual_matrix_store::get_portion_async;
	virtual async_cres_t get_portion_async(
			size_t start_row, size_t start_col, size_t num_rows,
			size_t num_cols, std::shared_ptr<portion_compute> compute) const {
		return get_save_result()->get_portion_async(start_row, start_col,
				num_rows, num_cols, compute);
	}

	virtual int get_portion_node_id(size_t id) const {
		return -1;
	}

	virtual std::pair<size_t, size_t> get_portion_size() const {
		return std::pair<size_t, size_t>(mem_matrix_store::CHUNK_SIZE,
				mem_matrix_store::CHUNK_SIZE);
	}

	virtual int get_num_nodes() const {
		return -1;
	}

	virtual std::vector<safs::io_interface::ptr> create_ios() const {
		return std::vector<safs::io_interface::ptr>();
	}
};

/*
 * This represents the computation in a sink matrix.
 * This class is used as a proxy so that we can materialize this sink matrix
 * with other matrices together. Only the methods related to getting portions
 * of the input matrices matter.
 */
class sink_compute_store: public virtual_matrix_store
{
public:
	sink_compute_store(size_t nrow, size_t ncol, bool in_mem,
			const scalar_type &type): virtual_matrix_store(nrow, ncol,
				in_mem, type) {
	}

	bool has_materialized() const {
		return false;
	}

	virtual void materialize_self() const {
	}

	virtual matrix_store::const_ptr materialize(bool in_mem,
		int num_nodes) const {
		assert(0);
		return matrix_store::const_ptr();
	}

	virtual matrix_store::const_ptr transpose() const {
		assert(0);
		return matrix_store::const_ptr();
	}

	virtual std::string get_name() const {
		return "sink_compute_store";
	}
};

/*
 * This sink matrix stores the agg results from a block matrix.
 * It binds the agg results into a single matrix.
 */
class block_sink_store: public sink_store
{
	matrix_store::const_ptr result;
	std::vector<sink_store::const_ptr> stores;
	size_t num_block_rows;
	size_t num_block_cols;

	block_sink_store(const std::vector<sink_store::const_ptr> &stores,
			size_t num_block_rows, size_t num_block_cols);
	const sink_store &get_mat(size_t i, size_t j) const {
		return *stores[i * num_block_cols + j];
	}
public:
	typedef std::shared_ptr<block_sink_store> ptr;
	typedef std::shared_ptr<const block_sink_store> const_ptr;

	static ptr create(const std::vector<matrix_store::const_ptr> &stores,
			size_t num_block_rows, size_t num_block_cols);

	virtual bool has_materialized() const;
	virtual matrix_store::const_ptr get_result() const;
	virtual std::vector<virtual_matrix_store::const_ptr> get_compute_matrices() const;
	virtual void materialize_self() const;
	virtual matrix_store::const_ptr materialize(bool in_mem, int num_nodes) const;

	virtual matrix_store::const_ptr transpose() const;

	virtual matrix_layout_t store_layout() const {
		return stores.front()->store_layout();
	}

	virtual std::string get_name() const;
	virtual std::unordered_map<size_t, size_t> get_underlying_mats() const;
};

}

}

#endif
