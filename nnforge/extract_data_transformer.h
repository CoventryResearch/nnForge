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

#pragma once

#include "data_transformer.h"

#include <memory>

namespace nnforge
{
	class extract_data_transformer : public data_transformer
	{
	public:
		extract_data_transformer(
			const std::vector<unsigned int>& input_window_sizes,
			const std::vector<unsigned int>& output_window_sizes);

		virtual ~extract_data_transformer();

		virtual void transform(
			const float * data,
			float * data_transformed,
			const layer_configuration_specific& original_config,
			unsigned int sample_id);

		virtual layer_configuration_specific get_transformed_configuration(const layer_configuration_specific& original_config) const;

	protected:
		std::vector<unsigned int> input_window_sizes;
		std::vector<unsigned int> output_window_sizes;
	};
}
