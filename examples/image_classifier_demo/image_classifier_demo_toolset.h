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

#include <nnforge/toolset.h>

#include <map>

#include <boost/filesystem/fstream.hpp>
#include <mutex>
#include <chrono>

#include <nnforge/nnforge.h>

#include <opencv2/core/core.hpp>

class image_classifier_demo_toolset : public nnforge::toolset, public nnforge::structured_data_bunch_reader, public nnforge::structured_data_bunch_writer
{
public:
	image_classifier_demo_toolset(nnforge::factory_generator::ptr factory);

	virtual ~image_classifier_demo_toolset();

public:
	virtual std::map<std::string, nnforge::layer_configuration_specific> get_config_map() const;

	virtual bool read(
		unsigned int entry_id,
		const std::map<std::string, float *>& data_map);

	virtual void set_epoch(unsigned int epoch_id);

	virtual int get_entry_count() const;

protected:
	virtual std::string get_default_action() const;

	virtual void do_custom_action();

	virtual std::vector<nnforge::bool_option> get_bool_options();

	virtual std::vector<nnforge::string_option> get_string_options();

	virtual void set_config_map(const std::map<std::string, nnforge::layer_configuration_specific> config_map);

	virtual void write(
		unsigned int entry_id,
		const std::map<std::string, const float *>& data_map);

private:
	void run_demo();

	void run_classifier_loop();

	void add_classifier_results(cv::Mat3b frame);

	std::shared_ptr<std::vector<float> > safe_peek_input_data();

	void safe_set_input_data(std::shared_ptr<std::vector<float> > val);

	std::shared_ptr<std::vector<std::pair<unsigned int, float> > > safe_peek_output_data();

	void safe_set_output_data(std::shared_ptr<std::vector<std::pair<unsigned int, float> > > val);

	bool safe_peek_demo_should_stop();

	void safe_set_demo_should_stop(bool val);

	float safe_peek_fps();

	void safe_set_fps(float val);

	void init_input_config();

	void set_input_data(cv::Mat original_image, bool truncate_image = true);

	void report_stats();

	void load_cls_class_info();

	std::string get_class_name_by_class_id(unsigned int class_id) const;

	void add_classifier_fps(cv::Mat3b frame, float fps);

	void add_classifier_output(cv::Mat3b frame, const std::vector<std::pair<unsigned int, float> >& output_data);

	void init_draw_params();

	void dump_help() const;

	void save_image(cv::Mat3b frame);

private:
	bool demo_should_stop;
	std::mutex demo_should_stop_mutex;

	std::shared_ptr<std::vector<float> > input_data_smart_ptr;
	std::mutex input_data_mutex;

	std::shared_ptr<std::vector<std::pair<unsigned int, float> > > output_data_smart_ptr;
	std::mutex output_data_mutex;

	std::chrono::high_resolution_clock::time_point last_write;
	float fps;
	std::mutex fps_mutex;

	std::string error_message;

	nnforge::layer_configuration_specific input_config;
	nnforge::normalize_data_transformer::ptr normalizer;

	bool should_report_stats;
	std::string input_layer_name;

	std::chrono::high_resolution_clock::time_point last_report_time;

	std::map<unsigned int, std::string> class_id_to_class_name_map;

	int text_height;
	double font_scale;
	int text_thickness;
	float text_prob_part;

	static const unsigned int top_n;
	static const char * class_names_filename;
	static const int border;
	static const int border_text;
	static const int font_face;
	static const float prob_part;

	friend struct callable;
};

struct callable
{
	callable(image_classifier_demo_toolset& ts)
	: ts(ts)
	{
	}

    void operator()()
    {
    	ts.run_classifier_loop();
    }

private:
	image_classifier_demo_toolset& ts;
};
