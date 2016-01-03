/*
 *  Copyright 2011-2016 Maxim Milakov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "reshape_layer_tester_cuda.h"

#include "util_cuda.h"

namespace nnforge
{
	namespace cuda
	{
		reshape_layer_tester_cuda::reshape_layer_tester_cuda()
		{
		}

		reshape_layer_tester_cuda::~reshape_layer_tester_cuda()
		{
		}

		void reshape_layer_tester_cuda::enqueue_forward_propagation(
			cudaStream_t stream_id,
			cuda_linear_buffer_device::ptr output_buffer,
			const std::vector<cuda_linear_buffer_device::const_ptr>& schema_data,
			const std::vector<cuda_linear_buffer_device::const_ptr>& data,
			const std::vector<cuda_linear_buffer_device::const_ptr>& data_custom,
			const std::vector<cuda_linear_buffer_device::const_ptr>& input_buffers,
			const std::vector<cuda_linear_buffer_device::const_ptr>& persistent_working_data,
			cuda_linear_buffer_device::ptr temporary_working_fixed_buffer,
			cuda_linear_buffer_device::ptr temporary_working_per_entry_buffer,
			unsigned int entry_count)
		{
			if ((const float *)(*input_buffers[0]) != (const float *)(*output_buffer))
			{
				cuda_util::copy_buffer(
					*cuda_config,
					*input_buffers[0],
					*output_buffer,
					output_elem_count_per_entry * entry_count,
					stream_id);
			}
		}

		int reshape_layer_tester_cuda::get_input_index_layer_can_write() const
		{
			return 0;
		}
	}
}
