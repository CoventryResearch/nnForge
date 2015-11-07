/*
 *  Copyright 2011-2014 Maxim Milakov
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

#include "local_contrast_subtractive_2d_layer_tester_cuda.h"

#include "../local_contrast_subtractive_layer.h"
#include "../nn_types.h"

#include "util_cuda.h"

namespace nnforge
{
	namespace cuda
	{
		__global__ void local_contrast_blur_horizontal_kernel(
			float * __restrict output,
			const float * __restrict input,
			const unsigned int * __restrict affected_feature_map_list,
			const float * __restrict weights,
			int feature_map_count,
			int affected_feature_map_count,
			int window_width,
			int width,
			int height,
			int entry_count)
		{
			int x = blockIdx.x * blockDim.x + threadIdx.x;
			int y = blockIdx.y * blockDim.y + threadIdx.y;
			int z = blockIdx.z * blockDim.z + threadIdx.z;
			int entry_id = z / affected_feature_map_count;

			bool in_bounds = (x < width) && (y < height) && (entry_id < entry_count);
			if (in_bounds)
			{
				int affected_feature_map_index = z - (entry_id * affected_feature_map_count);
				int affected_feature_map_id = affected_feature_map_list[affected_feature_map_index];
				const float * current_input = input + (int)(((entry_id * feature_map_count + affected_feature_map_id) * height + y) * width + x);
				const float * current_input_low = current_input;
				const float * current_input_high = current_input;
				const float * current_weights = weights;
				float res = *current_input * *current_weights;
				#pragma unroll 4
				for(int i = 1; i < window_width; ++i)
				{
					current_weights++;
					if (i < x + 1)
						current_input_low--;
					if (i > x + 1)
						current_input_low++;
					if (i < width - x)
						current_input_high++;
					if (i > width - x)
						current_input_high--;
					res += (*current_input_low + *current_input_high) * *current_weights;
				}

				output[(z * height + y) * width + x] = res;
			}
		}

		template<int WINDOW_WIDTH>
		__global__ void local_contrast_blur_horizontal_exact_kernel(
			float * __restrict output,
			const float * __restrict input,
			const unsigned int * __restrict affected_feature_map_list,
			const float * __restrict weights,
			int feature_map_count,
			int affected_feature_map_count,
			int width,
			int height,
			int entry_count)
		{
			int x = blockIdx.x * blockDim.x + threadIdx.x;
			int y = blockIdx.y * blockDim.y + threadIdx.y;
			int z = blockIdx.z * blockDim.z + threadIdx.z;
			int entry_id = z / affected_feature_map_count;

			bool in_bounds = (x < width) && (y < height) && (entry_id < entry_count);
			if (in_bounds)
			{
				int affected_feature_map_index = z - (entry_id * affected_feature_map_count);
				int affected_feature_map_id = affected_feature_map_list[affected_feature_map_index];
				const float * current_input = input + (int)(((entry_id * feature_map_count + affected_feature_map_id) * height + y) * width + x);
				const float * current_input_low = current_input;
				const float * current_input_high = current_input;
				const float * current_weights = weights;
				float res = *current_input * *current_weights;
				#pragma unroll
				for(int i = 1; i < WINDOW_WIDTH; ++i)
				{
					current_weights++;
					if (i < x + 1)
						current_input_low--;
					if (i > x + 1)
						current_input_low++;
					if (i < width - x)
						current_input_high++;
					if (i > width - x)
						current_input_high--;
					res += (*current_input_low + *current_input_high) * *current_weights;
				}

				output[(z * height + y) * width + x] = res;
			}
		}

		__global__ void local_contrast_blur_vertical_and_subtract_kernel(
			float * __restrict output,
			const float * __restrict horizontal_blurred_input,
			const float * __restrict original_input,
			const unsigned int * __restrict affected_feature_map_list,
			const float * __restrict weights,
			int input_feature_map_count,
			int affected_feature_map_count,
			int window_height,
			int width,
			int height,
			int entry_count)
		{
			int x = blockIdx.x * blockDim.x + threadIdx.x;
			int y = blockIdx.y * blockDim.y + threadIdx.y;
			int z = blockIdx.z * blockDim.z + threadIdx.z;
			int entry_id = z / affected_feature_map_count;

			bool in_bounds = (x < width) && (y < height) && (entry_id < entry_count);
			if (in_bounds)
			{
				int affected_feature_map_index = z - (entry_id * affected_feature_map_count);
				int affected_feature_map_id = affected_feature_map_list[affected_feature_map_index];
				const float * current_input = horizontal_blurred_input + (int)((z * height + y) * width + x);
				const float * current_input_low = current_input;
				const float * current_input_high = current_input;
				const float * current_weights = weights;
				float res = *current_input * *current_weights;
				#pragma unroll 4
				for(int i = 1; i < window_height; ++i)
				{
					current_weights++;
					if (i < y + 1)
						current_input_low -= width;
					if (i > y + 1)
						current_input_low += width;
					if (i < height - y)
						current_input_high += width;
					if (i > height - y)
						current_input_high -= width;
					res += (*current_input_low + *current_input_high) * *current_weights;
				}

				int offset = ((entry_id * input_feature_map_count + affected_feature_map_id) * height + y) * width + x;
				output[offset] = original_input[offset] - res;
			}
		}

		template<int WINDOW_HEIGHT>
		__global__ void local_contrast_blur_vertical_and_subtract_exact_kernel(
			float * __restrict output,
			const float * __restrict horizontal_blurred_input,
			const float * __restrict original_input,
			const unsigned int * __restrict affected_feature_map_list,
			const float * __restrict weights,
			int input_feature_map_count,
			int affected_feature_map_count,
			int width,
			int height,
			int entry_count)
		{
			int x = blockIdx.x * blockDim.x + threadIdx.x;
			int y = blockIdx.y * blockDim.y + threadIdx.y;
			int z = blockIdx.z * blockDim.z + threadIdx.z;
			int entry_id = z / affected_feature_map_count;

			bool in_bounds = (x < width) && (y < height) && (entry_id < entry_count);
			if (in_bounds)
			{
				int affected_feature_map_index = z - (entry_id * affected_feature_map_count);
				int affected_feature_map_id = affected_feature_map_list[affected_feature_map_index];
				const float * current_input = horizontal_blurred_input + (int)((z * height + y) * width + x);
				const float * current_input_low = current_input;
				const float * current_input_high = current_input;
				const float * current_weights = weights;
				float res = *current_input * *current_weights;
				#pragma unroll
				for(int i = 1; i < WINDOW_HEIGHT; ++i)
				{
					current_weights++;
					if (i < y + 1)
						current_input_low -= width;
					if (i > y + 1)
						current_input_low += width;
					if (i < height - y)
						current_input_high += width;
					if (i > height - y)
						current_input_high -= width;
					res += (*current_input_low + *current_input_high) * *current_weights;
				}

				int offset = ((entry_id * input_feature_map_count + affected_feature_map_id) * height + y) * width + x;
				output[offset] = original_input[offset] - res;
			}
		}

		local_contrast_subtractive_2d_layer_tester_cuda::local_contrast_subtractive_2d_layer_tester_cuda()
		{
		}

		local_contrast_subtractive_2d_layer_tester_cuda::~local_contrast_subtractive_2d_layer_tester_cuda()
		{
		}

		void local_contrast_subtractive_2d_layer_tester_cuda::enqueue_forward_propagation(
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
			std::pair<dim3, dim3> kernel_1st_dims = cuda_util::get_grid_and_threadblock_sizes_2d_access(
				*cuda_config,
				output_configuration_specific.dimension_sizes[0],
				output_configuration_specific.dimension_sizes[1],
				affected_feature_map_count * entry_count);
			switch(half_window_sizes[0])
			{
			case 1:
				local_contrast_blur_horizontal_exact_kernel<1><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 2:
				local_contrast_blur_horizontal_exact_kernel<2><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 3:
				local_contrast_blur_horizontal_exact_kernel<3><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 4:
				local_contrast_blur_horizontal_exact_kernel<4><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 5:
				local_contrast_blur_horizontal_exact_kernel<5><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 6:
				local_contrast_blur_horizontal_exact_kernel<6><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 7:
				local_contrast_blur_horizontal_exact_kernel<7><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 8:
				local_contrast_blur_horizontal_exact_kernel<8><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 9:
				local_contrast_blur_horizontal_exact_kernel<9><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 10:
				local_contrast_blur_horizontal_exact_kernel<10><<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(*temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[1], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			default:
				local_contrast_blur_horizontal_kernel<<<kernel_1st_dims.first, kernel_1st_dims.second, 0, stream_id>>>(
					*temporary_working_per_entry_buffer,
					*input_buffers[0],
					*schema_data[0],
					*schema_data[1],
					output_configuration_specific.feature_map_count,
					affected_feature_map_count,
					half_window_sizes[0],
					output_configuration_specific.dimension_sizes[0],
					output_configuration_specific.dimension_sizes[1],
					entry_count);
				break;
			}

			if ((affected_feature_map_count != output_configuration_specific.feature_map_count)
				&& ((const float *)(*output_buffer) != (const float *)(*input_buffers[0])))
			{
				cuda_util::copy_buffer(
					*cuda_config,
					*input_buffers[0],
					*output_buffer,
					output_elem_count_per_entry * entry_count,
					stream_id);
			}

			std::pair<dim3, dim3> kernel_2nd_dims = cuda_util::get_grid_and_threadblock_sizes_2d_access(
				*cuda_config,
				output_configuration_specific.dimension_sizes[0],
				output_configuration_specific.dimension_sizes[1],
				affected_feature_map_count * entry_count);
			switch(half_window_sizes[1])
			{
			case 1:
				local_contrast_blur_vertical_and_subtract_exact_kernel<1><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 2:
				local_contrast_blur_vertical_and_subtract_exact_kernel<2><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 3:
				local_contrast_blur_vertical_and_subtract_exact_kernel<3><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 4:
				local_contrast_blur_vertical_and_subtract_exact_kernel<4><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 5:
				local_contrast_blur_vertical_and_subtract_exact_kernel<5><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 6:
				local_contrast_blur_vertical_and_subtract_exact_kernel<6><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 7:
				local_contrast_blur_vertical_and_subtract_exact_kernel<7><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 8:
				local_contrast_blur_vertical_and_subtract_exact_kernel<8><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 9:
				local_contrast_blur_vertical_and_subtract_exact_kernel<9><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			case 10:
				local_contrast_blur_vertical_and_subtract_exact_kernel<10><<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(*output_buffer, *temporary_working_per_entry_buffer, *input_buffers[0], *schema_data[0], *schema_data[2], output_configuration_specific.feature_map_count, affected_feature_map_count, output_configuration_specific.dimension_sizes[0], output_configuration_specific.dimension_sizes[1], entry_count);
				break;
			default:
				local_contrast_blur_vertical_and_subtract_kernel<<<kernel_2nd_dims.first, kernel_2nd_dims.second, 0, stream_id>>>(
					*output_buffer,
					*temporary_working_per_entry_buffer,
					*input_buffers[0],
					*schema_data[0],
					*schema_data[2],
					output_configuration_specific.feature_map_count,
					affected_feature_map_count,
					half_window_sizes[1],
					output_configuration_specific.dimension_sizes[0],
					output_configuration_specific.dimension_sizes[1],
					entry_count);
				break;
			}
		}

		void local_contrast_subtractive_2d_layer_tester_cuda::tester_configured()
		{
			nnforge_shared_ptr<const local_contrast_subtractive_layer> layer_derived = nnforge_dynamic_pointer_cast<const local_contrast_subtractive_layer>(layer_schema);

			affected_feature_map_count = static_cast<int>(layer_derived->feature_maps_affected.size());

			for(std::vector<unsigned int>::const_iterator it = layer_derived->window_sizes.begin(); it != layer_derived->window_sizes.end(); ++it)
				half_window_sizes.push_back(static_cast<int>((*it + 1) >> 1));
		}

		size_t local_contrast_subtractive_2d_layer_tester_cuda::get_temporary_working_per_entry_buffer_size() const
		{
			return output_elem_count_per_feature_map * affected_feature_map_count * sizeof(float);
		}

		int local_contrast_subtractive_2d_layer_tester_cuda::get_input_index_layer_can_write() const
		{
			return 0;
		}
	}
}
