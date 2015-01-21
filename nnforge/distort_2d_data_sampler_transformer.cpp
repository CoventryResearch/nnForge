/*
 *  Copyright 2011-2015 Maxim Milakov
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

#include "distort_2d_data_sampler_transformer.h"

#include "neural_network_exception.h"
#include "data_transformer_util.h"

#include <opencv2/core/core.hpp>
#include <boost/format.hpp>
#include <cstring>

namespace nnforge
{
	distort_2d_data_sampler_transformer::distort_2d_data_sampler_transformer(
		const std::vector<distort_2d_data_sampler_param>& params,
		unsigned char border_value)
		: params(params)
		, border_value(border_value)
	{
	}

	distort_2d_data_sampler_transformer::distort_2d_data_sampler_transformer(
		const std::vector<float>& rotation_angle_in_degrees_list,
		const std::vector<float>& scale_list,
		const std::vector<float>& shift_right_x_list,
		const std::vector<float>& shift_down_y_list,
		unsigned char border_value)
		: border_value(border_value)
	{
		for(std::vector<float>::const_iterator it1 = rotation_angle_in_degrees_list.begin(); it1 != rotation_angle_in_degrees_list.end(); ++it1)
			for(std::vector<float>::const_iterator it2 = scale_list.begin(); it2 != scale_list.end(); ++it2)
				for(std::vector<float>::const_iterator it3 = shift_right_x_list.begin(); it3 != shift_right_x_list.end(); ++it3)
					for(std::vector<float>::const_iterator it4 = shift_down_y_list.begin(); it4 != shift_down_y_list.end(); ++it4)
					{
						distort_2d_data_sampler_param new_item;
						new_item.rotation_angle_in_degrees = *it1;
						new_item.scale = *it2;
						new_item.shift_right_x = *it3;
						new_item.shift_down_y = *it4;
						params.push_back(new_item);
					}
	}

	distort_2d_data_sampler_transformer::~distort_2d_data_sampler_transformer()
	{
	}

	void distort_2d_data_sampler_transformer::transform(
		const void * data,
		void * data_transformed,
		neuron_data_type::input_type type,
		const layer_configuration_specific& original_config,
		unsigned int sample_id)
	{
		if (type != neuron_data_type::type_byte)
			throw neural_network_exception("distort_2d_data_sampler_transformer is implemented for data stored as bytes only");

		if (original_config.dimension_sizes.size() < 2)
			throw neural_network_exception((boost::format("distort_2d_data_sampler_transformer is processing at least 2d data, data is passed with number of dimensions %1%") % original_config.dimension_sizes.size()).str());

		float rotation_angle = params[sample_id].rotation_angle_in_degrees;
		float scale = params[sample_id].scale;
		float shift_x = params[sample_id].shift_right_x;
		float shift_y = params[sample_id].shift_down_y;

		unsigned int neuron_count_per_image = original_config.dimension_sizes[0] * original_config.dimension_sizes[1];
		unsigned int image_count = original_config.get_neuron_count() / neuron_count_per_image;
		for(unsigned int image_id = 0; image_id < image_count; ++image_id)
		{
			cv::Mat1b src_image(static_cast<int>(original_config.dimension_sizes[1]), static_cast<int>(original_config.dimension_sizes[0]), const_cast<unsigned char *>(static_cast<const unsigned char *>(data)) + (image_id * neuron_count_per_image));
			cv::Mat1b image(static_cast<int>(original_config.dimension_sizes[1]), static_cast<int>(original_config.dimension_sizes[0]), static_cast<unsigned char *>(data_transformed) + (image_id * neuron_count_per_image));
			memcpy(
				((unsigned char *)data_transformed) + image_id * neuron_count_per_image,
				((unsigned char *)data) + image_id * neuron_count_per_image,
				neuron_count_per_image * neuron_data_type::get_input_size(type));

			data_transformer_util::rotate_scale_shift(
				image,
				cv::Point2f(static_cast<float>(image.cols) * 0.5F, static_cast<float>(image.rows) * 0.5F),
				rotation_angle,
				scale,
				shift_x,
				shift_y,
				1.0F,
				0.0F,
				border_value);
		}
	}

	bool distort_2d_data_sampler_transformer::is_in_place() const
	{
		return false;
	}

	unsigned int distort_2d_data_sampler_transformer::get_sample_count() const
	{
		return static_cast<unsigned int>(params.size());
	}

	bool distort_2d_data_sampler_transformer::is_deterministic() const
	{
		return true;
	}
}
