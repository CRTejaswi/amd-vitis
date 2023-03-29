/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include "kernels.h"
#include <sstream>
#include <type_traits>

static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / 2);
static constexpr int ELEM_WITH_METADATA_RGBA = TILE_ELEMENTS_RGBA + (xf::cv::aie::METADATA_SIZE / 2);

using namespace adf;

/*
 * ADF graph to compute weighted moving average of
 * the last 8 samples in a stream of numbers
 */

template <int CORES>
class demosaicGraph : public adf::graph {
   private:
    std::array<kernel, CORES> k;

   public:
    std::array<input_plio, CORES> in1;
    std::array<output_plio, CORES> out1;

    demosaicGraph(int start_col, int start_row) { create_core<0>(start_col, start_row); }

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX >= CORES)>::type* = nullptr>
    void create_core(int col, int row) {}

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX < CORES)>::type* = nullptr>
    void create_core(int col, int row) {
        k[CORE_IDX] = kernel::create_object<DemosaicRunner>(std::vector<int16_t>({0}), std::vector<int16_t>({0}),
                                                            std::vector<int16_t>({0}), std::vector<int16_t>({0}),
                                                            std::vector<int16_t>({0}));

        std::stringstream ssi;
        ssi << "DataIn" << CORE_IDX;
        in1[CORE_IDX] = input_plio::create(ssi.str().c_str(), adf::plio_64_bits, "data/input.txt");

        std::stringstream sso;
        sso << "DataOut" << CORE_IDX;
        out1[CORE_IDX] = output_plio::create(sso.str().c_str(), adf::plio_64_bits, "data/output.txt");

        // create nets to connect kernels and IO ports
        connect<>(in1[CORE_IDX].out[0], k[CORE_IDX].in[0]);
        connect<>(k[CORE_IDX].out[0], out1[CORE_IDX].in[0]);

        adf::dimensions(k[CORE_IDX].in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k[CORE_IDX].out[0]) = {ELEM_WITH_METADATA_RGBA};

        // specify kernel sources
        source(k[CORE_IDX]) = "xf_demosaicing.cc";

        // location constraints
        location<kernel>(k[CORE_IDX]) = tile(col, row);

        runtime<ratio>(k[CORE_IDX]) = 1.0;

        create_core<CORE_IDX + 1>(col, (row + 1));
    }
};

#endif
