/*
 * Copyright 2014 Open Connectome Project (http://openconnecto.me)
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

#include <unordered_map>

#include "matrix_worker_thread.h"
#include "sparse_matrix.h"

static const int MAX_PENDING_IOS = 32;

class matrix_io_callback: public callback
{
	typedef std::unordered_map<off_t, compute_task::ptr> task_map_t;
	task_map_t tasks;
public:
	int invoke(io_request *reqs[], int num);
	void add_task(const io_request &req, compute_task::ptr task) {
		tasks.insert(task_map_t::value_type(req.get_offset(), task));
	}
};

int matrix_io_callback::invoke(io_request *reqs[], int num)
{
	for (int i = 0; i < num; i++) {
		off_t off = reqs[i]->get_offset();
		task_map_t::const_iterator it = tasks.find(off);
		it->second->run(reqs[i]->get_buf(), reqs[i]->get_size());
	}
	return 0;
}

bool matrix_worker_thread::get_next_io(matrix_io &io)
{
	if (this_io_gen->has_next_io()) {
		io = this_io_gen->get_next_io();
		return true;
	}
	else
		return false;
}

void matrix_worker_thread::run()
{
	matrix_io_callback *cb = new matrix_io_callback();
	io->set_callback(cb);
	matrix_io mio;
	while (get_next_io(mio)) {
		compute_task::ptr task = tcreator->create(mio);
		io_request req = task->get_request();
		cb->add_task(req, task);
		io->access(&req, 1);

		// TODO it might not be good to have a fixed number of pending I/O.
		// We need to control memory consumption for the buffers.
		while (io->num_pending_ios() > MAX_PENDING_IOS)
			io->wait4complete(1);
	}
	io->wait4complete(io->num_pending_ios());
	stop();
}
