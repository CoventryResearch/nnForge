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

#include "network_trainer_sgd.h"

#include <boost/format.hpp>
#include <numeric>
#include <limits>

#include "neural_network_exception.h"

namespace nnforge
{
	network_trainer_sgd::network_trainer_sgd(
		network_schema_smart_ptr schema,
		network_updater_smart_ptr updater)
		: network_trainer(schema)
		, updater(updater)
	{
	}

	network_trainer_sgd::~network_trainer_sgd()
	{
	}

	void network_trainer_sgd::train_step(
		supervised_data_reader& reader,
		training_task_state& task)
	{
		boost::chrono::steady_clock::time_point start = boost::chrono::high_resolution_clock::now();

		std::pair<std::vector<std::vector<float> >, std::string> lr_and_comment = prepare_learning_rates(task.get_current_epoch(), task.data);
		task.comments.push_back(lr_and_comment.second);

		std::pair<testing_result_smart_ptr, training_stat_smart_ptr> train_result = updater->update(
			reader,
			lr_and_comment.first,
			task.data,
			batch_size,
			weight_decay,
			momentum,
			false);

		boost::chrono::duration<float> sec = (boost::chrono::high_resolution_clock::now() - start);

		float flops = updater->get_flops_for_single_entry();

		train_result.first->time_to_complete_seconds = sec.count();
		train_result.first->flops = static_cast<float>(train_result.first->get_entry_count()) * flops;

		task.history.push_back(train_result);
	}

	std::pair<std::vector<std::vector<float> >, std::string> network_trainer_sgd::prepare_learning_rates(
		unsigned int epoch,
		network_data_smart_ptr data)
	{
		float learning_rate = get_global_learning_rate(static_cast<unsigned int>(epoch));

		std::vector<std::vector<float> > res;

		for(layer_data_list::const_iterator it = data->data_list.begin(); it != data->data_list.end(); ++it)
			res.push_back(std::vector<float>((*it)->size(), learning_rate));

		std::string comment = (boost::format("LR %|1$.5e|") % learning_rate).str();

		return std::make_pair(res, comment);
	}

	void network_trainer_sgd::initialize_train(supervised_data_reader& reader)
	{
		updater->set_input_configuration_specific(reader.get_input_configuration());
	}
}
