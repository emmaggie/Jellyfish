#include <fstream>

#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/mer_iterator.hpp>

namespace {
using std::string;
using jellyfish::mer_dna;
typedef std::vector<string> string_vector;
template<typename Iterator>
struct opened_streams {
  Iterator begin_, end_;

  opened_streams(Iterator begin, Iterator end) : begin_(begin), end_(end) { }
  std::unique_ptr<std::istream> next() {
    std::unique_ptr<std::istream> res;
    if(begin_ != end_) {
      res.reset(*begin_);
      ++begin_;
    }
    return res;
  }
};
typedef jellyfish::mer_overlap_sequence_parser<opened_streams<std::ifstream**> > parser_type;
typedef jellyfish::mer_iterator<parser_type, mer_dna> mer_iterator_type;

string generate_sequence(int len) {
  static const char bases[4] = { 'A', 'C', 'G', 'T' };
  string res;

  for(int i = 0; i < len; ++i)
    res += bases[random() % 0x3];

  return res;
}

TEST(MerIterator, Sequence) {
  static const int nb_sequences = 100;
  const char* file_name = "Sequence.fa";
  file_unlink fu(file_name);
  string_vector sequences;
  mer_dna::k(35);

  {
    std::ofstream input_fasta(file_name);
    for(int i = 0; i < nb_sequences; ++i) {
      sequences.push_back(generate_sequence(20 + random() % 200));
      input_fasta << ">" << i << "\n" << sequences.back() << "\n";
    }
  }

  // Check that every mer of the iterator matches the sequence
  auto input_fasta = new std::ifstream(file_name);
  {
    opened_streams<std::ifstream**> streams(&input_fasta, &input_fasta + 1);
    parser_type parser(mer_dna::k(), 1, 10, 100, streams);
    mer_iterator_type mit(parser);
    for(string_vector::const_iterator it = sequences.begin(); it != sequences.end(); ++it) {
      if(it->size() >= mer_dna::k()) {
        for(int i = 0; i < it->size() - (mer_dna::k() - 1); ++i, ++mit) {
          ASSERT_NE(mer_iterator_type(), mit);
          EXPECT_EQ(it->substr(i, mer_dna::k()), mit->to_str());
        }
      }
    }
    EXPECT_EQ(mer_iterator_type(), mit);
  }
}

  // Same but with canonical mers
TEST(MerIterator, SequenceCanonical) {
  const char* file_name = "SequenceCanonical.fa";
  file_unlink fu(file_name);
  string_vector sequences;
  static const int nb_sequences = 100;
  mer_dna::k(35);

  {
    std::ofstream input_fasta(file_name);
    for(int i = 0; i < nb_sequences; ++i) {
      sequences.push_back(generate_sequence(20 + random() % 200));
      input_fasta << ">" << i << "\n" << sequences.back() << "\n";
    }
  }

  auto input_fasta = new std::ifstream(file_name);
  {
    opened_streams<std::ifstream**> streams(&input_fasta, &input_fasta + 1);
    parser_type parser(mer_dna::k(), 1, 10, 100, streams);
    mer_iterator_type mit(parser, true);
    for(string_vector::const_iterator it = sequences.begin(); it != sequences.end(); ++it) {
      if(it->size() >= mer_dna::k()) {
        for(int i = 0; i < it->size() - (mer_dna::k() - 1); ++i, ++mit) {
          ASSERT_NE(mer_iterator_type(), mit);
          ASSERT_EQ(*mit, mit->get_canonical());
          EXPECT_TRUE((it->substr(i, mer_dna::k()) == mit->to_str()) ||
                      (it->substr(i, mer_dna::k()) == mit->get_reverse_complement().to_str()));
        }
      }
    }
    EXPECT_EQ(mer_iterator_type(), mit);
  }
}
} // namespace {
