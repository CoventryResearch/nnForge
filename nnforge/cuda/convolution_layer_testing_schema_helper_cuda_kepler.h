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

#pragma once

#include "layer_tester_cuda.h"

namespace nnforge
{
	namespace cuda
	{
		class convolution_layer_testing_schema_helper_cuda_kepler
		{
		public:
			static layer_tester_cuda::ptr create_tester_specific(
				const layer_configuration_specific& input_configuration_specific,
				const layer_configuration_specific& output_configuration_specific);
		};
	}
}
