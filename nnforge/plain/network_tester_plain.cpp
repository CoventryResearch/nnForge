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

#include "network_tester_plain.h"

#include "layer_tester_plain_factory.h"
#include "../neural_network_exception.h"
#include "../debug_util.h"

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

namespace nnforge
{
	namespace plain
	{
		network_tester_plain::network_tester_plain(
			network_schema_smart_ptr schema,
			plain_running_configuration_const_smart_ptr plain_config)
			: network_tester(schema)
			, plain_config(plain_config)
		{
			const const_layer_list& layer_list = *schema;
			for(const_layer_list::const_iterator it = layer_list.begin(); it != layer_list.end(); ++it)
				tester_list.push_back(plain::single_layer_tester_plain_factory::get_const_instance().get_tester_plain_layer((*it)->get_uuid()));
		}

		network_tester_plain::~network_tester_plain()
		{
		}

		output_neuron_value_set_smart_ptr network_tester_plain::actual_run(
			unsupervised_data_reader& reader,
			unsigned int sample_count)
		{
			reader.reset();

			const unsigned int input_neuron_count = reader.get_input_configuration().get_neuron_count();
			const unsigned int output_neuron_count = (layer_config_list.end() - 1)->get_neuron_count();
			const unsigned int entry_count = reader.get_entry_count();
			const unsigned int input_feature_map_count = reader.get_input_configuration().feature_map_count;
			const unsigned int neuron_count_per_input_feature_map = reader.get_input_configuration().get_neuron_count_per_feature_map();
			neuron_data_type::input_type type_code = reader.get_input_type();
			size_t input_neuron_elem_size = reader.get_input_neuron_elem_size();

			output_neuron_value_set_smart_ptr predicted_output_neuron_value_set(new output_neuron_value_set(entry_count / sample_count, output_neuron_count));

			buffer_plain_size_configuration buffers_config;
			update_buffers_configuration_testing(buffers_config);
			buffers_config.add_per_entry_buffer(input_neuron_count * input_neuron_elem_size); // input
			buffers_config.add_per_entry_buffer(input_neuron_count * sizeof(float)); // converted input

			const unsigned int max_entry_count = std::min<unsigned int>(plain_config->get_max_entry_count(buffers_config), reader.get_entry_count());

			std::vector<unsigned char> input_buf(input_neuron_count * max_entry_count * input_neuron_elem_size);
			additional_buffer_smart_ptr input_converted_buf(new std::vector<float>(input_neuron_count * max_entry_count));

			additional_buffer_smart_ptr output_buffer = input_converted_buf;
			std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> > input_buffer_and_additional_buffers_pack;
			{
				const const_layer_list& layer_list = *schema;
				const_layer_list::const_iterator layer_it = layer_list.begin();
				layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
				for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it, ++layer_it, ++input_config_it)
				{
					additional_buffer_set additional_buffers = (*it)->allocate_additional_buffers(
						max_entry_count,
						*layer_it,
						*input_config_it,
						*(input_config_it + 1),
						plain_config);
					input_buffer_and_additional_buffers_pack.push_back(std::make_pair(output_buffer, additional_buffers));
					output_buffer = (*it)->get_output_buffer(output_buffer, additional_buffers);
				}
			}

			bool entries_remained_for_loading = true;
			unsigned int entries_copied_count = 0;
			while (entries_remained_for_loading)
			{
				unsigned int entries_available_for_processing_count = 0;
				while(entries_available_for_processing_count < max_entry_count)
				{
					bool entry_read = reader.read(&(*(input_buf.begin() + (input_neuron_count * entries_available_for_processing_count * input_neuron_elem_size))));
					if (!entry_read)
					{
						entries_remained_for_loading = false;
						break;
					}
					entries_available_for_processing_count++;
				}

				if (entries_available_for_processing_count == 0)
					break;

				// Convert input
				{
					const int elem_count = static_cast<int>(entries_available_for_processing_count * input_neuron_count);
					const std::vector<float>::iterator input_converted_buf_it_start = input_converted_buf->begin();
					if (type_code == neuron_data_type::type_byte)
					{
						const unsigned char * const input_buf_it_start = &(*input_buf.begin());
						#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
						for(int i = 0; i < elem_count; ++i)
							*(input_converted_buf_it_start + i) = static_cast<float>(*(input_buf_it_start + i)) * (1.0F / 255.0F);
					}
					else if (type_code == neuron_data_type::type_float)
					{
						const float * const input_buf_it_start = reinterpret_cast<float *>(&(*input_buf.begin()));
						#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
						for(int i = 0; i < elem_count; ++i)
							*(input_converted_buf_it_start + i) = *(input_buf_it_start + i);
					}
					else throw neural_network_exception((boost::format("actual_run cannot handle input neurons of type %1%") % type_code).str());
				}

				// Run ann
				{
					const const_layer_list& layer_list = *schema;
					const_layer_list::const_iterator layer_it = layer_list.begin();
					layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
					std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> >::iterator buffers_it = input_buffer_and_additional_buffers_pack.begin();
					layer_data_list::const_iterator data_it = net_data->data_list.begin();
					layer_data_custom_list::const_iterator data_custom_it = net_data->data_custom_list.begin();
					unsigned int layer_id = 0;
					for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it, ++layer_it, ++input_config_it, ++buffers_it, ++data_it, ++data_custom_it, ++layer_id)
					{
						/*
						{
							boost::filesystem::path dir = "Debug";
							dir /= "CPU";
							boost::filesystem::create_directories(dir);
							debug_util::dump_list(
								&(*buffers_it->first->begin()),
								buffers_it->first->size(),
								(dir / (boost::format("input_neurons_%1%.txt") % layer_id).str()).string().c_str());
						}
						*/

						(*it)->test(
							buffers_it->first,
							buffers_it->second,
							plain_config,
							*layer_it,
							*data_it,
							*data_custom_it,
							*input_config_it,
							*(input_config_it + 1),
							entries_available_for_processing_count);
					}

					/*
					{
						boost::filesystem::path dir = "Debug";
						dir /= "CPU";
						boost::filesystem::create_directories(dir);
						debug_util::dump_list(
							&(*output_buffer->begin()),
							output_buffer->size(),
							(dir / "output_neurons.txt").string().c_str());
					}
					*/
				}

				// Copy predicted values
				{
					float mult = 1.0F / static_cast<float>(sample_count);
					for(unsigned int i = 0; i < entries_available_for_processing_count; ++i)
					{
						std::vector<float>& dst = predicted_output_neuron_value_set->neuron_value_list[(i + entries_copied_count) / sample_count];
						std::vector<float>::const_iterator src_it = output_buffer->begin() + (i * output_neuron_count);
						for(unsigned int j = 0; j < output_neuron_count; ++j)
							dst[j] += mult * *(src_it + j);
					}
				}

				entries_copied_count += entries_available_for_processing_count;
			}

			return predicted_output_neuron_value_set;
		}

		void network_tester_plain::actual_set_data(network_data_smart_ptr data)
		{
			net_data = data;
		}

		void network_tester_plain::actual_clear_data()
		{
			net_data.reset();
		}

		std::vector<layer_configuration_specific_snapshot_smart_ptr> network_tester_plain::actual_get_snapshot(
			const void * input,
			neuron_data_type::input_type type_code)
		{
			std::vector<layer_configuration_specific_snapshot_smart_ptr> res;

			const unsigned int input_neuron_count = layer_config_list[0].get_neuron_count();
			const unsigned int input_feature_map_count = layer_config_list[0].feature_map_count;
			const unsigned int neuron_count_per_input_feature_map = layer_config_list[0].get_neuron_count_per_feature_map();

			additional_buffer_smart_ptr input_converted_buf(new std::vector<float>(input_neuron_count));

			additional_buffer_smart_ptr output_buffer = input_converted_buf;
			std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> > input_buffer_and_additional_buffers_pack;
			std::vector<additional_buffer_smart_ptr> output_buffer_list;
			{
				const const_layer_list& layer_list = *schema;
				const_layer_list::const_iterator layer_it = layer_list.begin();
				layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
				for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it)
				{
					additional_buffer_set additional_buffers = (*it)->allocate_additional_buffers(
						1,
						*layer_it,
						*input_config_it,
						*(input_config_it + 1),
						plain_config);
					input_buffer_and_additional_buffers_pack.push_back(std::make_pair(output_buffer, additional_buffers));
					output_buffer = (*it)->get_output_buffer(output_buffer, additional_buffers);
					output_buffer_list.push_back(output_buffer);
					++layer_it;
					++input_config_it;
				}
			}

			// Convert input
			{
				layer_configuration_specific_snapshot_smart_ptr input_elem(new layer_configuration_specific_snapshot(layer_config_list[0]));
				res.push_back(input_elem);
				const int elem_count = static_cast<int>(input_neuron_count);
				const std::vector<float>::iterator input_converted_buf_it_start = input_converted_buf->begin();
				const std::vector<float>::iterator input_elem_it_start = input_elem->data.begin();
				if (type_code == neuron_data_type::type_byte)
				{
					const unsigned char * const input_buf_it_start = static_cast<const unsigned char *>(input);
					#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
					for(int i = 0; i < elem_count; ++i)
					{

						float val = static_cast<float>(*(input_buf_it_start + i)) * (1.0F / 255.0F);
						*(input_converted_buf_it_start + i) = val;
						*(input_elem_it_start + i) = val;
					}
				}
				else if (type_code == neuron_data_type::type_float)
				{
					const float * const input_buf_it_start = static_cast<const float *>(input);
					#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
					for(int i = 0; i < elem_count; ++i)
					{
						float val = *(input_buf_it_start + i);
						*(input_converted_buf_it_start + i) = val;
						*(input_elem_it_start + i) = val;
					}
				}
				else
					throw neural_network_exception((boost::format("actual_get_snapshot cannot handle input neurons of type %1%") % type_code).str());
			}

			// Run ann
			{
				const const_layer_list& layer_list = *schema;
				const_layer_list::const_iterator layer_it = layer_list.begin();
				layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
				std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> >::iterator buffers_it = input_buffer_and_additional_buffers_pack.begin();
				layer_data_list::const_iterator data_it = net_data->data_list.begin();
				layer_data_custom_list::const_iterator data_custom_it = net_data->data_custom_list.begin();
				std::vector<additional_buffer_smart_ptr>::iterator output_it = output_buffer_list.begin();
				for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it, ++layer_it, ++input_config_it, ++buffers_it, ++output_it, ++data_it, ++data_custom_it)
				{
					(*it)->test(
						buffers_it->first,
						buffers_it->second,
						plain_config,
						*layer_it,
						*data_it,
						*data_custom_it,
						*input_config_it,
						*(input_config_it + 1),
						1);

					layer_configuration_specific_snapshot_smart_ptr new_elem(new layer_configuration_specific_snapshot(*(input_config_it + 1)));
					res.push_back(new_elem);

					std::copy((*output_it)->begin(), (*output_it)->end(), new_elem->data.begin());
				}
			}

			return res;
		}

		layer_configuration_specific_snapshot_smart_ptr network_tester_plain::actual_run(
			const void * input,
			neuron_data_type::input_type type_code)
		{
			layer_configuration_specific_snapshot_smart_ptr res(new layer_configuration_specific_snapshot(layer_config_list[layer_config_list.size() - 1]));

			const unsigned int input_neuron_count = layer_config_list[0].get_neuron_count();
			const unsigned int input_feature_map_count = layer_config_list[0].feature_map_count;
			const unsigned int neuron_count_per_input_feature_map = layer_config_list[0].get_neuron_count_per_feature_map();

			additional_buffer_smart_ptr input_converted_buf(new std::vector<float>(input_neuron_count));

			additional_buffer_smart_ptr output_buffer = input_converted_buf;
			std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> > input_buffer_and_additional_buffers_pack;
			{
				const const_layer_list& layer_list = *schema;
				const_layer_list::const_iterator layer_it = layer_list.begin();
				layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
				for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it)
				{
					additional_buffer_set additional_buffers = (*it)->allocate_additional_buffers(
						1,
						*layer_it,
						*input_config_it,
						*(input_config_it + 1),
						plain_config);
					input_buffer_and_additional_buffers_pack.push_back(std::make_pair(output_buffer, additional_buffers));
					output_buffer = (*it)->get_output_buffer(output_buffer, additional_buffers);
					++layer_it;
					++input_config_it;
				}
			}

			// Convert input
			{
				const int elem_count = static_cast<int>(input_neuron_count);
				const std::vector<float>::iterator input_converted_buf_it_start = input_converted_buf->begin();
				if (type_code == neuron_data_type::type_byte)
				{
					const unsigned char * const input_buf_it_start = static_cast<const unsigned char *>(input);
					#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
					for(int i = 0; i < elem_count; ++i)
						*(input_converted_buf_it_start + i) = static_cast<float>(*(input_buf_it_start + i)) * (1.0F / 255.0F);
				}
				else if (type_code == neuron_data_type::type_float)
				{
					const float * const input_buf_it_start = static_cast<const float *>(input);
					#pragma omp parallel for default(none) schedule(guided) num_threads(plain_config->openmp_thread_count)
					for(int i = 0; i < elem_count; ++i)
						*(input_converted_buf_it_start + i) = *(input_buf_it_start + i);
				}
				else
					throw neural_network_exception((boost::format("actual_run cannot handle input neurons of type %1%") % type_code).str());
			}

			// Run ann
			{
				const const_layer_list& layer_list = *schema;
				const_layer_list::const_iterator layer_it = layer_list.begin();
				layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
				std::vector<std::pair<additional_buffer_smart_ptr, additional_buffer_set> >::iterator buffers_it = input_buffer_and_additional_buffers_pack.begin();
				layer_data_list::const_iterator data_it = net_data->data_list.begin();
				layer_data_custom_list::const_iterator data_custom_it = net_data->data_custom_list.begin();
				for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it, ++layer_it, ++input_config_it, ++buffers_it, ++data_it, ++data_custom_it)
				{
					(*it)->test(
						buffers_it->first,
						buffers_it->second,
						plain_config,
						*layer_it,
						*data_it,
						*data_custom_it,
						*input_config_it,
						*(input_config_it + 1),
						1);
				}
			}

			std::copy(output_buffer->begin(), output_buffer->end(), res->data.begin());

			return res;
		}

		void network_tester_plain::layer_config_list_modified()
		{
		}

		void network_tester_plain::update_buffers_configuration_testing(buffer_plain_size_configuration& buffer_configuration) const
		{
			for(std::vector<layer_data_smart_ptr>::const_iterator it = net_data->data_list.begin(); it != net_data->data_list.end(); ++it)
				for(layer_data::const_iterator it2 = (*it)->begin(); it2 != (*it)->end(); ++it2)
					buffer_configuration.add_constant_buffer(it2->size() * sizeof(float));
			for(std::vector<layer_data_custom_smart_ptr>::const_iterator it = net_data->data_custom_list.begin(); it != net_data->data_custom_list.end(); ++it)
				for(layer_data_custom::const_iterator it2 = (*it)->begin(); it2 != (*it)->end(); ++it2)
					buffer_configuration.add_constant_buffer(it2->size() * sizeof(float));

			const const_layer_list& layer_list = *schema;
			const_layer_list::const_iterator layer_it = layer_list.begin();
			layer_configuration_specific_list::const_iterator input_config_it = layer_config_list.begin();
			for(std::vector<const_layer_tester_plain_smart_ptr>::const_iterator it = tester_list.begin(); it != tester_list.end(); ++it, ++layer_it, ++input_config_it)
			{
				(*it)->update_buffer_configuration(
					buffer_configuration,
					*layer_it,
					*input_config_it,
					*(input_config_it + 1),
					plain_config);
			}
		}
	}
}
