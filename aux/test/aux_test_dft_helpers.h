// This is only for sharing some common code betweeen different
// aux/test/*.cxx tests.  Not for "real" use.

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Persist.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDFT.h"

#include <cassert>
#include <iostream>

namespace WireCell::Aux::Test {


    // fixme: add support for config
    IDFT::pointer make_dft(const std::string& tn="FftwDFT",
                           const std::string& pi="WireCellAux",
                           Configuration cfg = Configuration())
    {
        std::cerr << "Making DFT " << tn << " from plugin " << pi << std::endl;

        PluginManager& pm = PluginManager::instance();
        pm.add(pi);
        
        // create first
        auto idft = Factory::lookup_tn<IDFT>(tn);
        assert(idft);
        // configure before use if configurable
        auto icfg = Factory::find_maybe_tn<IConfigurable>(tn);
        if (icfg) {
            auto def_cfg = icfg->default_configuration();
            def_cfg = update(def_cfg, cfg);
            icfg->configure(def_cfg);
        }
        return idft;
    }
    IDFT::pointer make_dft_args(int argc, char* argv[]) 
    {
        std::string dft_tn="FftwDFT";
        std::string dft_pi="WireCellAux";
        if (argc > 1) dft_tn = argv[1];
        if (argc > 2) dft_pi = argv[2];
        Configuration cfg;
        if (argc > 3) {
            // Either we get directly a "data" object 
            cfg = Persist::load(argv[3]);
            // or we go searching a list for matching type/name.
            if (cfg.isArray()) {
                for (auto one : cfg) {
                    std::string tn = get<std::string>(one, "type");
                    std::string n = get<std::string>(one, "name", "");
                    if (not n.empty()) {
                        tn = tn + ":" + n;
                    }
                    if (tn == dft_tn) {
                        cfg = one["data"];
                        break;
                    }
                }
            }

        }
        return make_dft(dft_tn, dft_pi);
    }

    const double default_eps = 1e-8;
    const std::complex<float> czero = 0.0;
    const std::complex<float> cone = 1.0;

    void assert_small(double val, double eps = default_eps) {
        if (val < eps) {
            return;
        }
        std::stringstream ss;
        ss << "value " << val << " >= " << eps;
        std::cerr << ss.str() << std::endl;
        THROW(WireCell::ValueError() << errmsg{ss.str()});
    }

    // Assert the array has only value val at index and near zero elsewhere
    template <typename ValueType>
    void assert_impulse_at_index(const ValueType* vec, size_t size,
                                 size_t index=0, ValueType val = 1.0)
    {
        ValueType tot = 0;
        for (size_t ind=0; ind<size; ++ind) {
            auto v = vec[ind];
            if (ind == index) {
                v -= val;
            }
            tot += v;
            assert_small(std::abs(v));
        }
        assert_small(std::abs(tot));
    }

    // Same as above but with pass by vector 
    template <typename VectorType>
    void assert_impulse_at_index(const VectorType& vec, 
                                 size_t index=0, const typename VectorType::value_type& val = 1.0)
    {
        assert_impulse_at_index(vec.data(), vec.size(), index, val);
    }

    // Assert all values in array are near given val
    template <typename ValueType>
    void assert_flat_value(const ValueType* vec, size_t size, ValueType val = 1.0)
    {
        ValueType tot = 0;
        for (size_t ind=0; ind<size; ++ind) {
            auto v = vec[ind];
            tot += v;
            assert_small(std::abs(v - val));
        }
        assert_small(std::abs(std::abs(tot) - std::abs(val)*size));
    }

    // As above but pass by vector
    template <typename VectorType>
    void assert_flat_value(const VectorType& vec, const typename VectorType::value_type& val = 1.0)
    {
        assert_flat_value(vec.data(), vec.size(), val);
    }

    // Print eigen array
    template<typename array_type>
    void dump(std::string name, const array_type& arr)
    {
        std::cerr << name << ":(" << arr.rows() << "," << arr.cols() << ") row-major:" << arr.IsRowMajor << "\n";
    }


    // Like std::iota, but dummer
    template<typename ValueType>
    void iota(ValueType* vec, size_t size, ValueType start = 0)
    {
        for (size_t ind=0; ind<size; ++ind) {
            vec[ind] = start++;
        }
    }
}
