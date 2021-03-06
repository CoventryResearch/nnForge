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

#pragma once

#include <nnforge/raw_to_structured_data_transformer.h>

class validating_imagenet_raw_to_structured_data_transformer : public nnforge::raw_to_structured_data_transformer
{
public:
	validating_imagenet_raw_to_structured_data_transformer(
		unsigned int image_size,
		unsigned int target_image_width,
		unsigned int target_image_height,
		const std::vector<std::pair<float, float> >& position_list);

	virtual ~validating_imagenet_raw_to_structured_data_transformer();

	virtual void transform(
		unsigned int sample_id,
		const std::vector<unsigned char>& raw_data,
		float * structured_data);

	virtual nnforge::layer_configuration_specific get_configuration() const;

	virtual unsigned int get_sample_count() const;

protected:
	unsigned int image_size;
	unsigned int target_image_width;
	unsigned int target_image_height;
	std::vector<std::pair<float, float> > position_list;
};
