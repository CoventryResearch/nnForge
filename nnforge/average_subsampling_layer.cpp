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

#include "average_subsampling_layer.h"

#include "neural_network_exception.h"
#include "proto/nnforge.pb.h"

#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/format.hpp>
#include <sstream>

namespace nnforge
{
	const std::string average_subsampling_layer::layer_type_name = "AverageSubsampling";

	average_subsampling_layer::average_subsampling_layer(
		const std::vector<unsigned int>& subsampling_sizes,
		unsigned int feature_map_subsampling_size,
		unsigned int entry_subsampling_size)
		: subsampling_sizes(subsampling_sizes)
		, feature_map_subsampling_size(feature_map_subsampling_size)
		, entry_subsampling_size(entry_subsampling_size)
	{
		check();
	}

	void average_subsampling_layer::check()
	{
		if (subsampling_sizes.size() == 0)
			throw neural_network_exception("subsampling sizes for average subsampling layer may not be empty");

		for(unsigned int i = 0; i < subsampling_sizes.size(); i++)
		{
			if (subsampling_sizes[i] == 0)
				throw neural_network_exception("window dimension for average subsampling layer may not be zero");
		}

		if (feature_map_subsampling_size == 0)
			throw neural_network_exception("feature map subsampling size for average subsampling layer may not be zero");

		if (entry_subsampling_size == 0)
			throw neural_network_exception("feature map subsampling size for average subsampling layer may not be zero");
	}

	std::string average_subsampling_layer::get_type_name() const
	{
		return layer_type_name;
	}

	layer::ptr average_subsampling_layer::clone() const
	{
		return layer::ptr(new average_subsampling_layer(*this));
	}

	layer_configuration average_subsampling_layer::get_layer_configuration(const std::vector<layer_configuration>& input_configuration_list) const
	{
		if ((input_configuration_list[0].dimension_count >= 0) && (input_configuration_list[0].dimension_count != static_cast<int>(subsampling_sizes.size())))
			throw neural_network_exception((boost::format("Dimension count in layer (%1%) and input configuration (%2%) don't match") % subsampling_sizes.size() % input_configuration_list[0].dimension_count).str());

		if ((input_configuration_list[0].feature_map_count >= 0) && (input_configuration_list[0].feature_map_count < static_cast<int>(feature_map_subsampling_size)))
			throw neural_network_exception((boost::format("Feature map count in input configuration (%1%) is smaller than feature map subsampling size (%2%)") % input_configuration_list[0].feature_map_count % feature_map_subsampling_size).str());
		int new_feature_map_count = input_configuration_list[0].feature_map_count;
		if (new_feature_map_count >= 0)
			new_feature_map_count /= feature_map_subsampling_size;

		return layer_configuration(new_feature_map_count, static_cast<int>(subsampling_sizes.size()));
	}

	layer_configuration_specific average_subsampling_layer::get_output_layer_configuration_specific(const std::vector<layer_configuration_specific>& input_configuration_specific_list) const
	{
		if (input_configuration_specific_list[0].get_dimension_count() != subsampling_sizes.size())
			throw neural_network_exception((boost::format("Dimension count in layer (%1%) and input configuration (%2%) don't match") % subsampling_sizes.size() % input_configuration_specific_list[0].get_dimension_count()).str());

		layer_configuration_specific res(input_configuration_specific_list[0].feature_map_count / feature_map_subsampling_size);

		for(unsigned int i = 0; i < subsampling_sizes.size(); ++i)
		{
			if (input_configuration_specific_list[0].dimension_sizes[i] < subsampling_sizes[i])
				throw neural_network_exception((boost::format("Input configuration size (%1%) of dimension (%2%) is smaller than subsampling size (%3%)") % input_configuration_specific_list[0].dimension_sizes[i] % i % subsampling_sizes[i]).str());

			res.dimension_sizes.push_back(input_configuration_specific_list[0].dimension_sizes[i] / subsampling_sizes[i]);
		}

		return res;
	}

	bool average_subsampling_layer::get_input_layer_configuration_specific(
		layer_configuration_specific& input_configuration_specific,
		const layer_configuration_specific& output_configuration_specific,
		unsigned int input_layer_id) const
	{
		if (output_configuration_specific.get_dimension_count() != subsampling_sizes.size())
			throw neural_network_exception((boost::format("Dimension count in layer (%1%) and output configuration (%2%) don't match") % subsampling_sizes.size() % output_configuration_specific.get_dimension_count()).str());

		input_configuration_specific = layer_configuration_specific(output_configuration_specific.feature_map_count * feature_map_subsampling_size);

		for(unsigned int i = 0; i < subsampling_sizes.size(); ++i)
			input_configuration_specific.dimension_sizes.push_back(output_configuration_specific.dimension_sizes[i] * subsampling_sizes[i]);

		return true;
	}

	std::vector<std::pair<unsigned int, unsigned int> > average_subsampling_layer::get_input_rectangle_borders(
		const std::vector<std::pair<unsigned int, unsigned int> >& output_rectangle_borders,
		unsigned int input_layer_id) const
	{
		if (output_rectangle_borders.size() != subsampling_sizes.size())
			throw neural_network_exception((boost::format("Dimension count in layer (%1%) and output borders (%2%) don't match") % subsampling_sizes.size() % output_rectangle_borders.size()).str());

		std::vector<std::pair<unsigned int, unsigned int> > res;

		for(unsigned int i = 0; i < subsampling_sizes.size(); ++i)
			res.push_back(std::make_pair(output_rectangle_borders[i].first * subsampling_sizes[i], output_rectangle_borders[i].second * subsampling_sizes[i]));

		return res;
	}

	void average_subsampling_layer::write_proto(void * layer_proto) const
	{
		protobuf::Layer * layer_proto_typed = reinterpret_cast<nnforge::protobuf::Layer *>(layer_proto);
		nnforge::protobuf::AverageSubsamplingParam * param = layer_proto_typed->mutable_average_subsampling_param();
		for(int i = 0; i < subsampling_sizes.size(); ++i)
		{
			nnforge::protobuf::AverageSubsamplingParam_AverageSubsamplingDimensionParam * dim_param = param->add_dimension_param();
			dim_param->set_subsampling_size(subsampling_sizes[i]);
		}

		if (feature_map_subsampling_size != 1)
			param->mutable_feature_map_param()->set_subsampling_size(feature_map_subsampling_size);

		if (entry_subsampling_size != 1)
			param->mutable_entry_param()->set_subsampling_size(entry_subsampling_size);
	}

	void average_subsampling_layer::read_proto(const void * layer_proto)
	{
		const protobuf::Layer * layer_proto_typed = reinterpret_cast<const nnforge::protobuf::Layer *>(layer_proto);
		if (!layer_proto_typed->has_average_subsampling_param())
			throw neural_network_exception((boost::format("No average_subsampling_param specified for layer %1% of type %2%") % instance_name % layer_proto_typed->type()).str());
		const protobuf::AverageSubsamplingParam& param = layer_proto_typed->average_subsampling_param();

		subsampling_sizes.resize(param.dimension_param_size());
		for(int i = 0; i < param.dimension_param_size(); ++i)
		{
			subsampling_sizes[i] = param.dimension_param(i).subsampling_size();
		}

		feature_map_subsampling_size = param.has_feature_map_param() ? param.feature_map_param().subsampling_size() : 1;

		entry_subsampling_size = param.has_entry_param() ? param.entry_param().subsampling_size() : 1;

		check();
	}

	float average_subsampling_layer::get_flops_per_entry(
		const std::vector<layer_configuration_specific>& input_configuration_specific_list,
		const layer_action& action) const
	{
		switch (action.get_action_type())
		{
		case layer_action::forward:
			{
				unsigned int neuron_count = get_output_layer_configuration_specific(input_configuration_specific_list).get_neuron_count();
				unsigned int per_item_flops = feature_map_subsampling_size * entry_subsampling_size;
				std::for_each(subsampling_sizes.begin(), subsampling_sizes.end(), per_item_flops *= boost::lambda::_1);
				return static_cast<float>(neuron_count) * static_cast<float>(per_item_flops);
			}
		case layer_action::backward_data:
			{
				unsigned int neuron_count = get_output_layer_configuration_specific(input_configuration_specific_list).get_neuron_count();
				return static_cast<float>(neuron_count);
			}
		default:
			return 0.0F;
		}
	}

	tiling_factor average_subsampling_layer::get_tiling_factor() const
	{
		return tiling_factor(entry_subsampling_size).get_inverse();
	}

	std::vector<std::string> average_subsampling_layer::get_parameter_strings() const
	{
		std::vector<std::string> res;

		std::stringstream ss;
		for(int i = 0; i < subsampling_sizes.size(); ++i)
		{
			if (i != 0)
				ss << "x";
			ss << subsampling_sizes[i];
		}

		if (feature_map_subsampling_size != 1)
			ss << ", fm " << feature_map_subsampling_size;

		if (entry_subsampling_size != 1)
			ss << ", samples " << entry_subsampling_size;

		res.push_back(ss.str());

		return res;
	}
}
