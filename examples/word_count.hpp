/*******************************************************************************
 * examples/word_count.cpp
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2015 Alexander Noe <aleexnoe@gmail.com>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <thrill/api/generate_from_file.hpp>
#include <thrill/api/read_lines.hpp>
#include <thrill/api/reduce.hpp>
#include <thrill/api/size.hpp>
#include <thrill/api/write_lines_many.hpp>
#include <thrill/common/string.hpp>
#include <thrill/api/generate.hpp>
#include <thrill/api/groupby_index.hpp>
#include <thrill/api/read_lines.hpp>
#include <thrill/api/reduce.hpp>
#include <thrill/api/reduce_to_index.hpp>
#include <thrill/api/size.hpp>
#include <thrill/api/sort.hpp>
#include <thrill/api/sum.hpp>
#include <thrill/api/write_lines.hpp>
#include <thrill/api/zip.hpp>
#include <thrill/common/cmdline_parser.hpp>
#include <thrill/common/logger.hpp>
#include <thrill/common/stats_timer.hpp>

#include <algorithm>
#include <random>
#include <string>
#include <utility>
#include <iostream>

using thrill::DIA;
using thrill::Context;

using namespace thrill; // NOLINT

namespace examples {

using WordCountPair = std::pair<std::string, size_t>;

//! The WordCount user program: reads a DIA containing std::string words, and
//! returns a DIA containing WordCountPairs.
template <typename InStack>
auto WordCount(const DIA<std::string, InStack> &input) {

    auto word_pairs = input.template FlatMap<WordCountPair>(
            [](const std::string &line, auto emit) -> void {
                /* map lambda: emit each word */
                for (const std::string &word : common::Split(line, ' ')) {
                    if (word.size() != 0)
                        emit(WordCountPair(word, 1));
                }
            });

    return word_pairs.ReduceBy(
            [](const WordCountPair &in) -> std::string {
                /* reduction key: the word string */
                return in.first;
            },
            [](const WordCountPair &a, const WordCountPair &b) -> WordCountPair {
                /* associative reduction operator: add counters */
                return WordCountPair(a.first, a.second + b.second);
            });
}

}

/******************************************************************************/