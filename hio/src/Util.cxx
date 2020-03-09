#include "WireCellHio/Util.h"

#include <mutex>
#include <thread>

using namespace WireCell;

namespace WireCell {
namespace Hio {
std::mutex g_h5cpp_mutex;
}
} // namespace WireCell

// h5::fd_t Hio::create(const std::string &path, unsigned flags,
//                      const h5::fcpl_t &fcpl, const h5::fapl_t &fapl) {
//   std::lock_guard<std::mutex> guard(Hio::g_h5cpp_mutex);
//   return h5::create(path, flags, fcpl, fapl);
// }

// template <class T, class... args_t>
// h5::ds_t Hio::write(const h5::fd_t &fd, const std::string &dataset_path,
//                     const T *ptr, args_t &&... args) {
//   std::lock_guard<std::mutex> guard(Hio::g_h5cpp_mutex);
//   return h5::write(fd, dataset_path, ptr, args...);
// }

// h5::fd_t Hio::open(const std::string &path, unsigned flags,
//                    const h5::fapl_t &fapl) {
//   std::lock_guard<std::mutex> guard(Hio::g_h5cpp_mutex);
//   return h5::open(path, flags, fapl);
// }

// template <class T, class... args_t>
// void Hio::read(const h5::fd_t &fd, const std::string &dataset_path, T *ptr,
//                args_t &&... args) {
//   std::lock_guard<std::mutex> guard(Hio::g_h5cpp_mutex);
//   h5::read(fd, dataset_path, ptr, args...);
// }

// template <class T, class... args_t>
// T Hio::read(const h5::fd_t &fd, const std::string &dataset_path,
//             args_t &&... args) {
//   std::lock_guard<std::mutex> guard(Hio::g_h5cpp_mutex);
//   return h5::read<T>(fd, dataset_path, args...);
// }