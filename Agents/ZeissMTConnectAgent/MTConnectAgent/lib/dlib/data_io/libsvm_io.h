// Copyright (C) 2010  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_LIBSVM_iO_H__
#define DLIB_LIBSVM_iO_H__

#include "libsvm_io_abstract.h"

#include <fstream>
#include <string>
#include <utility>
#include "../algs.h"
#include "../matrix.h"
#include "../string.h"
#include <vector>

namespace dlib
{
    struct sample_data_io_error : public error
    {
        sample_data_io_error(const std::string& message): error(message) {}
    };

// ----------------------------------------------------------------------------------------

    namespace impl
    {
        template <typename T> struct strip_const { typedef T type; };
        template <typename T> struct strip_const<const T> { typedef T type; };
        template <typename T> struct strip_const<const T&> { typedef T type; };
    }

    template <typename sample_type, typename label_type, typename alloc1, typename alloc2>
    void load_libsvm_formatted_data (
        const std::string& file_name,
        std::vector<sample_type, alloc1>& samples,
        std::vector<label_type, alloc2>& labels
    )
    {
        using namespace std;
        typedef typename sample_type::value_type pair_type;
        typedef typename impl::strip_const<typename pair_type::first_type>::type key_type;
        typedef typename pair_type::second_type value_type;

        // You must use unsigned integral key types in your sparse vectors
        COMPILE_TIME_ASSERT(is_unsigned_type<key_type>::value);

        samples.clear();

        ifstream fin(file_name.c_str());

        if (!fin)
            throw sample_data_io_error("Unable to open file " + file_name);

        string line;
        istringstream sin;
        key_type key;
        value_type value;
        label_type label;
        sample_type sample;
        long line_num = 0;
        while (fin.peek() != EOF)
        {
            ++line_num;
            getline(fin, line);

            string::size_type pos = line.find_first_not_of(" \t\r\n");

            // ignore empty lines or comment lines
            if (pos == string::npos || line[pos] == '#')
                continue;

            sin.clear();
            sin.str(line);
            sample.clear();

            sin >> label;

            if (!sin)
                throw sample_data_io_error("On line: " + cast_to_string(line_num) + ", error while reading file " + file_name );

            // eat whitespace
            sin >> ws;

            while (sin.peek() != EOF && sin.peek() != '#')
            {

                sin >> key >> ws;

                // ignore what should be a : character
                if (sin.get() != ':')
                    throw sample_data_io_error("On line: " + cast_to_string(line_num) + ", error while reading file " + file_name);

                sin >> value >> ws;

                if (sin && value != 0)
                {
                    sample.insert(sample.end(), make_pair(key, value));
                }
            }

            samples.push_back(sample);
            labels.push_back(label);
        }

    }

// ----------------------------------------------------------------------------------------

// This is an overload for sparse vectors
    template <typename sample_type, typename label_type, typename alloc1, typename alloc2>
    typename disable_if<is_matrix<sample_type>,void>::type save_libsvm_formatted_data (
        const std::string& file_name,
        const std::vector<sample_type, alloc1>& samples,
        const std::vector<label_type, alloc2>& labels
    )
    {
        typedef typename sample_type::value_type pair_type;
        typedef typename impl::strip_const<typename pair_type::first_type>::type key_type;

        // You must use unsigned integral key types in your sparse vectors
        COMPILE_TIME_ASSERT(is_unsigned_type<key_type>::value);

        // make sure requires clause is not broken
        DLIB_ASSERT(samples.size() == labels.size(),
            "\t void save_libsvm_formatted_data()"
            << "\n\t You have to have labels for each sample and vice versa"
            << "\n\t samples.size(): " << samples.size()
            << "\n\t labels.size():  " << labels.size()
            );


        using namespace std;
        ofstream fout(file_name.c_str());
        fout.precision(14);

        if (!fout)
            throw sample_data_io_error("Unable to open file " + file_name);

        for (unsigned long i = 0; i < samples.size(); ++i)
        {
            fout << labels[i];

            for (typename sample_type::const_iterator j = samples[i].begin(); j != samples[i].end(); ++j)
            {
                if (j->second != 0)
                    fout << " " << j->first << ":" << j->second;
            }
            fout << "\n";

            if (!fout)
                throw sample_data_io_error("Error while writing to file " + file_name);
        }

    }

// ----------------------------------------------------------------------------------------

// This is an overload for dense vectors
    template <typename sample_type, typename label_type, typename alloc1, typename alloc2>
    typename enable_if<is_matrix<sample_type>,void>::type save_libsvm_formatted_data (
        const std::string& file_name,
        const std::vector<sample_type, alloc1>& samples,
        const std::vector<label_type, alloc2>& labels
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT(samples.size() == labels.size(),
            "\t void save_libsvm_formatted_data()"
            << "\n\t You have to have labels for each sample and vice versa"
            << "\n\t samples.size(): " << samples.size()
            << "\n\t labels.size():  " << labels.size()
            );

        using namespace std;
        ofstream fout(file_name.c_str());
        fout.precision(14);

        if (!fout)
            throw sample_data_io_error("Unable to open file " + file_name);

        for (unsigned long i = 0; i < samples.size(); ++i)
        {
            fout << labels[i];

            for (long j = 0; j < samples[i].size(); ++j)
            {
                if (samples[i](j) != 0)
                    fout << " " << j << ":" << samples[i](j);
            }
            fout << "\n";

            if (!fout)
                throw sample_data_io_error("Error while writing to file " + file_name);
        }

    }

// ----------------------------------------------------------------------------------------

    template <typename sample_type, typename alloc>
    std::vector<matrix<typename sample_type::value_type::second_type,0,1> > sparse_to_dense (
        const std::vector<sample_type, alloc>& samples
    )
    {
        typedef typename sample_type::value_type pair_type;
        typedef typename impl::strip_const<typename pair_type::first_type>::type key_type;

        // You must use unsigned integral key types in your sparse vectors
        COMPILE_TIME_ASSERT(is_unsigned_type<key_type>::value);

        typedef typename sample_type::value_type pair_type;
        typedef typename impl::strip_const<typename pair_type::first_type>::type key_type;
        typedef typename pair_type::second_type value_type;

        std::vector< matrix<value_type,0,1> > result;

        // do nothing if there aren't any samples
        if (samples.size() == 0)
            return result;


        // figure out how many elements we need in our dense vectors.  
        unsigned long max_dim = 0;
        for (unsigned long i = 0; i < samples.size(); ++i)
        {
            if (samples[i].size() > 0)
                max_dim = std::max<unsigned long>(max_dim, (--samples[i].end())->first + 1);
        }


        // now turn all the samples into dense samples
        result.resize(samples.size());

        for (unsigned long i = 0; i < samples.size(); ++i)
        {
            result[i].set_size(max_dim);
            result[i] = 0;
            for (typename sample_type::const_iterator j = samples[i].begin(); j != samples[i].end(); ++j)
            {
                result[i](j->first) = j->second;
            }
        }

        return result;
    }

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_LIBSVM_iO_H__

