/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <vector>

#include <jellyfish/err.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jellyfish/stream_manager.hpp>
#include <jellyfish/mer_dna_bloom_counter.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/file_header.hpp>
#include <sub_commands/bf_main_cmdline.hpp>

static bf_main_cmdline args; // Command line switches and arguments
typedef std::vector<const char*> file_vector;
using jellyfish::mer_dna;
using jellyfish::mer_dna_bloom_counter;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_manager<file_vector::iterator> > sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, jellyfish::mer_dna> mer_iterator;

template<typename PathIterator>
class mer_bloom_counter : public jellyfish::thread_exec {
  int                                     nb_threads_;
  mer_dna_bloom_counter&                  filter_;
  jellyfish::stream_manager<PathIterator> streams_;
  sequence_parser                         parser_;

public:
  mer_bloom_counter(int nb_threads, mer_dna_bloom_counter& filter, PathIterator file_begin, PathIterator file_end) :
    filter_(filter),
    streams_(file_begin, file_end),
    parser_(jellyfish::mer_dna::k(), 1, 3 * nb_threads, 4096, streams_)
  { }

  virtual void start(int thid) {
    for(mer_iterator mers(parser_, args.canonical_flag) ; mers; ++mers) {
      filter_.insert(*mers);
    }
  }
};


int bf_main(int argc, char *argv[])
{
  jellyfish::file_header header;
  header.fill_standard();
  header.set_cmdline(argc, argv);

  args.parse(argc, argv);
  mer_dna::k(args.mer_len_arg);

  std::ofstream output(args.output_arg);
  if(!output.good())
    die << "Can't open output file '" << args.output_arg << "'";


  header.format("bloomcounter");
  header.key_len(args.mer_len_arg * 2);
  jellyfish::hash_pair<mer_dna> hash_fns;
  header.matrix(hash_fns.m1, 1);
  header.matrix(hash_fns.m2, 2);

  mer_dna_bloom_counter filter(args.fpr_arg, args.size_arg, hash_fns);
  header.size(filter.m());
  header.nb_hashes(filter.k());
  header.write(output);

  mer_bloom_counter<file_vector::iterator> counter(args.threads_arg, filter, args.file_arg.begin(), args.file_arg.end());
  counter.exec_join(args.threads_arg);

  filter.write_bits(output);
  output.close();

  return 0;
}